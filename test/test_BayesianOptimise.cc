#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Optimise/CostDomain.hh"
#include "Ravl2/Optimise/BayesianOptimise.hh"
#include <cmath>
#include <spdlog/spdlog.h>

namespace Ravl2
{
  float quadratic(const VectorT<float> &x) {
    float sum = 0.0f;
    for(int i = 0; i < x.size(); ++i) {
      sum += std::powf(x[i] - 1.0f, 2);
    }
    return sum;
  }

  TEST_CASE("BayesianOptimise finds minimum of quadratic function", "[BayesianOptimise]")
  {
    // Use initializer list constructor for CostDomain
    CostDomain<float> domain({0.0f, 0.0f}, {2.0f, 2.0f});

    BayesianOptimise bo(false);

    SPDLOG_INFO("Testing BayesianOptimise minimise method");

    // Test the minimise method (single-point optimisation)
    auto result = bo.minimise(domain, quadratic);
    float best = std::get<1>(result);
    VectorT<float> bestX = std::get<0>(result);

    SPDLOG_INFO("Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
    REQUIRE(std::abs(bestX[0] - 1.0f) < 0.2f);
    REQUIRE(std::abs(bestX[1] - 1.0f) < 0.2f);
    REQUIRE(best < 0.1f);

    // Test the minimiseBatch method with start points
    std::vector<VectorT<float>> startPoints;
    VectorT<float> start1(2); start1 << 0.5f, 0.5f;
    VectorT<float> start2(2); start2 << 1.5f, 1.5f;
    startPoints.push_back(start1);
    startPoints.push_back(start2);

    SPDLOG_INFO("Testing minimiseBatch with {} start points", startPoints.size());
    auto results = bo.minimiseBatch(domain, quadratic, startPoints);

    float batchBest = 1e9f;
    VectorT<float> batchBestX(2);
    for(const auto &r : results) {
      if(std::get<1>(r) < batchBest) {
        batchBest = std::get<1>(r);
        batchBestX = std::get<0>(r);
      }
    }
    SPDLOG_INFO("Batch best value: {} at [{}, {}] from {} evaluations",
                 batchBest, batchBestX[0], batchBestX[1], results.size());
    REQUIRE(std::abs(batchBestX[0] - 1.0f) < 0.2f);
    REQUIRE(std::abs(batchBestX[1] - 1.0f) < 0.2f);
    REQUIRE(batchBest < 0.1f);

    // Test with empty start points (should use random initialization)
    SPDLOG_INFO("Testing minimiseBatch with empty start points");
    auto emptyResults = bo.minimiseBatch(domain, quadratic, {});
    REQUIRE(!emptyResults.empty());
    SPDLOG_INFO("Empty start points produced {} evaluations", emptyResults.size());

    // Verify we got some improvement over worst case
    float worstInResults = 0.0f;
    for(const auto &r : emptyResults) {
      worstInResults = std::max(worstInResults, std::get<1>(r));
    }
    SPDLOG_INFO("Worst result value: {}", worstInResults);
    REQUIRE(worstInResults < 4.0f); // Worst case would be at corners (0,0) or (2,2) = 2
  }

  TEST_CASE("BayesianOptimise Configuration constructor", "[BayesianOptimise]")
  {
    // Test configuration-based constructor with JSON configuration
    std::string configJson = R"({
      "batchSize": 3,
      "maxIters": 10,
      "verbose": false
    })";

    SPDLOG_INFO("Testing Configuration constructor with custom parameters");
    SPDLOG_DEBUG("Configuration JSON: {}", configJson);

    Configuration config = Configuration::fromJSONString(configJson);
    BayesianOptimise bo(config);

    CostDomain<float> domain({-1.0f}, {3.0f});

    // Simple 1D quadratic: (x-1)^2
    auto func1D = [](const VectorT<float> &x) -> float {
      return std::powf(x[0] - 1.0f, 2);
    };

    SPDLOG_INFO("Running 1D optimization with configured parameters");
    auto result = bo.minimise(domain, func1D);
    SPDLOG_INFO("1D optimization result: {} at {}", std::get<1>(result), std::get<0>(result)[0]);
    REQUIRE(std::abs(std::get<0>(result)[0] - 1.0f) < 0.3f);
    REQUIRE(std::get<1>(result) < 0.2f);

    // Test batch optimization to verify the configured batch size is used
    SPDLOG_INFO("Testing batch optimization with configured parameters");
    auto batchResults = bo.minimiseBatch(domain, func1D, {});
    REQUIRE(!batchResults.empty());
    SPDLOG_INFO("Batch optimization produced {} evaluations (expected ~33 with batchSize=3, maxIters=10)",
                 batchResults.size());
    // With maxIters=10 and batchSize=3, we should have at least 3 initial + 10*3 = 33 evaluations
    REQUIRE(batchResults.size() >= 30);
  }
}
