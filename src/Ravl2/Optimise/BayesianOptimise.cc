#include "Ravl2/Optimise/BayesianOptimise.hh"
#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <cassert>
#include <limits>

namespace Ravl2
{

  BayesianOptimise::BayesianOptimise(size_t maxIters, size_t batchSize, size_t maxThreads, bool verbose,
                                   RealT gpLengthScale, RealT gpNoise, bool fixedSeed, unsigned seed,
                                   RealT tolerance, size_t minItersBeforeConvergence,
                                   RealT explorationWeight, RealT localSearchRadius,
                                   bool adaptiveLengthScale)
    : Optimise(verbose),
     mBatchSize(batchSize),
     mMaxIters(maxIters),
     mMaxThreads(maxThreads),
     mFixedSeed(fixedSeed),
     mSeed(seed),
     mGPLengthScale(gpLengthScale),
     mGPNoise(gpNoise),
     mTolerance(tolerance),
     mMinItersBeforeConvergence(minItersBeforeConvergence),
     mExplorationWeight(explorationWeight),
     mLocalSearchRadius(localSearchRadius),
     mAdaptiveLengthScale(adaptiveLengthScale)
  {
    // Parameter validation
    if(maxIters == 0) {
      SPDLOG_ERROR("Invalid parameter: maxIters must be greater than 0, got {} ({}:{})", maxIters, __FILE__, __LINE__);
      throw std::invalid_argument("maxIters must be greater than 0");
    }
    if(batchSize == 0) {
      SPDLOG_ERROR("Invalid parameter: batchSize must be greater than 0, got {} ({}:{})", batchSize, __FILE__, __LINE__);
      throw std::invalid_argument("batchSize must be greater than 0");
    }
    if(gpLengthScale <= 0) {
      SPDLOG_ERROR("Invalid parameter: gpLengthScale must be positive, got {} ({}:{})", gpLengthScale, __FILE__, __LINE__);
      throw std::invalid_argument("gpLengthScale must be positive");
    }
    if(gpNoise <= 0) {
      SPDLOG_ERROR("Invalid parameter: gpNoise must be positive, got {} ({}:{})", gpNoise, __FILE__, __LINE__);
      throw std::invalid_argument("gpNoise must be positive");
    }
    if(tolerance <= 0) {
      SPDLOG_ERROR("Invalid parameter: tolerance must be positive, got {} ({}:{})", tolerance, __FILE__, __LINE__);
      throw std::invalid_argument("tolerance must be positive");
    }
    if(minItersBeforeConvergence >= maxIters) {
      SPDLOG_ERROR("Invalid parameter: minItersBeforeConvergence must be less than maxIters, got {} >= {} ({}:{})",
                   minItersBeforeConvergence, maxIters, __FILE__, __LINE__);
      throw std::invalid_argument("minItersBeforeConvergence must be less than maxIters");
    }

    // Create default sample generators
    mInitialGenerator = std::make_shared<SobolSampleGenerator>();
    mAcquisitionGenerator = std::make_shared<AcquisitionSampleGenerator>(
      explorationWeight, localSearchRadius, 0, 100, fixedSeed, seed);
  }

  BayesianOptimise::BayesianOptimise(Configuration &config)
      : Optimise(config),
      mBatchSize(config.getUnsigned("batchSize","Batch size.", 1, 1, 100)),
      mMaxIters(config.getUnsigned("maxIters","Maximum number of iterations", 20, 1, 1000)),
      mMaxThreads(config.getUnsigned("maxThreads","Maximum number of threads for parallel evaluation (0 = single-threaded)", 0, 0, 100)),
      mFixedSeed(config.getBool("fixedSeed", "Use fixed random seed for reproducibility.", true)),
      mSeed(config.getUnsigned("seed", "Random seed value if fixedSeed is true.", 42, 0, std::numeric_limits<unsigned>::max())),
      mGPLengthScale(static_cast<RealT>(config.getNumber("gpLengthScale", "Length scale parameter for Gaussian Process kernel.", 1.0, 1e-6, 100.0))),
      mGPNoise(static_cast<RealT>(config.getNumber("gpNoise", "Noise parameter for Gaussian Process.", 1e-6, 1e-10, 1.0))),
      mTolerance(static_cast<RealT>(config.getNumber("tolerance", "Convergence tolerance for early stopping.", 1e-6, 1e-12, 1.0))),
      mMinItersBeforeConvergence(config.getUnsigned("minItersBeforeConvergence", "Minimum iterations before checking convergence.", 5, 1, 1000)),
      mExplorationWeight(static_cast<RealT>(config.getNumber("explorationWeight", "Weight for exploration vs exploitation (0=pure exploitation, 1=pure exploration).", 0.1, 0.0, 1.0))),
      mLocalSearchRadius(static_cast<RealT>(config.getNumber("localSearchRadius", "Radius for local search around best points (fraction of domain size).", 0.1, 0.01, 0.5))),
      mAdaptiveLengthScale(config.getBool("adaptiveLengthScale", "Use adaptive length scale for Gaussian Process.", false))
  {
    // Create default sample generators
    mInitialGenerator = std::make_shared<SobolSampleGenerator>();
    mAcquisitionGenerator = std::make_shared<AcquisitionSampleGenerator>(
      mExplorationWeight, mLocalSearchRadius, 0, 100, mFixedSeed, mSeed);
  }

  std::tuple<VectorT<BayesianOptimise::RealT>, BayesianOptimise::RealT> BayesianOptimise::minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start) const {
    std::vector<VectorT<RealT>> startPoints;
    if (!start.isZero(0)) {
      startPoints.push_back(start);
    }
    auto results = minimiseBatch(domain, func, startPoints);

    // Return the best result
    auto minIt = std::min_element(results.begin(), results.end(),
                                 [](const auto &a, const auto &b) {
                                   return std::get<1>(a) < std::get<1>(b);
                                 });
    return (minIt != results.end()) ? *minIt : std::make_tuple(VectorT<RealT>(), RealT(0));
  }

  std::vector<std::tuple<VectorT<BayesianOptimise::RealT>, BayesianOptimise::RealT>> BayesianOptimise::minimiseBatch(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const std::vector<VectorT<RealT>> &startPoints) const {

    // Input validation
    if(domain.dim() == 0) {
      SPDLOG_ERROR("BayesianOptimise::minimiseBatch() called with empty domain: domain.dim()={}", domain.dim());
      throw std::invalid_argument("Domain must have at least one dimension");
    }
    if(!func) {
      SPDLOG_ERROR("BayesianOptimise::minimiseBatch() called with null function pointer");
      throw std::invalid_argument("Function cannot be null");
    }

    // Validate starting points have the correct dimension
    for(const auto& point : startPoints) {
      if(point.size() != static_cast<Eigen::Index>(domain.dim())) {
        SPDLOG_ERROR("BayesianOptimise::minimiseBatch() starting point dimension mismatch: point.size()={}, domain.dim()={}", point.size(), domain.dim());
        throw std::invalid_argument("Starting point dimension doesn't match domain dimension");
      }
    }

    // Ensure we have default generators
    auto initialGen = mInitialGenerator ? mInitialGenerator : std::make_shared<SobolSampleGenerator>();
    auto acquisitionGen = mAcquisitionGenerator ? mAcquisitionGenerator :
      std::make_shared<AcquisitionSampleGenerator>(mExplorationWeight, mLocalSearchRadius, 0, 100, mFixedSeed, mSeed);

    State state(mGPLengthScale, mGPNoise, initialGen, acquisitionGen, verbose());
    std::vector<std::tuple<VectorT<RealT>, RealT>> results;
    std::vector<VectorT<RealT>> initPoints = startPoints;

    // Generate initial points if none provided
    if(initPoints.empty()) {
      initPoints = state.generateInitialPoints(domain, mBatchSize);
    }

    // Evaluate initial points using batch evaluator
    auto initResults = evaluateBatch(initPoints, func);
    for(size_t i = 0; i < initPoints.size(); ++i) {
      state.addPoint(initPoints[i], initResults[i]);
      results.emplace_back(initPoints[i], initResults[i]);
    }

    // Track the best values for convergence checking
    std::vector<RealT> recentBestValues;
    recentBestValues.reserve(mMinItersBeforeConvergence + 1);

    for(size_t iter = 0; iter < mMaxIters; ++iter) {
      state.fitSurrogate();

      // Adapt GP length scale based on optimization progress
      if(mAdaptiveLengthScale && iter > 0) {
        state.adaptLengthScale(domain, iter, mMaxIters);
      }

      auto batch = state.selectBatch(domain, mBatchSize);

      // Early termination if no candidates found
      if(batch.empty()) {
        if(verbose()) {
          SPDLOG_WARN("No candidates found at iteration {}, terminating early", iter);
        }
        break;
      }

      // Evaluate batch using parallel evaluator
      auto batchResults = evaluateBatch(batch, func);
      for(size_t i = 0; i < batch.size(); ++i) {
        state.addPoint(batch[i], batchResults[i]);
        results.emplace_back(batch[i], batchResults[i]);
      }

      const RealT currentBest = state.getBestY();
      recentBestValues.push_back(currentBest);

      // Check for convergence after minimum iterations
      if(iter >= mMinItersBeforeConvergence) {
        // Keep only the last mMinItersBeforeConvergence + 1 values
        if(recentBestValues.size() > mMinItersBeforeConvergence + 1) {
          recentBestValues.erase(recentBestValues.begin());
        }

        // Check if the improvement is below tolerance
        const RealT oldestBest = recentBestValues.front();
        const RealT improvement = oldestBest - currentBest;
        if(improvement < mTolerance) {
          if(verbose()) {
            SPDLOG_INFO("Converged at iteration {} (improvement {} < tolerance {})", iter, improvement, mTolerance);
          }
          break;
        }
      }

      if(verbose()) {
        SPDLOG_INFO("Iteration {}: best value = {}", iter, currentBest);
      }
    }

    return results;
  }

  std::vector<BayesianOptimise::RealT> BayesianOptimise::evaluateBatch(
      const std::vector<VectorT<RealT>> &batch,
      const std::function<RealT(const VectorT<RealT> &)> &func) const
  {
    if(batch.empty()) {
      return {};
    }

    std::vector<RealT> results(batch.size());

    if(mMaxThreads == 0 || batch.size() == 1) {
      // Sequential evaluation
      for(size_t i = 0; i < batch.size(); ++i) {
        results[i] = func(batch[i]);
      }
    } else {
      // Parallel evaluation
      const size_t numThreads = std::min(mMaxThreads, batch.size());
      std::vector<std::future<void>> futures;
      futures.reserve(numThreads);

      std::atomic<size_t> nextIndex{0};
      std::mutex resultMutex;

      for(size_t t = 0; t < numThreads; ++t) {
        futures.emplace_back(std::async(std::launch::async, [&]() {
          while(true) {
            size_t index = nextIndex.fetch_add(1);
            if(index >= batch.size()) break;

            RealT value = func(batch[index]);

            {
              std::lock_guard<std::mutex> lock(resultMutex);
              results[index] = value;
            }
          }
        }));
      }

      // Wait for all threads to complete
      for(auto& future : futures) {
        future.wait();
      }
    }

    return results;
  }

  void BayesianOptimise::State::fitSurrogate() {
    if(mX.empty()) return;

    const auto nDims = mX[0].size();
    const size_t nPoints = mX.size();

    // Convert data to matrix format
    typename GaussianProcess<RealT>::Matrix X(static_cast<Eigen::Index>(nPoints), nDims);
    VectorT<RealT> Y(static_cast<Eigen::Index>(nPoints));

    for(size_t i = 0; i < nPoints; ++i) {
      X.row(static_cast<Eigen::Index>(i)) = mX[i].transpose();
      Y[static_cast<Eigen::Index>(i)] = mY[i];
    }

    mGP.fit(X, Y);

    if(mVerbose) {
      SPDLOG_INFO("Fitted GP with {} data points", nPoints);
    }
  }

  std::vector<VectorT<BayesianOptimise::RealT>> BayesianOptimise::State::generateInitialPoints(
      const CostDomain<RealT> &domain, size_t numPoints) {

    mInitialGen->reset(domain);
    return mInitialGen->generatePoints(numPoints);
  }

  std::vector<VectorT<BayesianOptimise::RealT>> BayesianOptimise::State::selectBatch(
      const CostDomain<RealT> &domain, size_t batchSize) {

    // Update acquisition generator with current evaluation data
    if(auto acqGen = std::dynamic_pointer_cast<AcquisitionSampleGenerator>(mAcquisitionGen)) {
      acqGen->setEvaluationData(mX, mY, &mGP);
      acqGen->reset(domain);
      return acqGen->generatePoints(batchSize);
    } else {
      // Fallback to the general generator
      mAcquisitionGen->reset(domain);
      return mAcquisitionGen->generatePoints(batchSize);
    }
  }

  void BayesianOptimise::State::adaptLengthScale(const CostDomain<RealT> &domain, size_t iteration, size_t maxIters) {
    if(mX.empty()) return;

    // Calculate typical domain size for normalization
    RealT domainSize = 0.0;
    for(Eigen::Index d = 0; d < domain.dim(); ++d) {
      domainSize += (domain.max(d) - domain.min(d)) * (domain.max(d) - domain.min(d));
    }
    domainSize = std::sqrt(domainSize);

    // Calculate progress through optimization (0.0 to 1.0)
    RealT progress = static_cast<RealT>(iteration) / static_cast<RealT>(maxIters - 1);

    // Calculate adaptive length scale based on:
    // 1. Progress through optimization (start high, end low)
    // 2. Data density (more data points = shorter length scale)
    // 3. Function variation (high variation = longer length scale)

    // Base length scale decreases with progress: start at 2x initial, end at 0.5x initial
    RealT baseLengthScale = mGP.getLengthScale(); // Get current length scale
    RealT initialScale = baseLengthScale; // Assume this is close to the original
    RealT progressFactor = 2.0f * (1.0f - progress) + 0.5f * progress;

    // Data density factor: more points = shorter length scale
    RealT dataPoints = static_cast<RealT>(mX.size());
    RealT densityFactor = 1.0f / (1.0f + dataPoints / 20.0f); // Normalize by expected ~20 points
    densityFactor = std::max(densityFactor, static_cast<RealT>(0.2)); // Don't go too small

    // Function variation factor: estimate from observed values
    if(mY.size() > 2) {
      RealT minY = *std::min_element(mY.begin(), mY.end());
      RealT maxY = *std::max_element(mY.begin(), mY.end());
      RealT yRange = maxY - minY;

      // If function varies a lot, use longer length scale for better interpolation
      RealT variationFactor = 1.0f + 0.5f * std::min(yRange / std::abs(minY + 1e-8f), 2.0f);

      // Combine all factors
      RealT newLengthScale = initialScale * progressFactor * densityFactor * variationFactor;

      // Ensure reasonable bounds relative to domain size
      newLengthScale = std::max(newLengthScale, domainSize * 0.01f); // At least 1% of domain
      newLengthScale = std::min(newLengthScale, domainSize * 2.0f);  // At most 2x domain size

      mGP.setLengthScale(newLengthScale);

      if(mVerbose) {
        SPDLOG_INFO("Adapted GP length scale: {} -> {} (progress: {:.2f}, density: {:.2f}, variation: {:.2f})",
                   baseLengthScale, newLengthScale, progress, densityFactor, variationFactor);
      }
    }
  }

} // namespace Ravl2
