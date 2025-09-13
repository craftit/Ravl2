#pragma once

#include "CostDomain.hh"
#include "Optimise.hh"
#include "GaussianProcess.hh"
#include <vector>
#include <random>
#include <memory>

namespace Ravl2
{

  //! Base class for sample point generators
  //!
  //! Provides a common interface for generating sample points within a domain
  //! using different strategies such as random, grid, or quasi-random sequences.
  class SampleGenerator
  {
  public:
    /// Real number type used for calculations
    using RealT = Optimise::RealT;

    /// Virtual destructor
    virtual ~SampleGenerator() = default;

    //! Generate sample points within the configured domain
    //! @param numPoints Number of points to generate
    //! @return Vector of sampling points
    virtual std::vector<VectorT<RealT>> generatePoints(size_t numPoints) = 0;

    //! Reset the generator to its initial state with a new domain
    //! @param domain Domain to sample from
    virtual void reset(const CostDomain<RealT> &domain) = 0;

    //! Reset the generator to its initial state using the current domain
    virtual void reset() = 0;

    //! Skip the next n points in the sequence
    //! @param n Number of points to skip
    virtual void skip(size_t n) = 0;

    //! Get the current position in the sequence
    //! @return Current position (number of points generated so far)
    virtual size_t getCurrentPosition() const = 0;

    //! Clone this generator
    virtual std::shared_ptr<SampleGenerator> clone() const = 0;
  };

  //! Random uniform sample generator
  //!
  //! Generates uniformly distributed random points within the domain bounds.
  class RandomSampleGenerator : public SampleGenerator
  {
  public:
    //! Constructor
    //! @param fixedSeed Whether to use a fixed random seed for reproducibility (default: true)
    //! @param seed Random seed value if fixedSeed is true (default: 42)
    explicit RandomSampleGenerator(bool fixedSeed = true, unsigned seed = 42);

    //! Construct from configuration
    //! @param config Configuration object containing generator parameters
    explicit RandomSampleGenerator(Configuration &config);

    //! Generate random sample points
    //! @param numPoints Number of points to generate
    //! @return Vector of random points
    std::vector<VectorT<RealT>> generatePoints(size_t numPoints) override;

    //! Clone this generator
    std::shared_ptr<SampleGenerator> clone() const override;

    //! Set random seed
    void setSeed(unsigned seed)
    {
      mSeed = seed;
      reset();
    }

    //! Get random seed
    unsigned getSeed() const { return mSeed; }

    //! Set whether to use fixed seed
    void setFixedSeed(bool fixedSeed) { mFixedSeed = fixedSeed; }

    //! Get whether using fixed seed
    bool getFixedSeed() const { return mFixedSeed; }

    void reset(const CostDomain<RealT> &domain) override
    {
      mDomain = domain;
      reset();
    }
    void reset() override
    {
      mGenerator.seed(mFixedSeed ? mSeed : std::random_device {}());
      mPosition = 0;
    }
    void skip(size_t n) override;
    size_t getCurrentPosition() const override { return mPosition; }

  private:
    bool mFixedSeed;          //!< Whether to use a fixed random seed for reproducibility
    unsigned mSeed;           //!< Random seed value if fixed seed is enabled
    std::mt19937 mGenerator;  //!< Random number generator (removed mutable)
    size_t mPosition = 0;     //!< Current position in the sequence
    CostDomain<RealT> mDomain;//!< Current domain
  };

  //! Grid sample generator
  //!
  //! Generates points on a regular grid within the domain bounds.
  class GridSampleGenerator : public SampleGenerator
  {
  public:
    //! Constructor
    //! @param pointsPerDim Number of points per dimension (default: 10)
    explicit GridSampleGenerator(size_t pointsPerDim = 10);

    //! Construct from configuration
    //! @param config Configuration object containing generator parameters
    explicit GridSampleGenerator(Configuration &config);

    //! Generate grid sample points
    //! @param numPoints Target number of points (actual number may differ)
    //! @return Vector of grid points
    std::vector<VectorT<RealT>> generatePoints(size_t numPoints) override;

    //! Clone this generator
    std::shared_ptr<SampleGenerator> clone() const override;

    //! Set points per dimension
    void setPointsPerDim(size_t pointsPerDim)
    {
      mPointsPerDim = pointsPerDim;
      reset();
    }

    //! Get points per dimension
    size_t getPointsPerDim() const { return mPointsPerDim; }

    void reset(const CostDomain<RealT> &domain) override
    {
      mDomain = domain;
      reset();
    }
    void reset() override { mPosition = 0; }
    void skip(size_t n) override { mPosition += n; }
    size_t getCurrentPosition() const override { return mPosition; }

  private:
    size_t mPointsPerDim;     //!< Number of points per dimension for grid sampling
    size_t mPosition = 0;     //!< Current position in the grid sequence
    CostDomain<RealT> mDomain;//!< Current domain
  };

  //! Sobol quasi-random sequence generator
  //!
  //! Generates low-discrepancy sequences using a simplified Sobol-like approach
  //! based on van der Corput sequences with different prime bases per dimension.
  class SobolSampleGenerator : public SampleGenerator
  {
  public:
    //! Default constructor
    SobolSampleGenerator() = default;

    //! Construct from configuration
    //! @param config Configuration object containing generator parameters
    explicit SobolSampleGenerator(Configuration &config);

    //! Generate Sobol sequence points
    //! @param numPoints Number of points to generate
    //! @return Vector of Sobol sequence points
    std::vector<VectorT<RealT>> generatePoints(size_t numPoints) override;

    //! Clone this generator
    std::shared_ptr<SampleGenerator> clone() const override;

    void reset(const CostDomain<RealT> &domain) override
    {
      mDomain = domain;
      reset();
    }
    void reset() override { mPosition = 0; }
    void skip(size_t n) override { mPosition += n; }
    size_t getCurrentPosition() const override { return mPosition; }

  private:
    //! Van der Corput sequence generator
    //! @param n Index in sequence
    //! @param base Base for the sequence
    //! @return Value in [0,1)
    static double vanDerCorput(size_t n, size_t base);

    size_t mPosition = 0;     //!< Current position in the Sobol sequence
    CostDomain<RealT> mDomain;//!< Current domain
  };

}// namespace Ravl2
