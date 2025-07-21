#pragma once

#include "Optimise.hh"
#include "GaussianProcess.hh"
#include <vector>
#include <tuple>
#include <functional>
#include <random>
#include <limits>

namespace Ravl2 {

//! Bayesian optimization using a Gaussian Process surrogate model.
//!
//! Implements Bayesian optimization to find global minima of expensive functions
//! by building a probabilistic surrogate model of the objective function.

class BayesianOptimise : public Optimise {
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
    size_t minItersBeforeConvergence = 5);

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
      const VectorT<RealT> &start = VectorT<RealT>()
  ) const override;

  //! Find multiple minima of a function in batch mode
  //! @param domain Domain of the function to minimize
  //! @param func Function to minimize
  //! @param startPoints Optional starting points
  //! @return Vector of tuples containing the minima points and their values
  std::vector<std::tuple<VectorT<RealT>, RealT>> minimiseBatch(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const std::vector<VectorT<RealT>> &startPoints
  ) const;

  //! Setters for configuration parameters
  void setBatchSize(size_t batchSize) { mBatchSize = batchSize; }
  void setMaxIters(size_t maxIters) { mMaxIters = maxIters; }
  void setAdaptiveLengthScale(bool adaptive) { mAdaptiveLengthScale = adaptive; }
  void setMaxThreads(size_t maxThreads) { mMaxThreads = maxThreads; }
  void setFixedSeed(bool fixedSeed) { mFixedSeed = fixedSeed; }
  void setSeed(unsigned seed) { mSeed = seed; }
  void setGPLengthScale(RealT lengthScale) { mGPLengthScale = lengthScale; }
  void setGPNoise(RealT noise) { mGPNoise = noise; }
  void setTolerance(RealT tolerance) { mTolerance = tolerance; }
  void setMinItersBeforeConvergence(size_t minIters) { mMinItersBeforeConvergence = minIters; }
  void setExplorationWeight(RealT weight) { mExplorationWeight = weight; }
  void setLocalSearchRadius(RealT radius) { mLocalSearchRadius = radius; }
  void setNumLocalCandidates(size_t numCandidates) { mNumLocalCandidates = numCandidates; }


private:

  size_t mBatchSize = 1; //!< Size of the batch for parallel evaluations
  size_t mMaxIters = 20; //!< Maximum number of optimization iterations
  size_t mMaxThreads = 0; //!< Maximum number of threads for parallel evaluation (0 = single-threaded)
  bool mFixedSeed = true; //!< Whether to use a fixed random seed for reproducibility
  unsigned mSeed = 42; //!< Random seed value if fixed seed is enabled
  RealT mGPLengthScale = RealT(1.0); //!< Length scale parameter for Gaussian Process kernel
  RealT mGPNoise = RealT(1e-6); //!< Noise parameter for Gaussian Process
  RealT mTolerance = RealT(1e-6); //!< Convergence tolerance for early stopping
  size_t mMinItersBeforeConvergence = 5; //!< Minimum iterations before checking convergence
  RealT mExplorationWeight = RealT(0.1); //!< Weight for exploration vs exploitation (0=pure exploitation, 1=pure exploration)
  RealT mLocalSearchRadius = RealT(0.1); //!< Radius for local search around best points (fraction of domain size)
  size_t mNumLocalCandidates = 0; //!< Number of candidates to generate around best points (0=auto: batchSize*2)
  bool mAdaptiveLengthScale = false; //!< Whether to adapt GP length scale during optimization

  //! Evaluate function on a batch of points, possibly in parallel
  //! @param batch Vector of points to evaluate
  //! @param func Function to evaluate
  //! @return Vector of function values
  std::vector<RealT> evaluateBatch(const std::vector<VectorT<RealT>> &batch, const std::function<RealT(const VectorT<RealT> &)> &func) const;

  //! Internal state class for Bayesian optimization process
  //!
  //! Maintains the current state of the optimization including
  //! evaluated points, function values, and surrogate model.
  class State {
  public:
    std::vector<VectorT<RealT>> mX;  //!< Points evaluated so far
    std::vector<RealT> mY; //!< Function values for evaluated points
    GaussianProcess<RealT> mGP; //!< Gaussian Process surrogate model
    std::mt19937 mGen; //!< Random number generator

    //! Constructor
    //! @param fixedSeed Whether to use a fixed seed
    //! @param seed Seed value if using fixed seed
    //! @param gpLengthScale Length scale parameter for GP kernel
    //! @param gpNoise Noise parameter for GP
    State(bool fixedSeed, unsigned seed, RealT gpLengthScale, RealT gpNoise,bool verbose)
      : mGP(gpLengthScale, gpNoise), mGen(fixedSeed ? seed : std::random_device{}()), mVerbose(verbose)
    {}

    //! Fit the surrogate model to current data points
    void fitSurrogate();

    //! Predict function value at a point using the surrogate model
    //! @param x Point to predict
    //! @return Predicted function value
    RealT predictSurrogate(const VectorT<RealT> &x) const;

    //! Select the next batch of points to evaluate
    //! @param domain Domain of the function
    //! @param batchSize Number of points to select
    //! @return Vector of points to evaluate next
    std::vector<VectorT<RealT>> selectBatch(const CostDomain<RealT> &domain, size_t batchSize);

    //! Select the next batch of points with improved exploration
    //! @param domain Domain of the function
    //! @param batchSize Number of points to select
    //! @param explorationWeight Weight for exploration vs exploitation
    //! @param localSearchRadius Radius for local search around best points
    //! @param numLocalCandidates Number of candidates to generate around best points
    //! @return Vector of points to evaluate next
    std::vector<VectorT<RealT>> selectBatchImproved(
        const CostDomain<RealT> &domain,
        size_t batchSize,
        RealT explorationWeight,
        RealT localSearchRadius,
        size_t numLocalCandidates);

    //! Add a new evaluated point to the state
    //! @param x Point coordinates
    //! @param y Function value at the point
    void addPoint(const VectorT<RealT> &x, RealT y) {
      mX.push_back(x);
      mY.push_back(y);
      // Invalidate caches when new data is added
      mCandidatesCacheValid = false;
      mPredictionsCacheValid = false;
    }

    //! Get the best (minimum) function value found so far
    //! @return Minimum function value or max value if no points evaluated
    RealT getBestY() const {
      return mY.empty() ? std::numeric_limits<RealT>::max() :
             *std::min_element(mY.begin(), mY.end());
    }

    //! Get the best point found so far
    //! @return Best point or empty vector if no points evaluated
    VectorT<RealT> getBestX() const {
      if(mY.empty()) return VectorT<RealT>();
      auto minIt = std::min_element(mY.begin(), mY.end());
      return mX[static_cast<std::size_t>(std::distance(mY.begin(), minIt))];
    }

    //! Adapt GP length scale based on optimization progress
    //! @param domain Domain of the function
    //! @param adaptiveLengthScale Whether to enable adaptive scaling
    void adaptLengthScale(const CostDomain<RealT> &domain, bool adaptiveLengthScale);

    //! Set the verbose mode for outputting progress information
    //! @param verbose Whether to output progress information
    void setVerbose(bool verbose) {
      mVerbose = verbose;
    }
  private:
    // Caching infrastructure for performance optimization
    bool mVerbose = false; //!< Whether to output progress information
    std::vector<VectorT<RealT>> mCandidatesCache; //!< Cached candidate points
    VectorT<RealT> mMeanCache;                    //!< Cached GP mean predictions
    VectorT<RealT> mVarianceCache;                //!< Cached GP variance predictions
    std::vector<std::pair<RealT, size_t>> mEICache; //!< Cached EI scores with indices
    bool mCandidatesCacheValid = false;           //!< Whether candidates cache is valid
    bool mPredictionsCacheValid = false;          //!< Whether predictions cache is valid
    bool mEICacheValid = false;                   //!< Whether EI cache is valid
    RealT mLastBestY = std::numeric_limits<RealT>::max(); //!< Last bestY used for EI calculation

    //! Generate or retrieve cached candidate points
    //! @param domain Domain of the function
    //! @param nCandidates Number of candidates to generate
    const std::vector<VectorT<RealT>>& getCandidates(const CostDomain<RealT> &domain, size_t nCandidates);

    //! Compute or retrieve cached GP predictions
    //! @param candidates Candidate points to predict
    void computePredictions(const std::vector<VectorT<RealT>>& candidates);

    //! Compute or retrieve cached Expected Improvement scores
    //! @param bestY Current best function value
    void computeExpectedImprovement(RealT bestY);
  };
};

} // namespace Ravl2
