#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Optimise/CostDomain.hh"
#include "Ravl2/Optimise/ParticleSwarmOptimise.hh"
#include <cmath>
#include <spdlog/spdlog.h>
#include <thread>

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

    // Rastrigin function - a more complex multimodal function with many local minima
    float rastrigin(const VectorT<float> &x) {
      float sum = 10.0f * static_cast<float>(x.size());
      for(int i = 0; i < x.size(); ++i) {
        sum += x[i] * x[i] - 10.0f * std::cos(static_cast<float>(2.0 * M_PI) * x[static_cast<Eigen::Index>(i)]);
      }
      return sum;
    }
  }

  TEST_CASE("ParticleSwarmOptimise finds minimum of quadratic function", "[ParticleSwarmOptimise]")
  {
    CostDomain<float> domain({-2.0f, -2.0f}, {2.0f, 2.0f});

    SECTION("Single-threaded PSO") {
      // Configure PSO with 30 particles, 100 iterations, single-threaded
      ParticleSwarmOptimise optimizer(100, 30, 0.7f, 1.5f, 1.5f, 0, true);

      SPDLOG_INFO("Testing ParticleSwarmOptimise in single-threaded mode");
      auto result = optimizer.minimise(domain, quadratic);
      float best = std::get<1>(result);
      VectorT<float> bestX = std::get<0>(result);

      SPDLOG_INFO("PSO-ST: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
      REQUIRE(std::abs(bestX[0] - 1.0f) < 0.1f);
      REQUIRE(std::abs(bestX[1] - 1.0f) < 0.1f);
      REQUIRE(best < 0.1f);
    }

    SECTION("Multi-threaded PSO") {
      // Only run multi-threaded test if hardware supports it
      if (std::thread::hardware_concurrency() > 1) {
        // Configure PSO with 30 particles, 100 iterations, multi-threaded (4 threads)
        ParticleSwarmOptimise optimizer(100, 30, 0.7f, 1.5f, 1.5f, 4, true);

        SPDLOG_INFO("Testing ParticleSwarmOptimise in multi-threaded mode");
        auto result = optimizer.minimise(domain, quadratic);
        float best = std::get<1>(result);
        VectorT<float> bestX = std::get<0>(result);

        SPDLOG_INFO("PSO-MT: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
        REQUIRE(std::abs(bestX[0] - 1.0f) < 0.1f);
        REQUIRE(std::abs(bestX[1] - 1.0f) < 0.1f);
        REQUIRE(best < 0.1f);
      } else {
        SPDLOG_INFO("Skipping multi-threaded test as hardware doesn't support it");
      }
    }
  }

  TEST_CASE("ParticleSwarmOptimise finds minimum of Rosenbrock function", "[ParticleSwarmOptimise]")
  {
    CostDomain<float> domain({-5.0f, -5.0f}, {5.0f, 5.0f});

    // Rosenbrock function is more challenging, so use more particles and iterations
    ParticleSwarmOptimise optimizer(150, 40, 0.7f, 1.5f, 1.5f, 0, true);

    SPDLOG_INFO("Testing ParticleSwarmOptimise with Rosenbrock function");
    auto result = optimizer.minimise(domain, rosenbrock);
    float best = std::get<1>(result);
    VectorT<float> bestX = std::get<0>(result);

    SPDLOG_INFO("PSO-Rosenbrock: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
    // Rosenbrock minimum is at (1,1)
    REQUIRE(std::abs(bestX[0] - 1.0f) < 0.2f);
    REQUIRE(std::abs(bestX[1] - 1.0f) < 0.2f);
    REQUIRE(best < 1.0f);
  }

  TEST_CASE("ParticleSwarmOptimise finds minimum of Rastrigin function", "[ParticleSwarmOptimise]")
  {
    // Rastrigin function domain typically between -5.12 and 5.12
    CostDomain<float> domain({-5.12f, -5.12f}, {5.12f, 5.12f});

    // Rastrigin is multimodal, so use more particles and iterations
    ParticleSwarmOptimise optimizer(200, 50, 0.7f, 1.5f, 1.5f, 0, true);

    SPDLOG_INFO("Testing ParticleSwarmOptimise with Rastrigin function");
    auto result = optimizer.minimise(domain, rastrigin);
    float best = std::get<1>(result);
    VectorT<float> bestX = std::get<0>(result);

    SPDLOG_INFO("PSO-Rastrigin: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
    // Rastrigin minimum is at (0,0)
    REQUIRE(std::abs(bestX[0]) < 0.5f);
    REQUIRE(std::abs(bestX[1]) < 0.5f);
    REQUIRE(best < 2.0f);
  }

  TEST_CASE("ParticleSwarmOptimise with known starting point", "[ParticleSwarmOptimise]")
  {
    CostDomain<float> domain({-2.0f, -2.0f}, {2.0f, 2.0f});

    ParticleSwarmOptimise optimizer(100, 30, 0.7f, 1.5f, 1.5f, 0, true);

    // Create a starting point
    VectorT<float> start(2);
    start.fill(0.5f); // Start at (0.5, 0.5)

    SPDLOG_INFO("Testing ParticleSwarmOptimise with starting point [{}, {}]", start[0], start[1]);
    auto result = optimizer.minimise(domain, quadratic, start);
    float best = std::get<1>(result);
    VectorT<float> bestX = std::get<0>(result);

    SPDLOG_INFO("PSO-Start: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
    REQUIRE(std::abs(bestX[0] - 1.0f) < 0.1f);
    REQUIRE(std::abs(bestX[1] - 1.0f) < 0.1f);
    REQUIRE(best < 0.1f);
  }

  TEST_CASE("ParticleSwarmOptimise with configuration", "[ParticleSwarmOptimise]")
  {
    // Create a configuration object
    std::string configJson = R"({
      "maxIters": 100,
      "swarmSize": 30,
      "inertiaWeight": 0.7,
      "cognitiveWeight": 1.5,
      "socialWeight": 1.5,
      "maxThreads": 0,
      "verbose": true,
      "fixedSeed": true,
      "seed": 42,
      "tolerance": 1e-6,
      "minItersBeforeConvergence": 10
      })";

    Configuration config = Configuration::fromJSONString(configJson);
    ParticleSwarmOptimise optimizer(config);

    CostDomain<float> domain({-2.0f, -2.0f}, {2.0f, 2.0f});

    SPDLOG_INFO("Testing ParticleSwarmOptimise with configuration");
    auto result = optimizer.minimise(domain, quadratic);
    float best = std::get<1>(result);
    VectorT<float> bestX = std::get<0>(result);

    SPDLOG_INFO("PSO-Config: Best value: {} at [{}, {}]", best, bestX[0], bestX[1]);
    REQUIRE(std::abs(bestX[0] - 1.0f) < 0.1f);
    REQUIRE(std::abs(bestX[1] - 1.0f) < 0.1f);
    REQUIRE(best < 0.1f);
  }

  TEST_CASE("ParticleSwarmOptimise in higher dimensions", "[ParticleSwarmOptimise]")
  {
    // Test in 5 dimensions
    const int dim = 5;

    // Create domain for 5D
    CostDomain<float> domain({-2.0f, -2.0f, -2.0f, -2.0f, -2.0f},
                             {2.0f, 2.0f, 2.0f, 2.0f, 2.0f});

    ParticleSwarmOptimise optimizer(150, 40, 0.7f, 1.5f, 1.5f, 0, true);

    SPDLOG_INFO("Testing ParticleSwarmOptimise in {} dimensions", dim);
    auto result = optimizer.minimise(domain, quadratic);
    float best = std::get<1>(result);
    VectorT<float> bestX = std::get<0>(result);

    SPDLOG_INFO("PSO-{}D: Best value: {}", dim, best);
    for (int i = 0; i < dim; i++) {
      SPDLOG_INFO("x[{}] = {}", i, bestX[i]);
      REQUIRE(std::abs(bestX[i] - 1.0f) < 0.2f);
    }
    REQUIRE(best < 0.5f);
  }
}
