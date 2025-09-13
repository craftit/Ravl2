#pragma once

#include "Optimise.hh"
#include "GaussianProcess.hh"
#include "SampleGenerators.hh"
#include <vector>
#include <tuple>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>

namespace Ravl2
{
  // Forward declaration for the point hash function
  template<typename RealT>
  struct PointHasher;

  // Function evaluation cache to avoid redundant evaluations
  template<typename RealT>
  class FunctionCache
  {
  public:
    //! Constructor
    //! @param tolerance Tolerance for considering two points equal (default: 1e-10)
    explicit FunctionCache(RealT tolerance = static_cast<RealT>(1e-10))
        : mTolerance(tolerance) {}

    // Add a point and its function value to the cache
    void add(const VectorT<RealT> &point, RealT value)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mCache[point] = value;
    }

    // Check if a point is in the cache (with tolerance)
    bool contains(const VectorT<RealT> &point) const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return findNearestPoint(point).has_value();
    }

    // Get the function value for a point (with tolerance)
    std::optional<RealT> get(const VectorT<RealT> &point) const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      auto nearest = findNearestPoint(point);
      return nearest ? std::optional<RealT>(nearest->second) : std::nullopt;
    }

    // Get the number of cached evaluations
    size_t size() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mCache.size();
    }

    // Clear the cache
    void clear()
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mCache.clear();
    }

    // Set tolerance for point matching
    void setTolerance(RealT tolerance) {
      std::lock_guard<std::mutex> lock(mMutex);
      mTolerance = tolerance;
    }

    RealT getTolerance() const {
      std::lock_guard<std::mutex> lock(mMutex);
      return mTolerance;
    }

  private:
    mutable std::mutex mMutex;  //!< Mutex for thread-safe access
    std::unordered_map<VectorT<RealT>, RealT, PointHasher<RealT>> mCache;
    RealT mTolerance;

    //! Find the nearest cached point within tolerance (must be called with lock held)
    std::optional<std::pair<VectorT<RealT>, RealT>> findNearestPoint(const VectorT<RealT> &point) const
    {
      // First try exact match
      auto it = mCache.find(point);
      if (it != mCache.end()) {
        return std::make_pair(it->first, it->second);
      }

      // If tolerance is very small, don't do approximate search
      if (mTolerance <= 0) {
        return std::nullopt;
      }

      // Search for approximate match
      for (const auto &cached : mCache) {
        if (point.size() == cached.first.size()) {
          RealT distance = (point - cached.first).norm();
          if (distance <= mTolerance) {
            return std::make_pair(cached.first, cached.second);
          }
        }
      }
      return std::nullopt;
    }
  };

  // Hash function for VectorT to enable using it as a key in unordered_map
  template<typename RealT>
  struct PointHasher
  {
    std::size_t operator()(const VectorT<RealT> &point) const
    {
      std::size_t seed = 0;
      for (int i = 0; i < point.size(); ++i) {
        // Use bit representation for more reliable hashing
        // Round to reduce floating point precision issues
        constexpr RealT scale = static_cast<RealT>(1e6); // 6 decimal places precision
        auto rounded = static_cast<std::int64_t>(std::round(point[i] * scale));
        seed ^= std::hash<std::int64_t>{}(rounded) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };

  //! Acquisition function sample generator for Bayesian optimization
  //!
  //! Generates candidate points and selects the best ones based on an acquisition function
  //! (Expected Improvement). Uses multiple strategies including random sampling around
  //! best points and quasi-random exploration.
  class AcquisitionSampleGenerator : public SampleGenerator
  {
  public:
    //! Constructor
    //! @param explorationWeight Weight for exploration vs exploitation (default: 0.1)
    //! @param localSearchRadius Radius for local search around best points (default: 0.1)
    //! @param numLocalCandidates Number of candidates around best points (default: 0 = auto)
    //! @param candidateMultiplier Multiplier for number of candidates vs requested points (default: 100)
    //! @param fixedSeed Whether to use fixed random seed (default: true)
    //! @param seed Random seed value (default: 42)
    explicit AcquisitionSampleGenerator(
      RealT explorationWeight = static_cast<RealT>(0.1),
      RealT localSearchRadius = static_cast<RealT>(0.1),
      size_t numLocalCandidates = 0,
      size_t candidateMultiplier = 100,
      bool fixedSeed = true,
      unsigned seed = 42);

    //! Construct from configuration
    //! @param config Configuration object containing generator parameters
    explicit AcquisitionSampleGenerator(Configuration &config);

    //! Generate points using acquisition function
    //! @param numPoints Number of points to generate
    //! @return Vector of points selected by acquisition function
    std::vector<VectorT<RealT>> generatePoints(size_t numPoints) override;

    //! Clone this generator
    std::shared_ptr<SampleGenerator> clone() const override;

    //! Set evaluation data for acquisition function
    //! @param X Evaluated points
    //! @param Y Function values
    //! @param gp Gaussian Process model
    void setEvaluationData(const std::vector<VectorT<RealT>> &X,
                           const std::vector<RealT> &Y,
                           const GaussianProcess<RealT> *gp);

    //! Set parameters
    void setExplorationWeight(RealT weight) { mExplorationWeight = weight; }
    void setLocalSearchRadius(RealT radius) { mLocalSearchRadius = radius; }
    void setNumLocalCandidates(size_t num) { mNumLocalCandidates = num; }
    void setCandidateMultiplier(size_t mult) { mCandidateMultiplier = mult; }

    void reset(const CostDomain<RealT> &domain) override
    {
      mDomain = domain;
      reset();
    }
    void reset() override
    {
      mRandomGen.seed(mFixedSeed ? mSeed : std::random_device {}());
      mSobolGen.reset(mDomain);
      mPosition = 0;
    }
    void skip(size_t n) override { mPosition += n; }
    size_t getCurrentPosition() const override { return mPosition; }

  private:
    RealT mExplorationWeight;
    RealT mLocalSearchRadius;
    size_t mNumLocalCandidates;
    size_t mCandidateMultiplier;
    bool mFixedSeed;
    unsigned mSeed;
    size_t mPosition = 0;

    CostDomain<RealT> mDomain;
    std::mt19937 mRandomGen;
    SobolSampleGenerator mSobolGen;

    // Evaluation data for acquisition function
    std::vector<VectorT<RealT>> mX;
    std::vector<RealT> mY;
    const GaussianProcess<RealT> *mGP = nullptr;

    //! Generate candidate points using multiple strategies
    std::vector<VectorT<RealT>> generateCandidates(size_t numCandidates);

    //! Compute Expected Improvement for candidates
    std::vector<std::pair<RealT, size_t>> computeExpectedImprovement(
      const std::vector<VectorT<RealT>> &candidates, RealT bestY);
  };

  //! Bayesian optimization using a Gaussian Process surrogate model.
  //!
  //! Implements Bayesian optimization to find global minima of expensive functions
  //! by building a probabilistic surrogate model of the objective function.
  //! Uses SampleGenerator framework for flexible point generation strategies.

  class BayesianOptimise : public Optimise
  {
  public:
    /// Real number type used for calculations
    using RealT = Optimise::RealT;

    /// Default constructor
    BayesianOptimise() = default;

    //! Constructor with verbosity setting
    //! @param verbose Whether to output progress information
    explicit BayesianOptimise(bool verbose)
        : Optimise(verbose) {}

    //! Programmatic constructor with common parameters.
    //! @param maxIters Maximum number of optimization iterations
    //! @param batchSize Number of points to evaluate in parallel (default: 1)
    //! @param maxThreads Maximum number of threads for parallel evaluation (0 = single-threaded, default: 0)
    //! @param verbose Whether to output progress information (default: false)
    //! @param gpLengthScale Length scale parameter for GP kernel (default: 1.0)
    //! @param gpNoise Noise parameter for GP (default: 1e-6)
    //! @param fixedSeed Whether to use fixed random seed for reproducibility (default: true)
    //! @param seed Random seed value if fixedSeed is true (default: 42)
    //! @param tolerance Convergence tolerance for early stopping (default: 1e-6)
    //! @param minItersBeforeConvergence Minimum iterations before checking convergence (default: 5)
    //! @param explorationWeight Weight for exploration vs exploitation (default: 0.1)
    //! @param localSearchRadius Radius for local search around best points (default: 0.1)
    //! @param adaptiveLengthScale Whether to use adaptive length scale (default: false)
    explicit BayesianOptimise(
      size_t maxIters,
      size_t batchSize = 1,
      size_t maxThreads = 0,
      bool verbose = false,
      RealT gpLengthScale = static_cast<RealT>(1.0),
      RealT gpNoise = static_cast<RealT>(1e-6),
      bool fixedSeed = true,
      unsigned seed = 42,
      RealT tolerance = static_cast<RealT>(1e-6),
      size_t minItersBeforeConvergence = 5,
      RealT explorationWeight = static_cast<RealT>(0.1),
      RealT localSearchRadius = static_cast<RealT>(0.1),
      bool adaptiveLengthScale = false);

    //! Constructor from configuration
    //! @param config Configuration object containing optimization parameters
    explicit BayesianOptimise(Configuration &config);

    //! Find minimum of a function
    //! @param domain Domain of the function to minimize
    //! @param func Function to minimize
    //! @param start Optional starting point
    //! @return Tuple containing the minimum point and its value
    [[nodiscard]] std::tuple<VectorT<RealT>, RealT> minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start = VectorT<RealT>()) const override;

    //! Find multiple minima of a function in batch mode
    //! @param domain Domain of the function to minimize
    //! @param func Function to minimize
    //! @param startPoints Optional starting points
    //! @return Vector of tuples containing the minima points and their values
    std::vector<std::tuple<VectorT<RealT>, RealT>> minimiseBatch(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const std::vector<VectorT<RealT>> &startPoints) const;

    //! Setters for configuration parameters
    void setBatchSize(size_t batchSize) { mBatchSize = batchSize; }
    void setMaxIters(size_t maxIters) { mMaxIters = maxIters; }
    void setMaxThreads(size_t maxThreads) { mMaxThreads = maxThreads; }
    void setFixedSeed(bool fixedSeed) { mFixedSeed = fixedSeed; }
    void setSeed(unsigned seed) { mSeed = seed; }
    void setGPLengthScale(RealT lengthScale) { mGPLengthScale = lengthScale; }
    void setGPNoise(RealT noise) { mGPNoise = noise; }
    void setTolerance(RealT tolerance) { mTolerance = tolerance; }
    void setMinItersBeforeConvergence(size_t minIters) { mMinItersBeforeConvergence = minIters; }
    void setExplorationWeight(RealT weight) { mExplorationWeight = weight; }
    void setLocalSearchRadius(RealT radius) { mLocalSearchRadius = radius; }
    void setAdaptiveLengthScale(bool adaptive) { mAdaptiveLengthScale = adaptive; }

    //! Set a custom sample generator for initial points
    //! @param generator Sample generator to use for initial sampling
    void setInitialSampleGenerator(std::shared_ptr<SampleGenerator> generator)
    {
      mInitialGenerator = generator;
    }

    //! Set a custom sample generator for acquisition-based sampling
    //! @param generator Sample generator to use for acquisition-based sampling
    void setAcquisitionSampleGenerator(std::shared_ptr<SampleGenerator> generator)
    {
      mAcquisitionGenerator = generator;
    }

  private:
    size_t mBatchSize = 1;                //!< Size of the batch for parallel evaluations
    size_t mMaxIters = 20;                //!< Maximum number of optimization iterations
    size_t mMaxThreads = 0;               //!< Maximum number of threads for parallel evaluation (0 = single-threaded)
    bool mFixedSeed = true;               //!< Whether to use a fixed random seed for reproducibility
    unsigned mSeed = 42;                  //!< Random seed value if fixed seed is enabled
    RealT mGPLengthScale = RealT(1.0);    //!< Length scale parameter for Gaussian Process kernel
    RealT mGPNoise = RealT(1e-6);         //!< Noise parameter for Gaussian Process
    RealT mTolerance = RealT(1e-6);       //!< Convergence tolerance for early stopping
    size_t mMinItersBeforeConvergence = 5;//!< Minimum iterations before checking convergence
    RealT mExplorationWeight = RealT(0.1);//!< Weight for exploration vs exploitation
    RealT mLocalSearchRadius = RealT(0.1);//!< Radius for local search around best points
    bool mAdaptiveLengthScale = false;    //!< Whether to use adaptive length scale

    std::shared_ptr<SampleGenerator> mInitialGenerator;    //!< Generator for initial points
    std::shared_ptr<SampleGenerator> mAcquisitionGenerator;//!< Generator for acquisition-based points


    //! Internal state class for Bayesian optimization process
    //!
    //! Maintains the current state of the optimization including
    //! evaluated points, function values, surrogate model, sample generators, and function cache.
    class State
    {
    public:
      std::vector<VectorT<RealT>> mX;                  //!< Points evaluated so far
      std::vector<RealT> mY;                           //!< Function values for evaluated points
      GaussianProcess<RealT> mGP;                      //!< Gaussian Process surrogate model
      std::shared_ptr<SampleGenerator> mInitialGen;    //!< Generator for initial points
      std::shared_ptr<SampleGenerator> mAcquisitionGen;//!< Generator for acquisition-based points
      FunctionCache<RealT> mFunctionCache;             //!< Function evaluation cache
      std::atomic<size_t> mCacheHits{0};               //!< Number of cache hits (thread-safe)

      //! Constructor
      //! @param gpLengthScale Length scale parameter for GP kernel
      //! @param gpNoise Noise parameter for GP
      //! @param initialGen Generator for initial sampling
      //! @param acquisitionGen Generator for acquisition-based sampling
      //! @param verbose Whether to output progress information
      State(RealT gpLengthScale, RealT gpNoise,
            std::shared_ptr<SampleGenerator> initialGen,
            std::shared_ptr<SampleGenerator> acquisitionGen,
            bool verbose)
          : mGP(gpLengthScale, gpNoise),
            mInitialGen(initialGen->clone()),
            mAcquisitionGen(acquisitionGen->clone()),
            mFunctionCache(static_cast<RealT>(1e-10)),
            mVerbose(verbose)
      {}

      //! Evaluate function with caching support (thread-safe)
      //! @param point Point to evaluate
      //! @param func Function to evaluate
      //! @return Function value
      RealT evaluateWithCache(const VectorT<RealT> &point,
                             const std::function<RealT(const VectorT<RealT> &)> &func);

      //! Get cache statistics
      //! @return Pair of (cache_hits, cache_size)
      std::pair<size_t, size_t> getCacheStats() const
      {
        return std::make_pair(mCacheHits.load(), mFunctionCache.size());
      }

      //! Fit the surrogate model to the current data
      void fitSurrogate();

      //! Select the next batch of points to evaluate, filling with new points
      //! @param domain Domain of the function
      //! @param batchSize Number of points to select
      //! @return Vector of points to evaluate next (guaranteed to have batchSize elements if possible)
      std::vector<VectorT<RealT>> selectBatchWithFill(const CostDomain<RealT> &domain, size_t batchSize);

      //! Generate initial points for optimization
      //! @param domain Domain to sample from
      //! @param numPoints Number of points to generate
      //! @return Vector of initial points
      std::vector<VectorT<RealT>> generateInitialPoints(const CostDomain<RealT> &domain, size_t numPoints);

      //! Select the next batch of points to evaluate using the acquisition function
      //! @param domain Domain of the function
      //! @param batchSize Number of points to select
      //! @return Vector of points to evaluate next
      std::vector<VectorT<RealT>> selectBatch(const CostDomain<RealT> &domain, size_t batchSize);

      //! Add a new evaluated point to the state
      //! @param x Point coordinates
      //! @param y Function value at the point
      void addPoint(const VectorT<RealT> &x, RealT y)
      {
        mX.push_back(x);
        mY.push_back(y);
      }

      //! Get the best (minimum) function value found so far
      //! @return Minimum function value or max value if no points evaluated
      RealT getBestY() const
      {
        return mY.empty() ? std::numeric_limits<RealT>::max() : *std::min_element(mY.begin(), mY.end());
      }

      //! Get the best point found so far
      //! @return Best point or empty vector if no points evaluated
      VectorT<RealT> getBestX() const
      {
        if(mY.empty()) return VectorT<RealT>();
        auto minIt = std::min_element(mY.begin(), mY.end());
        return mX[static_cast<std::size_t>(std::distance(mY.begin(), minIt))];
      }

      //! Adapt GP length scale based on optimization progress
      //! @param domain Domain of the function
      //! @param iteration Current iteration number
      //! @param maxIters Maximum number of iterations
      void adaptLengthScale(const CostDomain<RealT> &domain, size_t iteration, size_t maxIters);

    private:
      bool mVerbose = false;//!< Whether to output progress information
    };

    //! Evaluate function on a batch of points, possibly in parallel, ensuring batch is filled
    //! @param batch Vector of points to evaluate
    //! @param func Function to evaluate
    //! @param state State object containing the function cache
    //! @param domain Domain for generating additional points if needed
    //! @return Vector of function values and points actually evaluated
    std::pair<std::vector<RealT>, std::vector<VectorT<RealT>>> evaluateBatchWithFill(
        const std::vector<VectorT<RealT>> &batch,
        const std::function<RealT(const VectorT<RealT> &)> &func,
        State &state,
        const CostDomain<RealT> &domain) const;

    //! Evaluate function on a batch of points, possibly in parallel
    //! @param batch Vector of points to evaluate
    //! @param func Function to evaluate
    //! @param state State object containing the function cache
    //! @return Vector of function values
    std::vector<RealT> evaluateBatch(
        const std::vector<VectorT<RealT>> &batch,
        const std::function<RealT(const VectorT<RealT> &)> &func,
        State &state) const;
  };

}// namespace Ravl2
