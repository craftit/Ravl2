#include "Ravl2/Optimise/BayesianOptimise.hh"
#include <random>
#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <cassert>

namespace Ravl2
{

  BayesianOptimise::BayesianOptimise(size_t maxIters, size_t batchSize, size_t maxThreads, bool verbose, RealT gpLengthScale, RealT gpNoise, bool fixedSeed, unsigned seed, RealT tolerance, size_t minItersBeforeConvergence)
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
     mExplorationWeight(0.1f),
     mLocalSearchRadius(0.1f),
     mNumLocalCandidates(0),
     mAdaptiveLengthScale(false)
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
  }

  //! Construct Bayesian optimizer from configuration
  //!
  //! Reads parameters from the provided configuration object:
  //! - batchSize: Number of points to evaluate in parallel
  //! - maxIters: Maximum number of optimization iterations
  //! - maxThreads: Maximum number of threads for parallel evaluation
  //! - fixedSeed: Whether to use fixed random seed
  //! - seed: Seed value if using fixed seed
  //! - gpLengthScale: Length scale parameter for GP kernel
  //! - gpNoise: Noise parameter for GP
  //! - tolerance: Convergence tolerance for early stopping
  //! - minItersBeforeConvergence: Minimum iterations before checking convergence
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
      mNumLocalCandidates(config.getUnsigned("numLocalCandidates", "Number of candidates to generate around best points (0=auto: batchSize*2).", 0, 0, 1000)),
      mAdaptiveLengthScale(config.getBool("adaptiveLengthScale", "Whether to adapt GP length scale during optimization.", false))
  {
  }

  //! Find the minimum of a function
  //!
  //! Uses Bayesian optimization to find the global minimum.
  //! If a starting point is provided, it will be used as the first evaluation.
  //!
  //! @return Tuple containing the minimum point and its value
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

  //! Find multiple minima of a function in batch mode
  //!
  //! Uses batch Bayesian optimization to explore the function space.
  //! Each iteration evaluates multiple points in parallel.
  //!
  //! @return Vector of evaluated points and their function values
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

    State state(mFixedSeed, mSeed, mGPLengthScale, mGPNoise,verbose());
    std::vector<std::tuple<VectorT<RealT>, RealT>> results;
    std::vector<VectorT<RealT>> initPoints = startPoints;

    // Generate random initial points if none provided
    if(initPoints.empty()) {
      initPoints.reserve(mBatchSize);
      for(size_t i = 0; i < mBatchSize; ++i) {
        VectorT<RealT> x(static_cast<Eigen::Index>(domain.dim()));
        for(Eigen::Index d = 0; d < x.size(); ++d) {
          std::uniform_real_distribution<RealT> dist(domain.min(d), domain.max(d));
          x[d] = dist(state.mGen);
        }
        initPoints.push_back(x);
      }
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
        state.adaptLengthScale(domain, mAdaptiveLengthScale);
      }

      auto batch = state.selectBatchImproved(domain, mBatchSize, mExplorationWeight, mLocalSearchRadius, mNumLocalCandidates);

      // Early termination if no candidates found
      if(batch.empty()) {
        if(mVerbose) {
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

        // Check if improvement over the last mMinItersBeforeConvergence iterations is below tolerance
        const RealT oldBest = recentBestValues.front();
        const RealT improvement = oldBest - currentBest;  // Improvement (should be positive for minimization)

        if(improvement <= mTolerance) {
          if(mVerbose) {
            SPDLOG_INFO("Converged at iteration {} (improvement: {} <= tolerance: {})",
                       iter, improvement, mTolerance);
          }
          break;
        }
      }

      if(mVerbose) {
        SPDLOG_INFO("Iteration {}, best y: {}", iter, currentBest);
      }
    }
    return results;
  }

  //! Fit Gaussian Process surrogate model to current data
  //!
  //! Converts vector data to matrix format and fits the GP model.
  void BayesianOptimise::State::fitSurrogate() {
    if(mX.empty() || mY.empty()) return;

    // Ensure consistent data sizes
    assert(mX.size() == mY.size() && "Mismatch between number of X points and Y values");

    // Convert vector of vectors to matrix efficiently
    const auto nSamples = static_cast<Eigen::Index>(mX.size());
    const auto nFeatures = mX[0].size();

    // Validate all vectors have the same dimension
#ifndef NDEBUG
    for(size_t i = 1; i < mX.size(); ++i) {
      assert(mX[i].size() == nFeatures && "Inconsistent input vector dimensions");
    }
#endif

    typename GaussianProcess<RealT>::Matrix Xmat(nSamples, nFeatures);
    VectorT<RealT> yvec(nSamples);

    // Use Eigen's Map for efficient data transfer
    for(Eigen::Index i = 0; i < nSamples; ++i) {
      Xmat.row(i) = mX[static_cast<size_t>(i)].transpose();
      yvec[i] = mY[static_cast<size_t>(i)];
    }

    mGP.fit(Xmat, yvec);

    // Invalidate prediction and EI caches since the GP model changed
    mPredictionsCacheValid = false;
    mEICacheValid = false;
  }

  //! Predict function value at a point using a surrogate model
  //!
  //! @param x Point to predict
  //! @return Predicted function value
  BayesianOptimise::RealT BayesianOptimise::State::predictSurrogate(const VectorT<RealT> &x) const {
    // Convert single vector to matrix format efficiently
    typename GaussianProcess<RealT>::Matrix Xtest(1, x.size());
    Xtest.row(0) = x.transpose();
    VectorT<RealT> mean, var;
    mGP.predict(Xtest, mean, var);
    return mean[0];
  }

  //! Generate or retrieve cached candidate points
  //! @param domain Domain of the function
  //! @param nCandidates Number of candidates to generate
  const std::vector<VectorT<BayesianOptimise::RealT>>& BayesianOptimise::State::getCandidates(
      const CostDomain<RealT> &domain, size_t nCandidates) {

    if(!mCandidatesCacheValid || mCandidatesCache.size() != nCandidates) {
      mCandidatesCache.clear();
      mCandidatesCache.reserve(nCandidates);

      // Generate random candidates
      const auto nDims = static_cast<Eigen::Index>(domain.dim());
      for(size_t i = 0; i < nCandidates; ++i) {
        VectorT<RealT> x(nDims);
        for(Eigen::Index d = 0; d < nDims; ++d) {
          std::uniform_real_distribution<RealT> dist(domain.min(d), domain.max(d));
          x[d] = dist(mGen);
        }
        mCandidatesCache.push_back(x);
      }
      mCandidatesCacheValid = true;
      // Invalidate dependent caches
      mPredictionsCacheValid = false;
      mEICacheValid = false;
    }

    return mCandidatesCache;
  }

  //! Compute or retrieve cached GP predictions
  //! @param candidates Candidate points to predict
  void BayesianOptimise::State::computePredictions(const std::vector<VectorT<RealT>>& candidates) {
    if(!mPredictionsCacheValid) {
      const size_t nCandidates = candidates.size();
      const auto nDims = candidates[0].size();

      // Convert candidates to matrix format
      typename GaussianProcess<RealT>::Matrix Xtest(static_cast<Eigen::Index>(nCandidates), nDims);
      for(size_t i = 0; i < nCandidates; ++i) {
        Xtest.row(static_cast<Eigen::Index>(i)) = candidates[i].transpose();
      }

      // Predict with the surrogate model
      mGP.predict(Xtest, mMeanCache, mVarianceCache);
      mPredictionsCacheValid = true;
      // Invalidate EI cache since predictions changed
      mEICacheValid = false;
    }
  }

  //! Compute or retrieve cached Expected Improvement scores
  //! @param bestY Current best function value
  void BayesianOptimise::State::computeExpectedImprovement(RealT bestY) {
    if(!mEICacheValid || std::abs(mLastBestY - bestY) > std::numeric_limits<RealT>::epsilon()) {
      const size_t nCandidates = static_cast<size_t>(mMeanCache.size());
      mEICache.clear();
      mEICache.reserve(nCandidates);

      // Calculate expected improvement for each candidate
      for(size_t i = 0; i < nCandidates; ++i) {
        const auto mu = mMeanCache[static_cast<Eigen::Index>(i)];
        const auto sigma = std::sqrt(std::max(mVarianceCache[static_cast<Eigen::Index>(i)], static_cast<RealT>(1e-9)));
        const auto z = static_cast<double>((bestY - mu) / sigma);
        const auto ei = sigma > 0 ?
                    static_cast<double>(bestY - mu) * std::erfc(-z/std::sqrt(2))/2 +
                    static_cast<double>(sigma) * std::exp(-0.5*z*z)/std::sqrt(2*M_PI) : 0;
        mEICache.emplace_back(static_cast<RealT>(ei), i);
      }

      // Sort by EI (descending) - this is expensive, so we cache it
      std::sort(mEICache.rbegin(), mEICache.rend());

      mEICacheValid = true;
      mLastBestY = bestY;
    }
  }

  //! Select the next batch of points to evaluate with intelligent caching
  //!
  //! Uses cached Expected Improvement acquisition function to select promising points.
  //!
  //! @param domain Domain of the function
  //! @param batchSize Number of points to select
  //! @return Vector of points to evaluate next
  std::vector<VectorT<BayesianOptimise::RealT>> BayesianOptimise::State::selectBatch(
      const CostDomain<RealT> &domain,
      size_t batchSize)
  {
    const size_t nCandidates = 100 * batchSize;

    // Get cached candidates (generates new ones if cache is invalid)
    const auto& candidates = getCandidates(domain, nCandidates);

    // Compute cached GP predictions
    computePredictions(candidates);

    // Compute cached Expected Improvement scores
    const RealT bestY = getBestY();
    computeExpectedImprovement(bestY);

    // Select the top candidates from cached, sorted EI scores
    std::vector<VectorT<RealT>> batch;
    batch.reserve(batchSize);
    for(size_t i = 0; i < batchSize && i < mEICache.size(); ++i) {
      batch.push_back(candidates[mEICache[i].second]);
    }

    return batch;
  }

  //! Evaluate function on a batch of points, possibly in parallel
  //!
  //! If maxThreads is 0 or batch size is 1, evaluates sequentially.
  //! Otherwise, distributes evaluations across multiple threads.
  //!
  //! @param batch Vector of points to evaluate
  //! @param func Function to evaluate
  //! @return Vector of function values
  std::vector<BayesianOptimise::RealT> BayesianOptimise::evaluateBatch(
      const std::vector<VectorT<RealT>> &batch,
      const std::function<RealT(const VectorT<RealT> &)> &func) const
  {
    if(batch.empty()) {
      return {};
    }

    std::vector<RealT> results(batch.size());

    if (mMaxThreads == 0 || batch.size() == 1) {
      // Single-threaded evaluation with exception handling
      try {
        for (size_t i = 0; i < batch.size(); ++i) {
          results[i] = func(batch[i]);
        }
      } catch(const std::exception& e) {
        throw std::runtime_error("Function evaluation failed: " + std::string(e.what()));
      }
    } else {
      // Multithreaded evaluation with thread limit
      const size_t numThreads = std::min(mMaxThreads, batch.size());
      const size_t itemsPerThread = batch.size() / numThreads;
      const size_t remainder = batch.size() % numThreads;

      std::vector<std::future<void>> futures;
      std::atomic<bool> error_occurred{false};
      std::string error_message;
      std::mutex error_mutex;

      futures.reserve(numThreads);

      size_t startIdx = 0;
      for (size_t t = 0; t < numThreads; ++t) {
        const size_t endIdx = startIdx + itemsPerThread + (t < remainder ? 1 : 0);

        futures.push_back(std::async(std::launch::async, [&, startIdx, endIdx]() {
          try {
            for (size_t i = startIdx; i < endIdx && !error_occurred.load(); ++i) {
              results[i] = func(batch[i]);
            }
          } catch(const std::exception& e) {
            error_occurred.store(true);
            std::lock_guard<std::mutex> lock(error_mutex);
            if(error_message.empty()) {
              error_message = "Function evaluation failed: " + std::string(e.what());
            }
          }
        }));

        startIdx = endIdx;
      }

      // Wait for all threads to complete
      for (auto& future : futures) {
        future.get();
      }

      // Check if any errors occurred
      if(error_occurred.load()) {
        throw std::runtime_error(error_message);
      }
    }

    return results;
  }

  //! Select the next batch of points with improved exploration around the best minima
  //!
  //! This method implements several improvements over the basic EI approach:
  //! 1. Mixed acquisition function combining EI with Upper Confidence Bound (UCB)
  //! 2. Local search around the best points found so far
  //! 3. Diverse candidate generation to avoid clustering
  //!
  //! @param domain Domain of the function
  //! @param batchSize Number of points to select
  //! @param explorationWeight Weight for exploration vs exploitation
  //! @param localSearchRadius Radius for local search around best points
  //! @param numLocalCandidates Number of candidates to generate around best points
  //! @return Vector of points to evaluate next
  std::vector<VectorT<BayesianOptimise::RealT>> BayesianOptimise::State::selectBatchImproved(
      const CostDomain<RealT> &domain,
      size_t batchSize,
      RealT explorationWeight,
      RealT localSearchRadius,
      size_t numLocalCandidates)
  {
    if(mX.empty() || mY.empty()) {
      // Fallback to random sampling if no data yet
      return selectBatch(domain, batchSize);
    }

    // Determine the number of local candidates (auto if 0)
    const size_t actualLocalCandidates = (numLocalCandidates == 0) ?
                                        std::max(static_cast<size_t>(2), batchSize * 2) :
                                        numLocalCandidates;

    // Generate diverse candidate set
    const size_t baseRandomCandidates = 50 * batchSize;
    const size_t totalCandidates = baseRandomCandidates + actualLocalCandidates;

    std::vector<VectorT<RealT>> allCandidates;
    allCandidates.reserve(totalCandidates);

    // 1. Generate random candidates across the domain
    const auto nDims = static_cast<Eigen::Index>(domain.dim());
    for(size_t i = 0; i < baseRandomCandidates; ++i) {
      VectorT<RealT> x(nDims);
      for(Eigen::Index d = 0; d < nDims; ++d) {
        std::uniform_real_distribution<RealT> dist(domain.min(d), domain.max(d));
        x[d] = dist(mGen);
      }
      allCandidates.push_back(x);
    }

    // 2. Generate local candidates around the best points
    // Find the top few best points to search around
    std::vector<std::pair<RealT, size_t>> sortedPoints;
    for(size_t i = 0; i < mY.size(); ++i) {
      sortedPoints.emplace_back(mY[i], i);
    }
    std::sort(sortedPoints.begin(), sortedPoints.end());

    // Generate candidates around the top 3 best points (or fewer if not available)
    const size_t numBestPoints = std::min(static_cast<size_t>(3), sortedPoints.size());
    const size_t candidatesPerBestPoint = actualLocalCandidates / std::max(numBestPoints, static_cast<size_t>(1));

    for(size_t bestIdx = 0; bestIdx < numBestPoints; ++bestIdx) {
      const auto& bestPoint = mX[sortedPoints[bestIdx].second];

      // Calculate domain-relative radius for each dimension
      std::vector<RealT> radiusPerDim(static_cast<std::size_t>(nDims));
      for(Eigen::Index d = 0; d < nDims; ++d) {
        radiusPerDim[static_cast<std::size_t>(d)] = localSearchRadius * (domain.max(d) - domain.min(d));
      }

      // Generate candidates around this best point
      const size_t candidatesForThisPoint = (bestIdx == numBestPoints - 1) ?
                                           (actualLocalCandidates - bestIdx * candidatesPerBestPoint) :
                                           candidatesPerBestPoint;

      for(size_t i = 0; i < candidatesForThisPoint; ++i) {
        VectorT<RealT> candidate(nDims);
        for(Eigen::Index d = 0; d < nDims; ++d) {
          std::normal_distribution<RealT> dist(bestPoint[d], radiusPerDim[static_cast<std::size_t>(d)]);
          candidate[d] = std::clamp(dist(mGen), domain.min(d), domain.max(d));
        }
        allCandidates.push_back(candidate);
      }
    }

    // 3. Compute GP predictions for all candidates
    typename GaussianProcess<RealT>::Matrix Xtest(static_cast<Eigen::Index>(allCandidates.size()), nDims);
    for(size_t i = 0; i < allCandidates.size(); ++i) {
      Xtest.row(static_cast<Eigen::Index>(i)) = allCandidates[i].transpose();
    }

    VectorT<RealT> means, variances;
    mGP.predict(Xtest, means, variances);

    // 4. Compute hybrid acquisition function (EI + UCB)
    const RealT bestY = getBestY();
    const RealT beta = static_cast<RealT>(2.0 * std::log(allCandidates.size())); // UCB exploration parameter

    std::vector<std::pair<RealT, size_t>> acquisitionScores;
    acquisitionScores.reserve(allCandidates.size());

    for(size_t i = 0; i < allCandidates.size(); ++i) {
      const auto mu = means[static_cast<Eigen::Index>(i)];
      const auto sigma = std::sqrt(std::max(variances[static_cast<Eigen::Index>(i)], static_cast<RealT>(1e-9)));

      // Expected Improvement
      const auto z = static_cast<double>((bestY - mu) / sigma);
      const auto ei = sigma > 0 ?
                  static_cast<double>(bestY - mu) * std::erfc(-z/std::sqrt(2))/2 +
                  static_cast<double>(sigma) * std::exp(-0.5*z*z)/std::sqrt(2*M_PI) : 0;

      // Upper Confidence Bound (for exploration)
      const auto ucb = static_cast<double>(-mu + std::sqrt(beta) * sigma); // Negative for minimization

      // Hybrid acquisition function
      const auto hybridScore = static_cast<RealT>((1.0 - static_cast<double>(explorationWeight)) * ei + static_cast<double>(explorationWeight) * ucb);

      acquisitionScores.emplace_back(hybridScore, i);
    }

    // 5. Sort by acquisition score (descending)
    std::sort(acquisitionScores.rbegin(), acquisitionScores.rend());

    // 6. Select diverse batch to avoid clustering
    std::vector<VectorT<RealT>> batch;
    batch.reserve(batchSize);

    // Always include the top candidate
    if(!acquisitionScores.empty()) {
      batch.push_back(allCandidates[acquisitionScores[0].second]);
    }

    // For remaining candidates, enforce a minimum distance to avoid clustering
    const RealT minDistance = static_cast<RealT>(0.02); // 2% of the average domain range
    RealT avgDomainRange = static_cast<RealT>(0.0);
    for(Eigen::Index d = 0; d < nDims; ++d) {
      avgDomainRange += (domain.max(d) - domain.min(d));
    }
    avgDomainRange /= static_cast<RealT>(nDims);
    const RealT minDistanceThreshold = minDistance * avgDomainRange;

    for(size_t scoreIdx = 1; scoreIdx < acquisitionScores.size() && batch.size() < batchSize; ++scoreIdx) {
      const auto& candidate = allCandidates[acquisitionScores[scoreIdx].second];

      // Check distance to all previously selected candidates
      bool tooClose = false;
      for(const auto& selected : batch) {
        RealT distance = static_cast<RealT>(0.0);
        for(Eigen::Index d = 0; d < nDims; ++d) {
          const RealT diff = candidate[d] - selected[d];
          distance += diff * diff;
        }
        distance = std::sqrt(distance);

        if(distance < minDistanceThreshold) {
          tooClose = true;
          break;
        }
      }

      if(!tooClose) {
        batch.push_back(candidate);
      }
    }

    // If we couldn't find enough diverse candidates, fill with the best remaining ones
    for(size_t scoreIdx = 1; scoreIdx < acquisitionScores.size() && batch.size() < batchSize; ++scoreIdx) {
      const auto& candidate = allCandidates[acquisitionScores[scoreIdx].second];

      // Check if this candidate is already in the batch
      bool alreadySelected = false;
      for(const auto& selected : batch) {
        bool same = true;
        for(Eigen::Index d = 0; d < nDims; ++d) {
          if(std::abs(candidate[d] - selected[d]) > std::numeric_limits<RealT>::epsilon() * 10) {
            same = false;
            break;
          }
        }
        if(same) {
          alreadySelected = true;
          break;
        }
      }

      if(!alreadySelected) {
        batch.push_back(candidate);
      }
    }

    return batch;
  }

  //! Adapt GP length scale based on optimization progress
  //!
  //! This method adjusts the GP length scale dynamically during optimization:
  //! 1. Analyzes the spread of data points to estimate the appropriate scale
  //! 2. Considers the convergence behaviour to balance exploration/exploitation
  //! 3. Adapts the length scale to improve GP model fit
  //!
  //! @param domain Domain of the function
  //! @param adaptiveLengthScale Whether to enable adaptive scaling
  void BayesianOptimise::State::adaptLengthScale(const CostDomain<RealT> &domain, bool adaptiveLengthScale) {
    if(!adaptiveLengthScale || mX.size() < 5) {
      return; // Need at least 5 points for meaningful adaptation
    }

    const auto nDims = domain.dim();
    const size_t nSamples = mX.size();

    // 1. Compute characteristic length scales based on data distribution
    std::vector<RealT> dataLengthScales(domain.size());

    for(Eigen::Index d = 0; d < nDims; ++d) {
      // Extract dimension values
      std::vector<RealT> dimValues;
      dimValues.reserve(nSamples);
      for(size_t i = 0; i < nSamples; ++i) {
        dimValues.push_back(mX[i][d]);
      }

      // Compute interquartile range as a robust measure of spread
      std::sort(dimValues.begin(), dimValues.end());
      const size_t q1Idx = nSamples / 4;
      const size_t q3Idx = 3 * nSamples / 4;
      const RealT iqr = dimValues[q3Idx] - dimValues[q1Idx];

      // Use IQR to estimate appropriate length scale for this dimension
      // Scale it relative to the domain range
      const RealT domainRange = domain.max(d) - domain.min(d);
      const RealT relativeIQR = iqr / domainRange;

      // Adaptive length scale: smaller when data is clustered, larger when spread out
      // Target length scale should be proportional to data spread but bounded
      dataLengthScales[static_cast<std::size_t>(d)] = std::clamp(relativeIQR * 2.0f, 0.01f * domainRange, 0.5f * domainRange);
    }

    // 2. Compute average length scale across dimensions
    RealT avgDataLengthScale = 0.0f;
    for(std::size_t d = 0; d < static_cast<std::size_t>(nDims); ++d) {
      avgDataLengthScale += dataLengthScales[d];
    }
    avgDataLengthScale /= static_cast<RealT>(nDims);

    // 3. Analyze convergence behavior to adjust exploration/exploitation
    RealT convergenceFactor = 1.0f;

    if(mY.size() >= 10) {
      // Look at recent improvement to gauge convergence
      std::vector<RealT> recentValues(mY.end() - 5, mY.end());
      std::sort(recentValues.begin(), recentValues.end());

      std::vector<RealT> olderValues;
      if(mY.size() >= 15) {
        olderValues.assign(mY.end() - 15, mY.end() - 10);
        std::sort(olderValues.begin(), olderValues.end());

        // Compare recent best with older best
        const RealT recentBest = recentValues[0];
        const RealT olderBest = olderValues[0];
        const RealT improvement = olderBest - recentBest;

        // If improving slowly, reduce length scale to focus search (exploitation)
        // If improving rapidly, maintain or increase length scale (exploration)
        const RealT avgFunctionValue = std::accumulate(mY.begin(), mY.end(), 0.0f) / static_cast<RealT>(mY.size());
        const RealT relativeImprovement = improvement / std::max(std::abs(avgFunctionValue), static_cast<RealT>(1e-6));

        if(relativeImprovement < 0.01f) {
          // Slow improvement -> focus search (reduce length scale)
          convergenceFactor = 0.7f;
        } else if(relativeImprovement > 0.1f) {
          // Fast improvement -> maintain exploration (increase length scale)
          convergenceFactor = 1.3f;
        }
      }
    }

    // 4. Compute new length scale
    RealT newLengthScale = avgDataLengthScale * convergenceFactor;

    // 5. Smooth the transition to avoid abrupt changes
    const RealT currentLengthScale = mGP.getLengthScale();
    const RealT smoothingFactor = 0.7f; // How much to keep of old vs new
    newLengthScale = smoothingFactor * currentLengthScale + (1.0f - smoothingFactor) * newLengthScale;

    // 6. Apply bounds to prevent extreme values
    RealT avgDomainRange = 0.0f;
    for(Eigen::Index d = 0; d < nDims; ++d) {
      avgDomainRange += (domain.max(d) - domain.min(d));
    }
    avgDomainRange /= static_cast<RealT>(nDims);

    newLengthScale = std::clamp(newLengthScale,
                               0.005f * avgDomainRange,  // Minimum: 0.5% of domain range
                               2.0f * avgDomainRange);   // Maximum: 200% of domain range

    // 7. Update the GP if the change is significant
    if(std::abs(newLengthScale - currentLengthScale) > 0.01f * avgDomainRange) {
      if(mVerbose) {
        SPDLOG_INFO("Adapting GP length scale from {:.4f} to {:.4f} (convergence factor: {:.2f})",
                   currentLengthScale, newLengthScale, convergenceFactor);
      }
      mGP.setLengthScale(newLengthScale);

      // Invalidate caches since GP parameters changed
      mPredictionsCacheValid = false;
      mEICacheValid = false;
      mCandidatesCacheValid = false;
    }
  }
}// namespace Ravl2
