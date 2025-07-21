#pragma once

#include "Optimise.hh"
#include <vector>
#include <tuple>
#include <functional>
#include <random>
#include <limits>

namespace Ravl2 {

//! Random search optimization using predetermined sampling strategies.
//!
//! Implements optimization by systematically evaluating points according to
//! sampling patterns such as random sampling, grid search, or Sobol sequences.
//! This provides a simple baseline for comparison with more sophisticated methods.

class RandomSearchOptimise : public Optimise {
public:
  /// Real number type used for calculations
  using RealT = Optimise::RealT;

  //! Sampling pattern types
  enum class PatternType {
    Random,     //!< Random uniform sampling
    Grid,       //!< Regular grid sampling
    Sobol       //!< Sobol quasi-random sequence (low-discrepancy)
  };

  /// Default constructor
  RandomSearchOptimise() = default;

  //! Programmatic constructor with common parameters.
  //! @param maxEvals Maximum number of function evaluations
  //! @param patternType Type of sampling pattern to use (default: Random)
  //! @param batchSize Number of points to evaluate in parallel (default: 1)
  //! @param maxThreads Maximum number of threads for parallel evaluation (0 = single-threaded, default: 0)
  //! @param verbose Whether to output progress information (default: false)
  //! @param fixedSeed Whether to use fixed random seed for reproducibility (default: true)
  //! @param seed Random seed value if fixedSeed is true (default: 42)
  //! @param gridPointsPerDim Points per dimension for grid sampling (default: 10)
  explicit RandomSearchOptimise(
      size_t maxEvals,
      PatternType patternType = PatternType::Random,
      size_t batchSize = 1,
      size_t maxThreads = 0,
      bool verbose = false,
      bool fixedSeed = true,
      unsigned seed = 42,
      size_t gridPointsPerDim = 10
  );

  //! Constructor from configuration
  //! @param config Configuration object containing optimization parameters
  explicit RandomSearchOptimise(Configuration &config);

  //! Find minimum of a function
  //! @param domain Domain of the function to minimize
  //! @param func Function to minimize
  //! @param start Optional starting point (ignored for random search patterns)
  //! @return Tuple containing the minimum point and its value
  [[nodiscard]] std::tuple<VectorT<RealT>, RealT> minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start = VectorT<RealT>()
  ) const override;

  //! Find multiple minima of a function in batch mode
  //! @param domain Domain of the function to minimize
  //! @param func Function to minimize
  //! @param startPoints Optional starting points (ignored for random search patterns)
  //! @return Vector of tuples containing the minima points and their values
  std::vector<std::tuple<VectorT<RealT>, RealT>> minimiseBatch(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const std::vector<VectorT<RealT>> &startPoints = {}
  ) const;

  // Setters for configuration
  void setMaxEvals(size_t maxEvals) { mMaxEvals = maxEvals; }
  void setPatternType(PatternType type) { mPatternType = type; }
  void setBatchSize(size_t batchSize) { mBatchSize = batchSize; }
  void setMaxThreads(size_t maxThreads) { mMaxThreads = maxThreads; }
  void setFixedSeed(bool fixedSeed) { mFixedSeed = fixedSeed; }
  void setSeed(unsigned seed) { mSeed = seed; }
  void setGridPointsPerDim(size_t gridPointsPerDim) { mGridPointsPerDim = gridPointsPerDim; }

  // Getters for configuration
  size_t getMaxEvals() const { return mMaxEvals; }
  PatternType getPatternType() const { return mPatternType; }
  size_t getBatchSize() const { return mBatchSize; }
  size_t getMaxThreads() const { return mMaxThreads; }
  bool getFixedSeed() const { return mFixedSeed; }
  unsigned getSeed() const { return mSeed; }
  size_t getGridPointsPerDim() const { return mGridPointsPerDim; }

private:
  size_t mMaxEvals = 100; //!< Maximum number of function evaluations
  PatternType mPatternType = PatternType::Random; //!< Type of sampling pattern
  size_t mBatchSize = 1; //!< Size of the batch for parallel evaluations
  size_t mMaxThreads = 0; //!< Maximum number of threads for parallel evaluation (0 = single-threaded)
  bool mFixedSeed = true; //!< Whether to use a fixed random seed for reproducibility
  unsigned mSeed = 42; //!< Random seed value if fixed seed is enabled
  size_t mGridPointsPerDim = 10; //!< Number of points per dimension for grid sampling

  //! Generate sampling points according to the specified pattern
  //! @param domain Domain to sample from
  //! @param numPoints Number of points to generate
  //! @param gen Random number generator
  //! @param offset Starting offset for deterministic patterns (Grid/Sobol)
  //! @return Vector of sampling points
  std::vector<VectorT<RealT>> generateSamplingPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints,
      std::mt19937 &gen,
      size_t offset = 0
  ) const;

  //! Generate random sampling points
  //! @param domain Domain to sample from
  //! @param numPoints Number of points to generate
  //! @param gen Random number generator
  //! @return Vector of random points
  std::vector<VectorT<RealT>> generateRandomPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints,
      std::mt19937 &gen
  ) const;

  //! Generate grid sampling points
  //! @param domain Domain to sample from
  //! @param numPoints Target number of points (actual number may differ)
  //! @param offset Starting offset in the grid sequence
  //! @return Vector of grid points
  std::vector<VectorT<RealT>> generateGridPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints,
      size_t offset = 0
  ) const;

  //! Generate Sobol sequence points
  //! @param domain Domain to sample from
  //! @param numPoints Number of points to generate
  //! @param offset Starting offset in the Sobol sequence
  //! @return Vector of Sobol sequence points
  std::vector<VectorT<RealT>> generateSobolPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints,
      size_t offset = 0
  ) const;

  //! Evaluate function on a batch of points, possibly in parallel
  //! @param batch Vector of points to evaluate
  //! @param func Function to evaluate
  //! @return Vector of function values
  std::vector<RealT> evaluateBatch(
      const std::vector<VectorT<RealT>> &batch,
      const std::function<RealT(const VectorT<RealT> &)> &func
  ) const;
};

} // namespace Ravl2
