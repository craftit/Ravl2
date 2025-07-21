#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Optimise/CostDomain.hh"
#include "Ravl2/Optimise/RandomSearchOptimise.hh"
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
      RandomSearchOptimise optimizer(200, RandomSearchOptimise::PatternType::Grid, 10, 0, false, true, 42);

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
      "gridPointsPerDim": 8
    })";

    SPDLOG_INFO("Testing Configuration constructor with custom parameters");

    Configuration config = Configuration::fromJSONString(configJson);
    RandomSearchOptimise optimizer(config);

    // Verify configuration was applied
    REQUIRE(optimizer.getMaxEvals() == 50);
    REQUIRE(optimizer.getPatternType() == RandomSearchOptimise::PatternType::Grid);
    REQUIRE(optimizer.getBatchSize() == 5);
    REQUIRE(optimizer.getFixedSeed() == true);
    REQUIRE(optimizer.getSeed() == 123);
    REQUIRE(optimizer.getGridPointsPerDim() == 8);

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
      REQUIRE(best < 10.0f); // Should find something reasonably close to origin
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

} // namespace Ravl2
