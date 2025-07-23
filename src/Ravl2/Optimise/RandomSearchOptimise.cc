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
      mMaxThreads(config.getUnsigned("maxThreads","Maximum number of threads for parallel evaluation (0 = single-threaded)", 0, 0, 100))
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

    // Use the component system to create the sample generator
    std::shared_ptr<SampleGenerator> generator;
    config.useComponent("sampleGenerator", "Sample point generator", generator, patternTypeStr);
    mSampleGenerator = generator;
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
      mMaxThreads(maxThreads)
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

    // Create the appropriate sample generator using direct constructors for programmatic interface
    switch(patternType) {
      case PatternType::Random:
        mSampleGenerator = std::make_shared<RandomSampleGenerator>(fixedSeed, seed);
        break;
      case PatternType::Grid:
        mSampleGenerator = std::make_shared<GridSampleGenerator>(gridPointsPerDim);
        break;
      case PatternType::Sobol:
        mSampleGenerator = std::make_shared<SobolSampleGenerator>();
        break;
    }
  }

  //! Constructor with custom sample generator
  RandomSearchOptimise::RandomSearchOptimise(
      size_t maxEvals,
      std::shared_ptr<SampleGenerator> sampleGenerator,
      size_t batchSize,
      size_t maxThreads,
      bool verbose
  ) : Optimise(verbose),
      mMaxEvals(maxEvals),
      mPatternType(PatternType::Random), // Default value, not used when custom generator is provided
      mBatchSize(batchSize),
      mMaxThreads(maxThreads),
      mSampleGenerator(sampleGenerator)
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
    if(!mSampleGenerator) {
      SPDLOG_ERROR("Sample generator cannot be null");
      throw std::invalid_argument("Sample generator cannot be null");
    }
  }

  //! Set pattern type and create corresponding generator
  void RandomSearchOptimise::setPatternType(PatternType type) {
    mPatternType = type;
    // Create appropriate sample generator using direct constructors
    switch(type) {
      case PatternType::Random:
        mSampleGenerator = std::make_shared<RandomSampleGenerator>();
        break;
      case PatternType::Grid:
        mSampleGenerator = std::make_shared<GridSampleGenerator>();
        break;
      case PatternType::Sobol:
        mSampleGenerator = std::make_shared<SobolSampleGenerator>();
        break;
    }
  }

  //! Find minimum of a function using random search sampling
  std::tuple<VectorT<RandomSearchOptimise::RealT>, RandomSearchOptimise::RealT> RandomSearchOptimise::minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start) const {

    if (!mSampleGenerator) {
      SPDLOG_ERROR("Sample generator is not initialized");
      throw std::runtime_error("Sample generator is not initialized");
    }

    // Set the domain for the generator once at the beginning
    mSampleGenerator->reset(domain);

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

      // Generate sampling points using the stateful generator
      std::vector<VectorT<RealT>> batch = mSampleGenerator->generatePoints(currentBatchSize);

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

  namespace {
    [[maybe_unused]] bool g_register = defaultConfigFactory().registerNamedType<Optimise, RandomSearchOptimise>("RandomSearchOptimise");
  }


} // namespace Ravl2
