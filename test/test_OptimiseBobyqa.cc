#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <stdexcept>
#include "Ravl2/Optimise/CostDomain.hh"
#include "Ravl2/Optimise/OptimiseBobyqa.hh"

namespace Ravl2
{
  namespace
  {
    using RealT = OptimiseBobyqa::RealT;

    VectorT<RealT> vec2(RealT a, RealT b)
    {
      VectorT<RealT> v(2);
      v << a, b;
      return v;
    }

    // Separable quadratic with its minimum at (0.3, -0.7, 1.2).
    RealT separableQuadratic(const VectorT<RealT> &x)
    {
      const RealT a = x[0] - 0.3f;
      const RealT b = x[1] + 0.7f;
      const RealT c = x[2] - 1.2f;
      return a * a + 2.0f * b * b + 0.5f * c * c;
    }

    RealT rosenbrock(const VectorT<RealT> &x)
    {
      const RealT a = 1.0f - x[0];
      const RealT b = x[1] - x[0] * x[0];
      return a * a + 100.0f * b * b;
    }
  }// namespace

  TEST_CASE("OptimiseBobyqa", "[Optimise]")
  {
    SECTION("Separable quadratic, minimum inside the box")
    {
      OptimiseBobyqa bobyqa(500, 0.2, 1e-6);
      const CostDomain<RealT> domain({-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f});
      auto result = bobyqa.minimiseDetailed(domain, separableQuadratic, VectorT<RealT>());
      REQUIRE(result.status == BobyqaStatus::Converged);
      REQUIRE(std::abs(result.x[0] - 0.3f) < 1e-4f);
      REQUIRE(std::abs(result.x[1] + 0.7f) < 1e-4f);
      REQUIRE(std::abs(result.x[2] - 1.2f) < 1e-4f);
      REQUIRE(result.cost < 1e-7f);
      REQUIRE(result.evaluations < 100);

      // The tuple interface gives the same answer.
      auto [x, cost] = bobyqa.minimise(domain, separableQuadratic, VectorT<RealT>());
      REQUIRE(x == result.x);
      REQUIRE(cost == result.cost);
    }

    SECTION("Unconstrained minimum outside the box: bound-active solution")
    {
      // Minimum of (x-3)^2 + (y+0.5)^2 is at (3,-0.5); with x <= 1 the answer is (1,-0.5).
      auto func = [](const VectorT<RealT> &x) -> RealT {
        return (x[0] - 3.0f) * (x[0] - 3.0f) + (x[1] + 0.5f) * (x[1] + 0.5f);
      };
      // rhoEnd 1e-4: near the minimum (cost 4) a float cost cannot resolve smaller steps.
      OptimiseBobyqa bobyqa(500, 0.1, 1e-4);
      const CostDomain<RealT> domain({-1.0f, -1.0f}, {1.0f, 1.0f});
      auto result = bobyqa.minimiseDetailed(domain, func, vec2(0.0f, 0.0f));
      REQUIRE(result.status == BobyqaStatus::Converged);
      REQUIRE(result.x[0] == 1.0f);// Exactly on the bound.
      REQUIRE(std::abs(result.x[1] + 0.5f) < 1e-4f);
      REQUIRE(std::abs(result.cost - 4.0f) < 1e-5f);
    }

    SECTION("Rosenbrock")
    {
      OptimiseBobyqa bobyqa(2000, 0.5, 1e-6);
      const CostDomain<RealT> domain({-2.0f, -2.0f}, {2.0f, 2.0f});
      auto result = bobyqa.minimiseDetailed(domain, rosenbrock, vec2(-1.2f, 1.0f));
      REQUIRE(result.status == BobyqaStatus::Converged);
      REQUIRE(std::abs(result.x[0] - 1.0f) < 1e-3f);
      REQUIRE(std::abs(result.x[1] - 1.0f) < 2e-3f);
      REQUIRE(result.cost < 1e-6f);
    }

    SECTION("Invalid parameters throw")
    {
      const CostDomain<RealT> domain({-1.0f, -1.0f}, {1.0f, 1.0f});
      // n = 2 needs 4 <= npt <= 6.
      REQUIRE_THROWS_AS(OptimiseBobyqa(100, 0.1, 1e-4, 3).minimise(domain, rosenbrock, VectorT<RealT>()), std::invalid_argument);
      REQUIRE_THROWS_AS(OptimiseBobyqa(100, 0.1, 1e-4, 7).minimise(domain, rosenbrock, VectorT<RealT>()), std::invalid_argument);
      REQUIRE_NOTHROW(OptimiseBobyqa(100, 0.1, 1e-4, 6).minimise(domain, rosenbrock, VectorT<RealT>()));
      // The domain must be at least 2*rhoBegin wide in every dimension.
      const CostDomain<RealT> narrow({-1.0f, 0.0f}, {1.0f, 0.15f});
      REQUIRE_THROWS_AS(OptimiseBobyqa(100, 0.1, 1e-4).minimise(narrow, rosenbrock, VectorT<RealT>()), std::invalid_argument);
      // Start outside the domain, a single variable, and rhoEnd > rhoBegin.
      REQUIRE_THROWS_AS(OptimiseBobyqa(100, 0.1, 1e-4).minimise(domain, rosenbrock, vec2(2.0f, 0.0f)), std::invalid_argument);
      const CostDomain<RealT> oneD({-1.0f}, {1.0f});
      auto f1 = [](const VectorT<RealT> &x) -> RealT { return x[0] * x[0]; };
      REQUIRE_THROWS_AS(OptimiseBobyqa(100, 0.1, 1e-4).minimise(oneD, f1, VectorT<RealT>()), std::invalid_argument);
      REQUIRE_THROWS_AS(OptimiseBobyqa(100, 0.1, 0.2), std::invalid_argument);
    }

    SECTION("Running out of evaluations returns the best point so far")
    {
      size_t calls = 0;
      RealT bestSeen = std::numeric_limits<RealT>::max();
      auto func = [&calls, &bestSeen](const VectorT<RealT> &x) -> RealT {
        ++calls;
        const RealT f = rosenbrock(x);
        bestSeen = std::min(bestSeen, f);
        return f;
      };
      const CostDomain<RealT> domain({-2.0f, -2.0f}, {2.0f, 2.0f});
      // Fewer evaluations than interpolation points, and a budget that ends mid-iteration.
      for(size_t budget : {3u, 20u}) {
        calls = 0;
        bestSeen = std::numeric_limits<RealT>::max();
        OptimiseBobyqa bobyqa(budget, 0.5, 1e-6);
        BobyqaResult<RealT> result;
        REQUIRE_NOTHROW(result = bobyqa.minimiseDetailed(domain, func, vec2(-1.2f, 1.0f)));
        REQUIRE(result.status == BobyqaStatus::MaxEvaluations);
        REQUIRE(result.evaluations == budget);
        REQUIRE(calls == budget);
        REQUIRE(std::isfinite(result.cost));
        REQUIRE(result.cost == bestSeen);
        REQUIRE(rosenbrock(result.x) == result.cost);
      }
    }

    SECTION("Non-finite cost throws")
    {
      auto func = [](const VectorT<RealT> &x) -> RealT { return x[0] > 0.5f ? std::numeric_limits<RealT>::quiet_NaN() : x[0] * x[0] + x[1] * x[1]; };
      const CostDomain<RealT> domain({-1.0f, -1.0f}, {1.0f, 1.0f});
      REQUIRE_THROWS_AS(OptimiseBobyqa(100, 0.6, 1e-4).minimise(domain, func, vec2(0.0f, 0.0f)), std::runtime_error);
    }

    SECTION("Construct from configuration")
    {
      Configuration config = Configuration::fromJSONString(R"({"maxEvaluations": 50, "rhoBegin": 0.25, "rhoEnd": 0.001, "numInterpolationPoints": 5})");
      OptimiseBobyqa bobyqa(config);
      REQUIRE(bobyqa.maxEvaluations() == 50);
      REQUIRE(bobyqa.rhoBegin() == 0.25);
      REQUIRE(bobyqa.rhoEnd() == 0.001);
      REQUIRE(bobyqa.numInterpolationPoints() == 5);
    }
  }
}// namespace Ravl2
