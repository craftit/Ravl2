#include "Ravl2/Optimise/BayesianOptimise.hh"
#include <random>
#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace Ravl2
{

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
      mMinItersBeforeConvergence(config.getUnsigned("minItersBeforeConvergence", "Minimum iterations before checking convergence.", 5, 1, 1000))
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
      throw std::invalid_argument("Domain must have at least one dimension");
    }
    if(!func) {
      throw std::invalid_argument("Function cannot be null");
    }

    // Validate starting points have correct dimension
    for(const auto& point : startPoints) {
      if(point.size() != static_cast<Eigen::Index>(domain.dim())) {
        throw std::invalid_argument("Starting point dimension doesn't match domain dimension");
      }
    }

    State state(mFixedSeed, mSeed, mGPLengthScale, mGPNoise);
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

    // Track best values for convergence checking
    std::vector<RealT> recentBestValues;
    recentBestValues.reserve(mMinItersBeforeConvergence + 1);

    for(size_t iter = 0; iter < mMaxIters; ++iter) {
      state.fitSurrogate();
      auto batch = state.selectBatch(domain, mBatchSize);

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
    if(mX.size() != mY.size()) {
      throw std::runtime_error("Mismatch between number of X points and Y values");
    }

    // Convert vector of vectors to matrix efficiently
    const auto nSamples = static_cast<Eigen::Index>(mX.size());
    const auto nFeatures = mX[0].size();

    // Validate all vectors have the same dimension
    for(size_t i = 1; i < mX.size(); ++i) {
      if(mX[i].size() != nFeatures) {
        throw std::runtime_error("Inconsistent input vector dimensions");
      }
    }

    typename GaussianProcess<RealT>::Matrix Xmat(nSamples, nFeatures);
    VectorT<RealT> yvec(nSamples);

    // Use Eigen's Map for efficient data transfer
    for(Eigen::Index i = 0; i < nSamples; ++i) {
      Xmat.row(i) = mX[static_cast<size_t>(i)].transpose();
      yvec[i] = mY[static_cast<size_t>(i)];
    }

    mGP.fit(Xmat, yvec);
  }

  //! Predict function value at a point using surrogate model
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

  //! Select next batch of points to evaluate
  //!
  //! Uses Expected Improvement acquisition function to select promising points.
  //!
  //! @param domain Domain of the function
  //! @param batchSize Number of points to select
  //! @return Vector of points to evaluate next
  std::vector<VectorT<BayesianOptimise::RealT>> BayesianOptimise::State::selectBatch(
      const CostDomain<RealT> &domain,
      size_t batchSize)
  {
    const size_t nCandidates = 100 * batchSize;
    std::vector<VectorT<RealT>> candidates;
    candidates.reserve(nCandidates);

    // Generate random candidates
    const auto nDims = static_cast<Eigen::Index>(domain.dim());
    for(size_t i = 0; i < nCandidates; ++i) {
      VectorT<RealT> x(nDims);
      for(Eigen::Index d = 0; d < nDims; ++d) {
        std::uniform_real_distribution<RealT> dist(domain.min(d), domain.max(d));
        x[d] = dist(mGen);
      }
      candidates.push_back(x);
    }

    // Convert candidates to a matrix format
    typename GaussianProcess<RealT>::Matrix Xtest(static_cast<Eigen::Index>(nCandidates), nDims);
    for(size_t i = 0; i < nCandidates; ++i) {
      Xtest.row(static_cast<Eigen::Index>(i)) = candidates[i].transpose();
    }

    // Predict with the surrogate model
    VectorT<RealT> mean, var;
    mGP.predict(Xtest, mean, var);
    const RealT bestY = getBestY();

    // Calculate expected improvement for each candidate
    std::vector<std::pair<RealT, size_t>> ei_scores;
    ei_scores.reserve(nCandidates);

    for(size_t i = 0; i < nCandidates; ++i) {
      const auto mu = mean[static_cast<Eigen::Index>(i)];
      const auto sigma = std::sqrt(std::max(var[static_cast<Eigen::Index>(i)], RealT(1e-9)));
      const auto z = static_cast<double>((bestY - mu) / sigma);
      const auto ei = sigma > 0 ?
                  static_cast<double>(bestY - mu) * std::erfc(-z/std::sqrt(2))/2 +
                  static_cast<double>(sigma) * std::exp(-0.5*z*z)/std::sqrt(2*M_PI) : 0;
      ei_scores.emplace_back(static_cast<RealT>(ei), i);
    }

    // Sort by EI (descending)
    std::sort(ei_scores.rbegin(), ei_scores.rend());

    // Select the top candidates
    std::vector<VectorT<RealT>> batch;
    batch.reserve(batchSize);
    for(size_t i = 0; i < batchSize && i < ei_scores.size(); ++i) {
      batch.push_back(candidates[ei_scores[i].second]);
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

}// namespace Ravl2
