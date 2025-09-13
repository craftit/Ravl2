#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Optimise/CostDomain.hh"
#include "Ravl2/Optimise/RandomSearchOptimise.hh"
#include "Ravl2/Optimise/SampleGenerators.hh"
#include <cmath>
#include <spdlog/spdlog.h>

namespace Ravl2
{
  namespace
  {
    // Test functions for optimization
    float quadratic(const VectorT<float> &x) {
      double sum = 0.0;
      for(int i = 0; i < x.size(); ++i) {
        sum += std::pow(x[i] - 1.0f, 2);
      }
      return static_cast<float>(sum);
    }

    float rosenbrock(const VectorT<float> &x) {
      if (x.size() != 2) return 1e6f; // Invalid input
      float a = 1.0f;
      float b = 100.0f;
      return (a - x[0]) * (a - x[0]) + b * (x[1] - x[0] * x[0]) * (x[1] - x[0] * x[0]);
    }

    float sphere(const VectorT<float> &x) {
      float sum = 0.0f;
      for(int i = 0; i < x.size(); ++i) {
        sum += x[i] * x[i];
      }
      return sum;
    }
  }

  TEST_CASE("RandomSearchOptimise finds minimum of quadratic function", "[RandomSearchOptimise]")
  {
    CostDomain<float> domain({0.0f, 0.0f}, {2.0f, 2.0f});

    SECTION("Random sampling") {
      RandomSearchOptimise optimizer(200, RandomSearchOptimise::PatternType::Random, 10, 0, false, true, 42);

      SPDLOG_INFO("Testing RandomSearchOptimise with Random sampling");
      auto result = optimizer.minimise(domain, quadratic);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("Random: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      REQUIRE(std::abs(bestX[0] - 1.0f) < 0.5f);
      REQUIRE(std::abs(bestX[1] - 1.0f) < 0.5f);
      REQUIRE(best < 0.5f);
    }

    SECTION("Grid sampling") {
      RandomSearchOptimise optimizer(200, RandomSearchOptimise::PatternType::Grid, 10, 0, false, true, 42, 10);

      SPDLOG_INFO("Testing RandomSearchOptimise with Grid sampling");
      auto result = optimizer.minimise(domain, quadratic);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("Grid: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      REQUIRE(std::abs(bestX[0] - 1.0f) < 0.5f);
      REQUIRE(std::abs(bestX[1] - 1.0f) < 0.5f);
      REQUIRE(best < 0.5f);
    }

    SECTION("Sobol sampling") {
      RandomSearchOptimise optimizer(200, RandomSearchOptimise::PatternType::Sobol, 10, 0, false, true, 42);

      SPDLOG_INFO("Testing RandomSearchOptimise with Sobol sampling");
      auto result = optimizer.minimise(domain, quadratic);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("Sobol: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      REQUIRE(std::abs(bestX[0] - 1.0f) < 0.5f);
      REQUIRE(std::abs(bestX[1] - 1.0f) < 0.5f);
      REQUIRE(best < 0.5f);
    }
  }

  TEST_CASE("SampleGenerators work independently", "[SampleGenerators]")
  {
    CostDomain<float> domain({0.0f, 0.0f}, {2.0f, 2.0f});

    SECTION("RandomSampleGenerator") {
      auto generator = std::make_unique<RandomSampleGenerator>(true, 42);
      generator->reset(domain);
      auto points = generator->generatePoints(10);

      REQUIRE(points.size() == 10);
      for (const auto& point : points) {
        REQUIRE(point.size() == 2);
        REQUIRE(point[0] >= 0.0f);
        REQUIRE(point[0] <= 2.0f);
        REQUIRE(point[1] >= 0.0f);
        REQUIRE(point[1] <= 2.0f);
      }
    }

    SECTION("GridSampleGenerator") {
      auto generator = std::make_unique<GridSampleGenerator>(5);
      generator->reset(domain);
      auto points = generator->generatePoints(25);

      REQUIRE(points.size() == 25);
      for (const auto& point : points) {
        REQUIRE(point.size() == 2);
        REQUIRE(point[0] >= 0.0f);
        REQUIRE(point[0] <= 2.0f);
        REQUIRE(point[1] >= 0.0f);
        REQUIRE(point[1] <= 2.0f);
      }
    }

    SECTION("SobolSampleGenerator") {
      auto generator = std::make_unique<SobolSampleGenerator>();
      generator->reset(domain);
      auto points = generator->generatePoints(10);

      REQUIRE(points.size() == 10);
      for (const auto& point : points) {
        REQUIRE(point.size() == 2);
        REQUIRE(point[0] >= 0.0f);
        REQUIRE(point[0] <= 2.0f);
        REQUIRE(point[1] >= 0.0f);
        REQUIRE(point[1] <= 2.0f);
      }
    }

    SECTION("Factory function with config") {
      std::string randomConfigJson = R"({
        "fixedSeed": true,
        "seed": 999
      })";

      std::string gridConfigJson = R"({
        "pointsPerDim": 5
      })";

      std::string sobolConfigJson = R"({})";

      Configuration randomConfig = Configuration::fromJSONString(randomConfigJson);
      Configuration gridConfig = Configuration::fromJSONString(gridConfigJson);
      Configuration sobolConfig = Configuration::fromJSONString(sobolConfigJson);

      auto randomGen = std::make_unique<RandomSampleGenerator>(randomConfig);
      auto gridGen = std::make_unique<GridSampleGenerator>(gridConfig);
      auto sobolGen = std::make_unique<SobolSampleGenerator>(sobolConfig);

      REQUIRE(randomGen != nullptr);
      REQUIRE(gridGen != nullptr);
      REQUIRE(sobolGen != nullptr);

      CostDomain<float> cost_domain({0.0f}, {1.0f});
      randomGen->reset(cost_domain);
      gridGen->reset(cost_domain);
      sobolGen->reset(cost_domain);
      REQUIRE(randomGen->generatePoints(5).size() == 5);
      REQUIRE(gridGen->generatePoints(5).size() == 5);
      REQUIRE(sobolGen->generatePoints(5).size() == 5);
    }
  }

  TEST_CASE("RandomSearchOptimise with custom generator", "[RandomSearchOptimise]")
  {
    CostDomain<float> domain({0.0f, 0.0f}, {2.0f, 2.0f});

    SECTION("Using custom RandomSampleGenerator") {
      auto generator = std::make_shared<RandomSampleGenerator>(true, 42);
      RandomSearchOptimise optimizer(100, generator, 5, 0, false);

      auto result = optimizer.minimise(domain, quadratic);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("Custom generator: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      REQUIRE(std::abs(bestX[0] - 1.0f) < 0.5f);
      REQUIRE(std::abs(bestX[1] - 1.0f) < 0.5f);
      REQUIRE(best < 0.5f);
    }

    SECTION("Using custom GridSampleGenerator") {
      auto generator = std::make_shared<GridSampleGenerator>(8);
      RandomSearchOptimise optimizer(64, generator, 8, 0, false);

      auto result = optimizer.minimise(domain, quadratic);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("Custom grid generator: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      REQUIRE(std::abs(bestX[0] - 1.0f) < 0.5f);
      REQUIRE(std::abs(bestX[1] - 1.0f) < 0.5f);
      REQUIRE(best < 0.5f);
    }
  }

  TEST_CASE("RandomSearchOptimise with start point", "[RandomSearchOptimise]")
  {
    CostDomain<float> domain({-5.0f, -5.0f}, {5.0f, 5.0f});
    RandomSearchOptimise optimizer(100, RandomSearchOptimise::PatternType::Random, 5, 0, false, true, 42);

    SECTION("Valid start point") {
      VectorT<float> startPoint(2);
      startPoint << 0.8f, 0.9f; // Close to optimal (1,1)

      SPDLOG_INFO("Testing with valid start point [{}, {}]", startPoint[0], startPoint[1]);
      auto result = optimizer.minimise(domain, quadratic, startPoint);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("With start point: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      REQUIRE(best <= quadratic(startPoint)); // Should be at least as good as start point
      REQUIRE(std::abs(bestX[0] - 1.0f) < 1.0f);
      REQUIRE(std::abs(bestX[1] - 1.0f) < 1.0f);
    }

    SECTION("Out of bounds start point") {
      VectorT<float> startPoint(2);
      startPoint << 10.0f, 10.0f; // Outside domain bounds

      SPDLOG_INFO("Testing with out-of-bounds start point [{}, {}]", startPoint[0], startPoint[1]);
      auto result = optimizer.minimise(domain, quadratic, startPoint);
      // Should still work, just ignoring the invalid start point
      REQUIRE(std::get<1>(result) < 100.0f); // Should find something reasonable
    }

    SECTION("Wrong dimension start point") {
      VectorT<float> startPoint(3); // Wrong number of dimensions
      startPoint << 1.0f, 1.0f, 1.0f;

      SPDLOG_INFO("Testing with wrong dimension start point");
      REQUIRE_THROWS_AS(optimizer.minimise(domain, quadratic, startPoint), std::invalid_argument);
    }
  }

  TEST_CASE("RandomSearchOptimise batch optimization", "[RandomSearchOptimise]")
  {
    CostDomain<float> domain({0.0f, 0.0f}, {2.0f, 2.0f});
    RandomSearchOptimise optimizer(100, RandomSearchOptimise::PatternType::Random, 10, 0, false, true, 42);

    SPDLOG_INFO("Testing RandomSearchOptimise batch optimization");

    // Test batch method (which currently just calls single minimize)
    auto results = optimizer.minimiseBatch(domain, quadratic, {});

    REQUIRE(results.size() == 1); // Current implementation returns single result
    float best = std::get<1>(results[0]);
    VectorT<float> bestX = std::get<0>(results[0]);

    SPDLOG_INFO("Batch result: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
    REQUIRE(std::abs(bestX[0] - 1.0f) < 0.5f);
    REQUIRE(std::abs(bestX[1] - 1.0f) < 0.5f);
    REQUIRE(best < 0.5f);
  }

  TEST_CASE("RandomSearchOptimise parallel evaluation", "[RandomSearchOptimise]")
  {
    CostDomain<float> domain({-2.0f, -2.0f}, {2.0f, 2.0f});

    SECTION("Single-threaded vs multi-threaded") {
      // Single-threaded
      RandomSearchOptimise optimizer1(100, RandomSearchOptimise::PatternType::Random, 20, 0, false, true, 42);
      auto result1 = optimizer1.minimise(domain, sphere);

      // Multi-threaded
      RandomSearchOptimise optimizer2(100, RandomSearchOptimise::PatternType::Random, 20, 4, false, true, 42);
      auto result2 = optimizer2.minimise(domain, sphere);

      SPDLOG_INFO("Single-threaded result: {}", std::get<1>(result1));
      SPDLOG_INFO("Multi-threaded result: {}", std::get<1>(result2));

      // Both should find good solutions (not necessarily identical due to potential race conditions)
      REQUIRE(std::get<1>(result1) < 2.0f);
      REQUIRE(std::get<1>(result2) < 2.0f);
    }
  }

  TEST_CASE("RandomSearchOptimise configuration constructor", "[RandomSearchOptimise]")
  {
    std::string configJson = R"({
      "maxEvals": 50,
      "patternType": "Grid",
      "batchSize": 5,
      "verbose": false,
      "fixedSeed": true,
      "seed": 123,
      "sampleGenerator": {
        "_type": "Grid",
        "pointsPerDim": 8
      }
    })";

    SPDLOG_INFO("Testing Configuration constructor with custom parameters");

    Configuration config = Configuration::fromJSONString(configJson);
    RandomSearchOptimise optimizer(config);

    // Verify configuration was applied
    REQUIRE(optimizer.getMaxEvals() == 50);
    REQUIRE(optimizer.getPatternType() == RandomSearchOptimise::PatternType::Grid);
    REQUIRE(optimizer.getBatchSize() == 5);

    // Test optimization with configured parameters
    CostDomain<float> domain({-1.0f}, {3.0f});
    auto func1D = [](const VectorT<float> &x) -> float {
      return static_cast<float>(std::pow(x[0] - 1.0f, 2));
    };

    SPDLOG_INFO("Running 1D optimization with configured parameters");
    auto result = optimizer.minimise(domain, func1D);
    SPDLOG_INFO("1D optimization result: {} at {}", std::get<1>(result), std::get<0>(result)[0]);

    REQUIRE(std::abs(std::get<0>(result)[0] - 1.0f) < 0.5f);
    REQUIRE(std::get<1>(result) < 0.5f);
  }

  TEST_CASE("RandomSearchOptimise parameter validation", "[RandomSearchOptimise]")
  {
    SECTION("Invalid maxEvals") {
      REQUIRE_THROWS_AS(RandomSearchOptimise(0), std::invalid_argument);
    }

    SECTION("Invalid batchSize") {
      REQUIRE_THROWS_AS(RandomSearchOptimise(100, RandomSearchOptimise::PatternType::Random, 0),
                        std::invalid_argument);
    }

    SECTION("Invalid gridPointsPerDim") {
      REQUIRE_THROWS_AS(RandomSearchOptimise(100, RandomSearchOptimise::PatternType::Grid, 1, 0, false, true, 42, 1),
                        std::invalid_argument);
    }

    SECTION("Invalid pattern type in config") {
      std::string configJson = R"({
        "patternType": "InvalidPattern"
      })";

      Configuration config = Configuration::fromJSONString(configJson);
      REQUIRE_THROWS_AS(RandomSearchOptimise(config), std::invalid_argument);
    }

    SECTION("Null custom generator") {
      REQUIRE_THROWS_AS(RandomSearchOptimise(100, std::shared_ptr<SampleGenerator>(nullptr)), std::invalid_argument);
    }
  }

  TEST_CASE("RandomSearchOptimise different functions", "[RandomSearchOptimise]")
  {
    RandomSearchOptimise optimizer(200, RandomSearchOptimise::PatternType::Random, 10, 0, false, true, 42);

    SECTION("Rosenbrock function") {
      CostDomain<float> domain({-2.0f, -1.0f}, {2.0f, 3.0f});

      SPDLOG_INFO("Testing Rosenbrock function optimization");
      auto result = optimizer.minimise(domain, rosenbrock);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("Rosenbrock: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      // Rosenbrock is harder, so more lenient requirements
      REQUIRE(best < 100.0f); // Should find something better than random
    }

    SECTION("High-dimensional sphere") {
      CostDomain<float> domain({-3.0f, -3.0f, -3.0f, -3.0f, -3.0f}, {3.0f, 3.0f, 3.0f, 3.0f, 3.0f});

      SPDLOG_INFO("Testing 5D sphere function optimization");
      auto result = optimizer.minimise(domain, sphere);
      float best = std::get<1>(result);

      SPDLOG_INFO("5D Sphere: Best value: {}", best);
      REQUIRE(best < 20.0f); // Should find something reasonable in 5D
    }
  }

  TEST_CASE("RandomSearchOptimise reproducibility", "[RandomSearchOptimise]")
  {
    CostDomain<float> domain({0.0f, 0.0f}, {2.0f, 2.0f});

    SECTION("Fixed seed produces reproducible results") {
      RandomSearchOptimise optimizer1(100, RandomSearchOptimise::PatternType::Random, 10, 0, false, true, 12345);
      RandomSearchOptimise optimizer2(100, RandomSearchOptimise::PatternType::Random, 10, 0, false, true, 12345);

      auto result1 = optimizer1.minimise(domain, quadratic);
      auto result2 = optimizer2.minimise(domain, quadratic);

      SPDLOG_INFO("Reproducibility test: {} vs {}", std::get<1>(result1), std::get<1>(result2));

      // With same seed, should get identical results
      REQUIRE(std::abs(std::get<1>(result1) - std::get<1>(result2)) < 1e-6f);
      REQUIRE(std::abs(std::get<0>(result1)[0] - std::get<0>(result2)[0]) < 1e-6f);
      REQUIRE(std::abs(std::get<0>(result1)[1] - std::get<0>(result2)[1]) < 1e-6f);
    }

    SECTION("Different seeds produce different results") {
      RandomSearchOptimise optimizer1(100, RandomSearchOptimise::PatternType::Random, 10, 0, false, true, 111);
      RandomSearchOptimise optimizer2(100, RandomSearchOptimise::PatternType::Random, 10, 0, false, true, 222);

      auto result1 = optimizer1.minimise(domain, quadratic);
      auto result2 = optimizer2.minimise(domain, quadratic);

      SPDLOG_INFO("Different seeds test: {} vs {}", std::get<1>(result1), std::get<1>(result2));

      // With different seeds, results should likely be different (though both good)
      bool different = (std::abs(std::get<1>(result1) - std::get<1>(result2)) > 1e-6f) ||
                      (std::abs(std::get<0>(result1)[0] - std::get<0>(result2)[0]) > 1e-6f) ||
                      (std::abs(std::get<0>(result1)[1] - std::get<0>(result2)[1]) > 1e-6f);

      REQUIRE(different); // Should be different with high probability
    }
  }

  TEST_CASE("SampleGenerators Configuration constructors", "[SampleGenerators]")
  {
    SECTION("RandomSampleGenerator from config") {
      std::string configJson = R"({
        "fixedSeed": true,
        "seed": 12345
      })";

      Configuration config = Configuration::fromJSONString(configJson);
      RandomSampleGenerator generator(config);

      REQUIRE(generator.getFixedSeed() == true);
      REQUIRE(generator.getSeed() == 12345);

      CostDomain<float> domain({0.0f, 0.0f}, {1.0f, 1.0f});
      generator.reset(domain);
      auto points = generator.generatePoints(5);
      REQUIRE(points.size() == 5);
    }

    SECTION("GridSampleGenerator from config") {
      std::string configJson = R"({
        "pointsPerDim": 6
      })";

      Configuration config = Configuration::fromJSONString(configJson);
      GridSampleGenerator generator(config);

      REQUIRE(generator.getPointsPerDim() == 6);

      CostDomain<float> domain({0.0f, 0.0f}, {1.0f, 1.0f});
      generator.reset(domain);
      auto points = generator.generatePoints(36);
      REQUIRE(points.size() == 36);
    }

    SECTION("SobolSampleGenerator from config") {
      std::string configJson = R"({})";

      Configuration config = Configuration::fromJSONString(configJson);
      SobolSampleGenerator generator(config);

      CostDomain<float> domain({0.0f, 0.0f}, {1.0f, 1.0f});
      generator.reset(domain);
      auto points = generator.generatePoints(10);
      REQUIRE(points.size() == 10);
    }
  }

} // namespace Ravl2
