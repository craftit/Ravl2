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
        mBatchSize(config.getUnsigned("batchSize", "Batch size.", 1, 1, 100)),
        mMaxIters(config.getUnsigned("maxIters", "Maximum number of iterations", 20, 1, 1000)),
        mMaxThreads(config.getUnsigned("maxThreads", "Maximum number of threads for parallel evaluation (0 = single-threaded)", 0, 0, 100)),
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
    const VectorT<RealT> &start) const
  {
    std::vector<VectorT<RealT>> startPoints;
    if(!start.isZero(0)) {
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
    const std::vector<VectorT<RealT>> &startPoints) const
  {

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
    for(const auto &point : startPoints) {
      if(point.size() != static_cast<Eigen::Index>(domain.dim())) {
        SPDLOG_ERROR("BayesianOptimise::minimiseBatch() starting point dimension mismatch: point.size()={}, domain.dim()={}", point.size(), domain.dim());
        throw std::invalid_argument("Starting point dimension doesn't match domain dimension");
      }
    }

    // Ensure we have default generators
    auto initialGen = mInitialGenerator ? mInitialGenerator : std::make_shared<SobolSampleGenerator>();
    auto acquisitionGen = mAcquisitionGenerator ? mAcquisitionGenerator : std::make_shared<AcquisitionSampleGenerator>(mExplorationWeight, mLocalSearchRadius, 0, 100, mFixedSeed, mSeed);

    State state(mGPLengthScale, mGPNoise, initialGen, acquisitionGen, verbose());
    std::vector<std::tuple<VectorT<RealT>, RealT>> results;
    std::vector<VectorT<RealT>> initPoints = startPoints;

    // Generate initial points if none provided
    if(initPoints.empty()) {
      initPoints = state.generateInitialPoints(domain, mBatchSize);
    }

    // Evaluate initial points using batch evaluator with fill
    auto initResult = evaluateBatchWithFill(initPoints, func, state, domain);
    auto initResults = initResult.first;
    auto actualInitPoints = initResult.second;

    for(size_t i = 0; i < actualInitPoints.size(); ++i) {
      state.addPoint(actualInitPoints[i], initResults[i]);
      results.emplace_back(actualInitPoints[i], initResults[i]);
    }

    // Track the best values for convergence checking
    std::vector<RealT> recentBestValues;
    recentBestValues.reserve(mMinItersBeforeConvergence + 1);

    // Clear cache statistics at start
    state.mCacheHits = 0;

    for(size_t iter = 0; iter < mMaxIters; ++iter) {
      state.fitSurrogate();

      // Adapt GP length scale based on optimization progress
      if(mAdaptiveLengthScale && iter > 0) {
        state.adaptLengthScale(domain, iter, mMaxIters);
      }

      auto batch = state.selectBatchWithFill(domain, mBatchSize);

      // Early termination if no candidates found
      if(batch.empty()) {
        if(verbose()) {
          SPDLOG_WARN("No candidates found at iteration {}, terminating early", iter);
        }
        break;
      }

      // Evaluate batch using parallel evaluator with fill
      auto batchResult = evaluateBatchWithFill(batch, func, state, domain);
      auto batchResults = batchResult.first;
      auto actualBatch = batchResult.second;

      for(size_t i = 0; i < actualBatch.size(); ++i) {
        state.addPoint(actualBatch[i], batchResults[i]);
        results.emplace_back(actualBatch[i], batchResults[i]);
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
        auto cacheStats = state.getCacheStats();
        SPDLOG_INFO("Iteration {}: best value = {}, cache hits: {}/{}, evaluated: {}/{}",
                   iter, currentBest, cacheStats.first, cacheStats.second,
                   actualBatch.size(), batch.size());
      }
    }

    return results;
  }

  BayesianOptimise::RealT BayesianOptimise::State::evaluateWithCache(
    const VectorT<RealT> &point,
    const std::function<RealT(const VectorT<RealT> &)> &func)
  {
    // Check cache first
    auto cachedValue = mFunctionCache.get(point);
    if (cachedValue.has_value()) {
      ++mCacheHits;
      return *cachedValue;
    }

    // Evaluate and cache
    RealT value = func(point);
    mFunctionCache.add(point, value);
    return value;
  }

  std::pair<std::vector<BayesianOptimise::RealT>, std::vector<VectorT<BayesianOptimise::RealT>>>
  BayesianOptimise::evaluateBatchWithFill(
    const std::vector<VectorT<RealT>> &batch,
    const std::function<RealT(const VectorT<RealT> &)> &func,
    State &state,
    const CostDomain<RealT> &domain) const
  {
    if(batch.empty()) {
      return {std::vector<RealT>(), std::vector<VectorT<RealT>>()};
    }

    // First pass: separate cached and uncached points
    std::vector<RealT> allResults;
    std::vector<VectorT<RealT>> actualPoints;
    std::vector<VectorT<RealT>> uncachedPoints;
    std::vector<size_t> uncachedIndices;

    allResults.reserve(batch.size());
    actualPoints.reserve(batch.size());

    for(size_t i = 0; i < batch.size(); ++i) {
      auto cachedValue = state.mFunctionCache.get(batch[i]);
      if(cachedValue.has_value()) {
        // Cache hit
        ++state.mCacheHits;
        allResults.push_back(*cachedValue);
        actualPoints.push_back(batch[i]);
      } else {
        // Cache miss - need to evaluate
        uncachedPoints.push_back(batch[i]);
        uncachedIndices.push_back(i);
      }
    }

    // If we have cache hits, we need to fill the batch with additional new points
    size_t targetEvaluations = batch.size();
    size_t cacheHits = batch.size() - uncachedPoints.size();

    if(cacheHits > 0 && uncachedPoints.size() < targetEvaluations) {
      // Generate additional points to maintain batch size
      size_t additionalNeeded = targetEvaluations - uncachedPoints.size();
      auto additionalBatch = state.selectBatch(domain, additionalNeeded);

      // Filter out any points that are already cached or in our current batch
      for(const auto& newPoint : additionalBatch) {
        if(uncachedPoints.size() >= targetEvaluations) break;

        // Check if this point is already cached
        if(!state.mFunctionCache.contains(newPoint)) {
          // Check if it's not already in our uncached list
          bool alreadyInBatch = false;
          for(const auto& existing : uncachedPoints) {
            if((newPoint - existing).norm() < static_cast<RealT>(1e-12)) {
              alreadyInBatch = true;
              break;
            }
          }
          if(!alreadyInBatch) {
            uncachedPoints.push_back(newPoint);
          }
        }
      }
    }

    // Evaluate uncached points in parallel
    if(!uncachedPoints.empty()) {
      std::vector<RealT> uncachedResults(uncachedPoints.size());

      if(mMaxThreads == 0 || uncachedPoints.size() == 1) {
        // Sequential evaluation
        for(size_t i = 0; i < uncachedPoints.size(); ++i) {
          RealT value = func(uncachedPoints[i]);
          state.mFunctionCache.add(uncachedPoints[i], value);
          uncachedResults[i] = value;
        }
      } else {
        // Parallel evaluation without broad mutex lock
        const size_t numThreads = std::min(mMaxThreads, uncachedPoints.size());
        std::vector<std::future<void>> futures;
        futures.reserve(numThreads);

        std::atomic<size_t> nextIndex{0};

        for(size_t t = 0; t < numThreads; ++t) {
          futures.emplace_back(std::async(std::launch::async, [&]() {
            while(true) {
              size_t index = nextIndex.fetch_add(1);
              if(index >= uncachedPoints.size()) break;

              // Evaluate function (this is the expensive part, no locking needed)
              RealT value = func(uncachedPoints[index]);

              // Cache the result (FunctionCache handles its own locking)
              state.mFunctionCache.add(uncachedPoints[index], value);
              uncachedResults[index] = value;
            }
          }));
        }

        // Wait for all threads to complete
        for(auto &future : futures) {
          future.wait();
        }
      }

      // Add uncached results to the final results
      for(size_t i = 0; i < uncachedPoints.size(); ++i) {
        allResults.push_back(uncachedResults[i]);
        actualPoints.push_back(uncachedPoints[i]);
      }
    }

    return {allResults, actualPoints};
  }

  // Keep the old method for backward compatibility
  std::vector<BayesianOptimise::RealT> BayesianOptimise::evaluateBatch(
    const std::vector<VectorT<RealT>> &batch,
    const std::function<RealT(const VectorT<RealT> &)> &func,
    State &state) const
  {
    // This is a simplified version that doesn't do batch filling
    // Used mainly for initial points where we don't need filling
    if(batch.empty()) {
      return {};
    }

    std::vector<RealT> results(batch.size());

    if(mMaxThreads == 0 || batch.size() == 1) {
      // Sequential evaluation with caching
      for(size_t i = 0; i < batch.size(); ++i) {
        results[i] = state.evaluateWithCache(batch[i], func);
      }
    } else {
      // Parallel evaluation with caching
      const size_t numThreads = std::min(mMaxThreads, batch.size());
      std::vector<std::future<void>> futures;
      futures.reserve(numThreads);

      std::atomic<size_t> nextIndex{0};

      for(size_t t = 0; t < numThreads; ++t) {
        futures.emplace_back(std::async(std::launch::async, [&]() {
          while(true) {
            size_t index = nextIndex.fetch_add(1);
            if(index >= batch.size()) break;

            results[index] = state.evaluateWithCache(batch[index], func);
          }
        }));
      }

      // Wait for all threads to complete
      for(auto &future : futures) {
        future.wait();
      }
    }

    return results;
  }

  std::vector<VectorT<BayesianOptimise::RealT>> BayesianOptimise::State::selectBatchWithFill(
    const CostDomain<RealT> &domain, size_t batchSize)
  {
    // This wraps the existing selectBatch method
    return selectBatch(domain, batchSize);
  }

  void BayesianOptimise::State::fitSurrogate()
  {
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
    const CostDomain<RealT> &domain, size_t numPoints)
  {

    mInitialGen->reset(domain);
    return mInitialGen->generatePoints(numPoints);
  }

  std::vector<VectorT<BayesianOptimise::RealT>> BayesianOptimise::State::selectBatch(
    const CostDomain<RealT> &domain, size_t batchSize)
  {

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

  void BayesianOptimise::State::adaptLengthScale(const CostDomain<RealT> &domain, size_t iteration, size_t maxIters)
  {
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
    RealT baseLengthScale = mGP.getLengthScale();// Get current length scale
    RealT initialScale = baseLengthScale;        // Assume this is close to the original
    RealT progressFactor = 2.0f * (1.0f - progress) + 0.5f * progress;

    // Data density factor: more points = shorter length scale
    RealT dataPoints = static_cast<RealT>(mX.size());
    RealT densityFactor = 1.0f / (1.0f + dataPoints / 20.0f);        // Normalize by expected ~20 points
    densityFactor = std::max(densityFactor, static_cast<RealT>(0.2));// Don't go too small

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
      newLengthScale = std::max(newLengthScale, domainSize * 0.01f);// At least 1% of domain
      newLengthScale = std::min(newLengthScale, domainSize * 2.0f); // At most 2x domain size

      mGP.setLengthScale(newLengthScale);

      if(mVerbose) {
        SPDLOG_INFO("Adapted GP length scale: {} -> {} (progress: {:.2f}, density: {:.2f}, variation: {:.2f})",
                    baseLengthScale, newLengthScale, progress, densityFactor, variationFactor);
      }
    }
  }

  // AcquisitionSampleGenerator implementation

  AcquisitionSampleGenerator::AcquisitionSampleGenerator(
    RealT explorationWeight, RealT localSearchRadius, size_t numLocalCandidates,
    size_t candidateMultiplier, bool fixedSeed, unsigned seed)
      : mExplorationWeight(explorationWeight), mLocalSearchRadius(localSearchRadius),
        mNumLocalCandidates(numLocalCandidates), mCandidateMultiplier(candidateMultiplier),
        mFixedSeed(fixedSeed), mSeed(seed), mRandomGen(fixedSeed ? seed : std::random_device {}())
  {
  }

  AcquisitionSampleGenerator::AcquisitionSampleGenerator(Configuration &config)
      : mExplorationWeight(static_cast<RealT>(config.getNumber("explorationWeight",
                                                               "Weight for exploration vs exploitation", 0.1, 0.0, 1.0))),
        mLocalSearchRadius(static_cast<RealT>(config.getNumber("localSearchRadius",
                                                               "Radius for local search around best points", 0.1, 0.01, 0.5))),
        mNumLocalCandidates(config.getUnsigned("numLocalCandidates",
                                               "Number of candidates around best points (0=auto)", 0, 0, 1000)),
        mCandidateMultiplier(config.getUnsigned("candidateMultiplier",
                                                "Multiplier for number of candidates vs requested points", 100, 10, 1000)),
        mFixedSeed(config.getBool("fixedSeed", "Use fixed random seed", true)),
        mSeed(config.getUnsigned("seed", "Random seed value", 42, 0, std::numeric_limits<unsigned>::max())),
        mRandomGen(mFixedSeed ? mSeed : std::random_device {}())
  {
  }

  std::vector<VectorT<AcquisitionSampleGenerator::RealT>> AcquisitionSampleGenerator::generatePoints(
    size_t numPoints)
  {

    if(mDomain.dim() == 0) {
      throw std::runtime_error("Domain not set. Call reset(domain) first.");
    }

    if(!mGP || mX.empty() || mY.empty()) {
      // Fallback to Sobol sampling if no GP data available
      mSobolGen.reset(mDomain);
      return mSobolGen.generatePoints(numPoints);
    }

    // Generate candidate points
    size_t numCandidates = mCandidateMultiplier * numPoints;
    auto candidates = generateCandidates(numCandidates);

    if(candidates.empty()) {
      // Fallback to Sobol sampling
      mSobolGen.reset(mDomain);
      return mSobolGen.generatePoints(numPoints);
    }

    // Find best current value
    RealT bestY = *std::min_element(mY.begin(), mY.end());

    // Compute Expected Improvement for all candidates
    auto eiScores = computeExpectedImprovement(candidates, bestY);

    // Select top candidates
    std::vector<VectorT<RealT>> selected;
    selected.reserve(numPoints);

    size_t maxSelect = std::min(numPoints, eiScores.size());
    for(size_t i = 0; i < maxSelect; ++i) {
      selected.push_back(candidates[eiScores[i].second]);
    }

    mPosition += numPoints;
    return selected;
  }

  std::shared_ptr<SampleGenerator> AcquisitionSampleGenerator::clone() const
  {
    return std::make_shared<AcquisitionSampleGenerator>(
      mExplorationWeight, mLocalSearchRadius, mNumLocalCandidates,
      mCandidateMultiplier, mFixedSeed, mSeed);
  }

  void AcquisitionSampleGenerator::setEvaluationData(const std::vector<VectorT<RealT>> &X,
                                                     const std::vector<RealT> &Y,
                                                     const GaussianProcess<RealT> *gp)
  {
    mX = X;
    mY = Y;
    mGP = gp;
  }

  std::vector<VectorT<AcquisitionSampleGenerator::RealT>> AcquisitionSampleGenerator::generateCandidates(
    size_t numCandidates)
  {

    std::vector<VectorT<RealT>> candidates;
    candidates.reserve(numCandidates);

    // Strategy 1: Sobol sequence for global exploration (60% of candidates)
    size_t globalCandidates = static_cast<size_t>(0.6 * numCandidates);
    mSobolGen.reset(mDomain);
    auto globalPoints = mSobolGen.generatePoints(globalCandidates);
    candidates.insert(candidates.end(), globalPoints.begin(), globalPoints.end());

    // Strategy 2: Random exploration with higher variance (25% of candidates)
    size_t randomCandidates = static_cast<size_t>(0.25 * numCandidates);
    std::uniform_real_distribution<RealT> uniformDist(0.0, 1.0);
    for(size_t i = 0; i < randomCandidates; ++i) {
      VectorT<RealT> point(mDomain.dim());
      for(Eigen::Index d = 0; d < mDomain.dim(); ++d) {
        RealT u = uniformDist(mRandomGen);
        point[d] = mDomain.min(d) + u * (mDomain.max(d) - mDomain.min(d));
      }
      candidates.push_back(point);
    }

    // Strategy 3: Local search around the best points (15% of candidates)
    if(!mY.empty()) {
      size_t localCandidates = numCandidates - globalCandidates - randomCandidates;
      size_t numLocalActual = mNumLocalCandidates > 0 ? mNumLocalCandidates : localCandidates;
      numLocalActual = std::min(numLocalActual, localCandidates);

      // Find best points
      std::vector<std::pair<RealT, size_t>> sortedY;
      for(size_t i = 0; i < mY.size(); ++i) {
        sortedY.emplace_back(mY[i], i);
      }
      std::sort(sortedY.begin(), sortedY.end());

      // Generate candidates around top points
      size_t numBestPoints = std::min(static_cast<size_t>(3), mY.size());
      std::normal_distribution<RealT> normalDist(0.0, 1.0);

      for(size_t i = 0; i < numLocalActual; ++i) {
        size_t bestIdx = sortedY[i % numBestPoints].second;
        const auto &bestPoint = mX[bestIdx];

        VectorT<RealT> localPoint(mDomain.dim());
        for(Eigen::Index d = 0; d < mDomain.dim(); ++d) {
          RealT domainRange = mDomain.max(d) - mDomain.min(d);
          RealT noise = normalDist(mRandomGen) * mLocalSearchRadius * domainRange;
          localPoint[d] = std::max(mDomain.min(d),
                                   std::min(mDomain.max(d), bestPoint[d] + noise));
        }
        candidates.push_back(localPoint);
      }
    }

    return candidates;
  }

  std::vector<std::pair<AcquisitionSampleGenerator::RealT, size_t>>
  AcquisitionSampleGenerator::computeExpectedImprovement(
    const std::vector<VectorT<RealT>> &candidates, RealT bestY)
  {

    if(!mGP || candidates.empty()) {
      return {};
    }

    const size_t nCandidates = candidates.size();
    const auto nDims = candidates[0].size();

    // Convert candidates to matrix format
    typename GaussianProcess<RealT>::Matrix Xtest(static_cast<Eigen::Index>(nCandidates), nDims);
    for(size_t i = 0; i < nCandidates; ++i) {
      Xtest.row(static_cast<Eigen::Index>(i)) = candidates[i].transpose();
    }

    // Predict with GP
    VectorT<RealT> mean, variance;
    mGP->predict(Xtest, mean, variance);

    // Compute Expected Improvement
    std::vector<std::pair<RealT, size_t>> eiScores;
    eiScores.reserve(nCandidates);

    for(size_t i = 0; i < nCandidates; ++i) {
      const auto mu = mean[static_cast<Eigen::Index>(i)];
      const auto sigma = std::sqrt(std::max(variance[static_cast<Eigen::Index>(i)], static_cast<RealT>(1e-9)));

      if(sigma > 0) {
        const auto z = static_cast<double>((bestY - mu) / sigma);
        const auto ei = static_cast<double>(bestY - mu) * std::erfc(-z / std::sqrt(2)) / 2 + static_cast<double>(sigma) * std::exp(-0.5 * z * z) / std::sqrt(2 * M_PI);
        eiScores.emplace_back(static_cast<RealT>(ei), i);
      } else {
        eiScores.emplace_back(0.0, i);
      }
    }

    // Sort by EI score (descending)
    std::sort(eiScores.rbegin(), eiScores.rend());

    return eiScores;
  }

  namespace
  {
    [[maybe_unused]] bool g_register = defaultConfigFactory().registerNamedType<Optimise, BayesianOptimise>("BayesianOptimise");
  }

}// namespace Ravl2
