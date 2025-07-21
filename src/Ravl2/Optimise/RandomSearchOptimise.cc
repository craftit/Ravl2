#include "Ravl2/Optimise/RandomSearchOptimise.hh"
#include <random>
#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <cmath>

namespace Ravl2
{

  //! Construct from configuration
  RandomSearchOptimise::RandomSearchOptimise(Configuration &config)
      : Optimise(config),
      mMaxEvals(config.getUnsigned("maxEvals","Maximum number of function evaluations", 100, 1, 100000)),
      mBatchSize(config.getUnsigned("batchSize","Batch size for parallel evaluations", 1, 1, 100)),
      mMaxThreads(config.getUnsigned("maxThreads","Maximum number of threads for parallel evaluation (0 = single-threaded)", 0, 0, 100)),
      mFixedSeed(config.getBool("fixedSeed", "Use fixed random seed for reproducibility", true)),
      mSeed(config.getUnsigned("seed", "Random seed value if fixedSeed is true", 42, 0, std::numeric_limits<unsigned>::max())),
      mGridPointsPerDim(config.getUnsigned("gridPointsPerDim", "Points per dimension for grid sampling", 10, 2, 100))
  {
    std::string patternTypeStr = config.getString("patternType", "Sampling pattern type: Random, Grid, or Sobol", "Random");
    if (patternTypeStr == "Random") {
      mPatternType = PatternType::Random;
    } else if (patternTypeStr == "Grid") {
      mPatternType = PatternType::Grid;
    } else if (patternTypeStr == "Sobol") {
      mPatternType = PatternType::Sobol;
    } else {
      SPDLOG_ERROR("Invalid pattern type '{}' in configuration. Must be Random, Grid, or Sobol", patternTypeStr);
      throw std::invalid_argument("Invalid pattern type: " + patternTypeStr + ". Must be Random, Grid, or Sobol");
    }
  }

  //! Programmatic constructor
  RandomSearchOptimise::RandomSearchOptimise(
      size_t maxEvals,
      PatternType patternType,
      size_t batchSize,
      size_t maxThreads,
      bool verbose,
      bool fixedSeed,
      unsigned seed,
      size_t gridPointsPerDim
  ) : Optimise(verbose),
      mMaxEvals(maxEvals),
      mPatternType(patternType),
      mBatchSize(batchSize),
      mMaxThreads(maxThreads),
      mFixedSeed(fixedSeed),
      mSeed(seed),
      mGridPointsPerDim(gridPointsPerDim)
  {
    // Parameter validation
    if(maxEvals == 0) {
      SPDLOG_ERROR("Invalid parameter: maxEvals must be greater than 0, got {}", maxEvals);
      throw std::invalid_argument("maxEvals must be greater than 0");
    }
    if(batchSize == 0) {
      SPDLOG_ERROR("Invalid parameter: batchSize must be greater than 0, got {}", batchSize);
      throw std::invalid_argument("batchSize must be greater than 0");
    }
    if(gridPointsPerDim < 2) {
      SPDLOG_ERROR("Invalid parameter: gridPointsPerDim must be at least 2, got {}", gridPointsPerDim);
      throw std::invalid_argument("gridPointsPerDim must be at least 2");
    }
  }

  //! Find minimum of a function using random search sampling
  std::tuple<VectorT<RandomSearchOptimise::RealT>, RandomSearchOptimise::RealT> RandomSearchOptimise::minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start) const {

    if (mVerbose) {
      std::string patternName;
      switch(mPatternType) {
        case PatternType::Random: patternName = "Random"; break;
        case PatternType::Grid: patternName = "Grid"; break;
        case PatternType::Sobol: patternName = "Sobol"; break;
      }
      SPDLOG_INFO("Starting RandomSearchOptimise with {} sampling, maxEvals={}, batchSize={}",
                   patternName, mMaxEvals, mBatchSize);
    }

    std::mt19937 gen(mFixedSeed ? mSeed : std::random_device{}());

    VectorT<RealT> bestPoint;
    RealT bestValue = std::numeric_limits<RealT>::max();

    size_t evaluationsUsed = 0;

    // If a start point is provided, evaluate it first
    if (start.size() != 0) {
      if (start.size() != domain.dim()) {
        SPDLOG_ERROR("Start point dimension mismatch: start point has {} dimensions, domain has {} dimensions",
                     start.size(), domain.size());
        throw std::invalid_argument("Start point dimension mismatch with domain");
      }

      // Check if start point is within domain bounds
      bool withinBounds = true;
      for (Eigen::Index i = 0; i < start.size(); ++i) {
        if (start[i] < domain.min(i) || start[i] > domain.max(i)) {
          withinBounds = false;
          break;
        }
      }

      if (withinBounds) {
        RealT startValue = func(start);
        bestPoint = start;
        bestValue = startValue;
        evaluationsUsed = 1;

        if (mVerbose) {
          SPDLOG_INFO("Evaluated start point, value: {}", startValue);
        }
      } else if (mVerbose) {
        SPDLOG_WARN("Start point is outside domain bounds, ignoring");
      }
    }

    while (evaluationsUsed < mMaxEvals) {
      // Determine batch size for this iteration
      size_t currentBatchSize = std::min(mBatchSize, mMaxEvals - evaluationsUsed);

      // Generate sampling points
      std::vector<VectorT<RealT>> batch = generateSamplingPoints(domain, currentBatchSize, gen);

      // Evaluate the batch
      std::vector<RealT> values = evaluateBatch(batch, func);

      // Update best point
      for (size_t i = 0; i < batch.size(); ++i) {
        if (values[i] < bestValue) {
          bestValue = values[i];
          bestPoint = batch[i];
        }
      }

      evaluationsUsed += batch.size();

      if (mVerbose) {
        SPDLOG_INFO("Evaluated {} points, best value so far: {}", evaluationsUsed, bestValue);
      }
    }

    if (mVerbose) {
      SPDLOG_INFO("RandomSearchOptimise completed. Best value: {} after {} evaluations", bestValue, evaluationsUsed);
    }

    return std::make_tuple(bestPoint, bestValue);
  }

  //! Find multiple minima using batch mode
  std::vector<std::tuple<VectorT<RandomSearchOptimise::RealT>, RandomSearchOptimise::RealT>> RandomSearchOptimise::minimiseBatch(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const std::vector<VectorT<RealT>> &startPoints) const
  {
    (void) startPoints; // Ignore start points for random search optimization
    // For random search optimization, we ignore start points and just run the standard minimization
    auto result = minimise(domain, func);
    return {result};
  }

  //! Generate sampling points according to the specified pattern
  std::vector<VectorT<RandomSearchOptimise::RealT>> RandomSearchOptimise::generateSamplingPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints,
      std::mt19937 &gen) const {

    switch (mPatternType) {
      case PatternType::Random:
        return generateRandomPoints(domain, numPoints, gen);
      case PatternType::Grid:
        return generateGridPoints(domain, numPoints);
      case PatternType::Sobol:
        return generateSobolPoints(domain, numPoints);
      default:
        {
          SPDLOG_ERROR("Unknown pattern type: {}", static_cast<int>(mPatternType));
          throw std::runtime_error("Unknown pattern type");
        }
    }
  }

  //! Generate random sampling points
  std::vector<VectorT<RandomSearchOptimise::RealT>> RandomSearchOptimise::generateRandomPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints,
      std::mt19937 &gen) const {

    std::vector<VectorT<RealT>> points;
    points.reserve(numPoints);

    std::uniform_real_distribution<RealT> dist(0.0, 1.0);

    for (Eigen::Index i = 0; i < Eigen::Index(numPoints); ++i) {
      VectorT<RealT> point(domain.size());
      for (Eigen::Index j = 0; j < domain.dim(); ++j) {
        RealT u = dist(gen);
        point[j] = domain.min(j) + u * (domain.max(j) - domain.min(j));
      }
      points.push_back(point);
    }

    return points;
  }

  //! Generate grid sampling points
  std::vector<VectorT<RandomSearchOptimise::RealT>> RandomSearchOptimise::generateGridPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints) const {

    Eigen::Index dims = domain.dim();
    if (dims == 0) {
      return {};
    }

    // Calculate points per dimension to get approximately numPoints total
    Eigen::Index pointsPerDim = static_cast<Eigen::Index>(std::round(std::pow(numPoints, 1.0 / dims)));
    pointsPerDim = std::max(pointsPerDim, Eigen::Index(2)); // At least 2 points per dimension

    std::vector<VectorT<RealT>> points;

    // Generate all combinations
    std::function<void(VectorT<RealT>&, Eigen::Index)> generateRecursive =
        [&](VectorT<RealT>& currentPoint, Eigen::Index dim) {
          if (dim == dims) {
            points.push_back(currentPoint);
            return;
          }

          for (Eigen::Index i = 0; i < pointsPerDim; ++i) {
            RealT t = (pointsPerDim == 1) ? 0.5 : static_cast<RealT>(i) / (pointsPerDim - 1);
            currentPoint[dim] = domain.min(dim) + t * (domain.max(dim) - domain.min(dim));
            generateRecursive(currentPoint, dim + 1);
          }
        };

    VectorT<RealT> currentPoint(dims);
    generateRecursive(currentPoint, 0);

    return points;
  }

  //! Generate Sobol sequence points (simplified implementation)
  std::vector<VectorT<RandomSearchOptimise::RealT>> RandomSearchOptimise::generateSobolPoints(
      const CostDomain<RealT> &domain,
      size_t numPoints) const {

    // Simple Sobol-like sequence using van der Corput sequence for each dimension
    std::vector<VectorT<RealT>> points;
    points.reserve(numPoints);

    auto vanDerCorput = [](size_t n, size_t base) -> double {
      double result = 0.0;
      double f = 1.0 / base;
      while (n > 0) {
        result += f * (n % base);
        n /= base;
        f /= base;
      }
      return result;
    };

    // Use different prime bases for each dimension
    std::vector<size_t> bases = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};

    for (Eigen::Index i = 0; i < Eigen::Index(numPoints); ++i) {
      VectorT<RealT> point(domain.size());
      for (Eigen::Index j = 0; j < domain.dim(); ++j) {
        size_t base = bases[static_cast<size_t>(j) % bases.size()];
        double u = vanDerCorput(static_cast<size_t>(i + 1), base); // +1 to avoid 0
        point[j] = domain.min(j) + static_cast<RealT>(u) * (domain.max(j) - domain.min(j));
      }
      points.push_back(point);
    }

    return points;
  }

  //! Evaluate function on a batch of points, possibly in parallel
  std::vector<RandomSearchOptimise::RealT> RandomSearchOptimise::evaluateBatch(
      const std::vector<VectorT<RealT>> &batch,
      const std::function<RealT(const VectorT<RealT> &)> &func) const {

    std::vector<RealT> results(batch.size());

    if (mMaxThreads == 0 || batch.size() == 1) {
      // Single-threaded evaluation
      for (size_t i = 0; i < batch.size(); ++i) {
        results[i] = func(batch[i]);
      }
    } else {
      // Multithreaded evaluation
      size_t hwThreads = std::thread::hardware_concurrency();
      if (hwThreads == 0) hwThreads = 1; // Fallback if hardware_concurrency returns 0
      size_t numThreads = std::min(mMaxThreads, hwThreads);
      numThreads = std::min(numThreads, batch.size());

      std::vector<std::future<void>> futures;
      std::atomic<size_t> nextIndex(0);

      for (size_t t = 0; t < numThreads; ++t) {
        futures.push_back(std::async(std::launch::async, [&]() {
          size_t index;
          while ((index = nextIndex.fetch_add(1)) < batch.size()) {
            try {
              results[index] = func(batch[index]);
            } catch (const std::exception& e) {
              SPDLOG_ERROR("Exception in function evaluation: {}", e.what());
              results[index] = std::numeric_limits<RealT>::max();
            }
          }
        }));
      }

      // Wait for all threads to complete
      for (auto& future : futures) {
        future.wait();
      }
    }

    return results;
  }

} // namespace Ravl2
