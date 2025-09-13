#pragma once

#include "Optimise.hh"
#include "SampleGenerators.hh"
#include <vector>
#include <tuple>
#include <functional>
#include <random>
#include <limits>
#include <memory>

namespace Ravl2
{

  //! Random search optimization using predetermined sampling strategies.
  //!
  //! Implements optimization by systematically evaluating points according to
  //! sampling patterns such as random sampling, grid search, or Sobol sequences.
  //! This provides a simple baseline for comparison with more sophisticated methods.

  class RandomSearchOptimise : public Optimise
  {
  public:
    /// Real number type used for calculations
    using RealT = Optimise::RealT;

    //! Sampling pattern types
    enum class PatternType
    {
      Random,//!< Random uniform sampling
      Grid,  //!< Regular grid sampling
      Sobol  //!< Sobol quasi-random sequence (low-discrepancy)
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
      size_t gridPointsPerDim = 10);

    //! Constructor from configuration
    //! @param config Configuration object containing optimization parameters
    explicit RandomSearchOptimise(Configuration &config);

    //! Constructor with custom sample generator
    //! @param maxEvals Maximum number of function evaluations
    //! @param sampleGenerator Custom sample generator to use
    //! @param batchSize Number of points to evaluate in parallel (default: 1)
    //! @param maxThreads Maximum number of threads for parallel evaluation (0 = single-threaded, default: 0)
    //! @param verbose Whether to output progress information (default: false)
    explicit RandomSearchOptimise(
      size_t maxEvals,
      std::shared_ptr<SampleGenerator> sampleGenerator,
      size_t batchSize = 1,
      size_t maxThreads = 0,
      bool verbose = false);

    //! Find minimum of a function
    //! @param domain Domain of the function to minimize
    //! @param func Function to minimize
    //! @param start Optional starting point (ignored for random search patterns)
    //! @return Tuple containing the minimum point and its value
    [[nodiscard]] std::tuple<VectorT<RealT>, RealT> minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start = VectorT<RealT>()) const override;

    //! Find multiple minima of a function in batch mode
    //! @param domain Domain of the function to minimize
    //! @param func Function to minimize
    //! @param startPoints Optional starting points (ignored for random search patterns)
    //! @return Vector of tuples containing the minima points and their values
    std::vector<std::tuple<VectorT<RealT>, RealT>> minimiseBatch(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const std::vector<VectorT<RealT>> &startPoints = {}) const;

    // Setters for configuration
    void setMaxEvals(size_t maxEvals) { mMaxEvals = maxEvals; }
    void setPatternType(PatternType type);
    void setBatchSize(size_t batchSize) { mBatchSize = batchSize; }
    void setMaxThreads(size_t maxThreads) { mMaxThreads = maxThreads; }
    void setSampleGenerator(std::shared_ptr<SampleGenerator> generator) { mSampleGenerator = generator; }

    // Getters for configuration
    size_t getMaxEvals() const { return mMaxEvals; }
    PatternType getPatternType() const { return mPatternType; }
    size_t getBatchSize() const { return mBatchSize; }
    size_t getMaxThreads() const { return mMaxThreads; }
    const SampleGenerator *getSampleGenerator() const { return mSampleGenerator.get(); }

  private:
    size_t mMaxEvals = 100;                           //!< Maximum number of function evaluations
    PatternType mPatternType = PatternType::Random;   //!< Type of sampling pattern
    size_t mBatchSize = 1;                            //!< Size of the batch for parallel evaluations
    size_t mMaxThreads = 0;                           //!< Maximum number of threads for parallel evaluation (0 = single-threaded)
    std::shared_ptr<SampleGenerator> mSampleGenerator;//!< Sample point generator

    //! Evaluate function on a batch of points, possibly in parallel
    //! @param batch Vector of points to evaluate
    //! @param func Function to evaluate
    //! @return Vector of function values
    std::vector<RealT> evaluateBatch(
      const std::vector<VectorT<RealT>> &batch,
      const std::function<RealT(const VectorT<RealT> &)> &func) const;
  };

}// namespace Ravl2
