#include "Ravl2/Optimise/SampleGenerators.hh"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Ravl2
{

  // RandomSampleGenerator implementation

  RandomSampleGenerator::RandomSampleGenerator(bool fixedSeed, unsigned seed)
      : mFixedSeed(fixedSeed), mSeed(seed), mGenerator(fixedSeed ? seed : std::random_device {}())
  {
  }

  RandomSampleGenerator::RandomSampleGenerator(Configuration &config)
      : mFixedSeed(config.getBool("fixedSeed", "Use fixed random seed for reproducibility", true)),
        mSeed(config.getUnsigned("seed", "Random seed value if fixedSeed is true", 42, 0, std::numeric_limits<unsigned>::max())),
        mGenerator(mFixedSeed ? mSeed : std::random_device {}())
  {
  }

  std::vector<VectorT<RandomSampleGenerator::RealT>> RandomSampleGenerator::generatePoints(
    size_t numPoints)
  {

    if(mDomain.dim() == 0) {
      SPDLOG_ERROR("Domain not set. Call reset(domain) first. ");
      throw std::runtime_error("Domain not set. Call reset(domain) first.");
    }

    std::vector<VectorT<RealT>> points;
    points.reserve(numPoints);

    std::uniform_real_distribution<RealT> dist(0.0, 1.0);

    for(Eigen::Index i = 0; i < Eigen::Index(numPoints); ++i) {
      VectorT<RealT> point(mDomain.size());
      for(Eigen::Index j = 0; j < mDomain.dim(); ++j) {
        RealT u = dist(mGenerator);
        point[j] = mDomain.min(j) + u * (mDomain.max(j) - mDomain.min(j));
      }
      points.push_back(point);
    }

    mPosition += numPoints;
    return points;
  }

  std::shared_ptr<SampleGenerator> RandomSampleGenerator::clone() const
  {
    return std::make_shared<RandomSampleGenerator>(mFixedSeed, mSeed);
  }

  void RandomSampleGenerator::skip(size_t n)
  {
    if(mDomain.dim() == 0) {
      SPDLOG_ERROR("Domain not set. Call reset(domain) first. ");
      throw std::runtime_error("Domain not set. Call reset(domain) first.");
    }

    // For random generators, we need to advance the generator state
    // Generate and discard random values for each dimension for n points
    std::uniform_real_distribution<RealT> dist(0.0, 1.0);
    for(size_t i = 0; i < n; ++i) {
      for(Eigen::Index j = 0; j < mDomain.dim(); ++j) {
        dist(mGenerator);// Advance the generator by consuming random values
      }
    }
    mPosition += n;
  }

  // GridSampleGenerator implementation

  GridSampleGenerator::GridSampleGenerator(size_t pointsPerDim)
      : mPointsPerDim(pointsPerDim)
  {
    if(pointsPerDim < 2) {
      SPDLOG_ERROR("Invalid parameter: pointsPerDim must be at least 2, got {}", pointsPerDim);
      throw std::invalid_argument("pointsPerDim must be at least 2");
    }
  }

  GridSampleGenerator::GridSampleGenerator(Configuration &config)
      : mPointsPerDim(config.getUnsigned("pointsPerDim", "Number of points per dimension for grid sampling", 10, 2, 1000))
  {
  }

  std::vector<VectorT<GridSampleGenerator::RealT>> GridSampleGenerator::generatePoints(
    size_t numPoints)
  {

    if(mDomain.dim() == 0) {
      SPDLOG_ERROR("Domain not set. Call reset(domain) first.");
      throw std::runtime_error("Domain not set. Call reset(domain) first.");
    }

    Eigen::Index dims = mDomain.dim();

    // Calculate points per dimension to get approximately numPoints total
    Eigen::Index pointsPerDim = static_cast<Eigen::Index>(std::round(std::pow(numPoints, 1.0 / static_cast<double>(dims))));
    pointsPerDim = std::max(pointsPerDim, static_cast<Eigen::Index>(2));// At least 2 points per dimension

    std::vector<VectorT<RealT>> points;
    points.reserve(numPoints);

    // Generate points starting from current position
    auto totalGridPoints = static_cast<size_t>(std::pow(pointsPerDim, dims));

    for(size_t pointIdx = 0; pointIdx < numPoints; ++pointIdx) {
      size_t currentIndex = (mPosition + pointIdx) % totalGridPoints;
      VectorT<RealT> point(dims);

      // Convert linear index to multi-dimensional coordinates
      Eigen::Index temp = static_cast<Eigen::Index>(currentIndex);
      for(Eigen::Index dim = 0; dim < dims; ++dim) {
        Eigen::Index coord = temp % pointsPerDim;
        temp /= pointsPerDim;

        RealT t = (pointsPerDim == 1) ? 0.5F : static_cast<RealT>(coord) / static_cast<RealT>(pointsPerDim - 1);
        point[dim] = mDomain.min(dim) + t * (mDomain.max(dim) - mDomain.min(dim));
      }

      points.push_back(point);
    }

    mPosition += numPoints;
    return points;
  }

  std::shared_ptr<SampleGenerator> GridSampleGenerator::clone() const
  {
    return std::make_shared<GridSampleGenerator>(mPointsPerDim);
  }

  // SobolSampleGenerator implementation

  SobolSampleGenerator::SobolSampleGenerator(Configuration &config)
  {
    // Sobol generator currently has no configurable parameters,
    // but we include the constructor for consistency and future extensibility
    (void)config;// Suppress unused parameter warning
  }

  std::vector<VectorT<SobolSampleGenerator::RealT>> SobolSampleGenerator::generatePoints(
    size_t numPoints)
  {

    if(mDomain.dim() == 0) {
      throw std::runtime_error("Domain not set. Call reset(domain) first.");
    }

    std::vector<VectorT<RealT>> points;
    points.reserve(numPoints);

    // Use different prime bases for each dimension
    std::vector<size_t> bases = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};

    // Generate points starting from current position
    for(size_t i = 0; i < numPoints; ++i) {
      VectorT<RealT> point(mDomain.size());
      for(Eigen::Index j = 0; j < mDomain.dim(); ++j) {
        size_t base = bases[static_cast<size_t>(j) % bases.size()];
        // Use mPosition + i + 1 to continue the sequence from the current position
        double u = vanDerCorput(mPosition + i + 1, base);
        point[j] = mDomain.min(j) + static_cast<RealT>(u) * (mDomain.max(j) - mDomain.min(j));
      }
      points.push_back(point);
    }

    mPosition += numPoints;
    return points;
  }

  std::shared_ptr<SampleGenerator> SobolSampleGenerator::clone() const
  {
    return std::make_shared<SobolSampleGenerator>();
  }

  double SobolSampleGenerator::vanDerCorput(size_t n, size_t base)
  {
    double result = 0.0;
    double f = 1.0 / static_cast<double>(base);
    while(n > 0) {
      result += f * static_cast<double>(n % base);
      n /= base;
      f /= static_cast<double>(base);
    }
    return result;
  }

  // Component registration
  namespace
  {
    [[maybe_unused]] static auto randomGeneratorReg =
      defaultConfigFactory().registerNamedType<SampleGenerator, RandomSampleGenerator>("Random");

    [[maybe_unused]] static auto gridGeneratorReg =
      defaultConfigFactory().registerNamedType<SampleGenerator, GridSampleGenerator>("Grid");

    [[maybe_unused]] static auto sobolGeneratorReg =
      defaultConfigFactory().registerNamedType<SampleGenerator, SobolSampleGenerator>("Sobol");

  }// namespace

}// namespace Ravl2
