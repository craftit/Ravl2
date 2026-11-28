
#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Optimise/OptimiseBrent.hh"
#include "Ravl2/Optimise/OptimisePowell.hh"
#include "Ravl2/Optimise/BracketMinimum.hh"
#include <random>

namespace Ravl2
{
  TEST_CASE("Optimise")
  {
    using RealT = float;

    SECTION("Powell")
    {

      auto func = [](const VectorT<RealT> &X) -> RealT
      {
        return X[0] * X[0] + X[1] * X[1];
      };

      OptimisePowell powell (100, OptimisePowell::RealT(1e-6),true,false);
      ;

      auto [sol,cost] = powell.minimise(CostDomain(VectorT<RealT>({{-1.0},{-1.0}}),VectorT<RealT>({{1.0},{1.0}})),func,VectorT<RealT>({{-1.0},{-1.0}}));
      //SPDLOG_INFO("Powell: X {} cost {}", sol, cost);
      CHECK(isNearZero(sol[0]));
      CHECK(isNearZero(sol[1]));
      CHECK(isNearZero(cost));
    }

    SECTION("Powell converges on a rotated valley")
    {
      // Narrow valley along x = y, which coordinate descent alone handles badly.
      auto valley = [](const VectorT<RealT> &X) -> RealT {
        const RealT u = X[0] + X[1] - 1;
        const RealT v = X[0] - X[1];
        return u * u + 100 * v * v;
      };
      OptimisePowell powell(3, RealT(1e-12), true, false);
      const CostDomain<RealT> domain(VectorT<RealT>({{-5.0}, {-5.0}}), VectorT<RealT>({{5.0}, {5.0}}));
      auto [sol, cost] = powell.minimise(domain, valley, VectorT<RealT>({{-3.0}, {4.0}}));
      CHECK(cost < 1e-4f);
      CHECK(std::abs(sol[0] - 0.5f) < 1e-2f);
      CHECK(std::abs(sol[1] - 0.5f) < 1e-2f);
    }

    SECTION("Powell keeps to its domain")
    {
      const CostDomain<RealT> domain(VectorT<RealT>({{-1.0}, {-1.0}}), VectorT<RealT>({{1.0}, {1.0}}));
      OptimisePowell powell(100, RealT(1e-6), true, false);
      // The unconstrained minimum (3, -0.5) is outside the box: the answer is on the bound.
      auto boundActive = [](const VectorT<RealT> &X) -> RealT {
        return (X[0] - 3) * (X[0] - 3) + (X[1] + 0.5f) * (X[1] + 0.5f);
      };
      auto [sol, cost] = powell.minimise(domain, boundActive, VectorT<RealT>({{0.0}, {0.0}}));
      CHECK(domain.isInDomain(sol));
      CHECK(std::abs(sol[0] - 1.0f) < 1e-3f);
      CHECK(std::abs(sol[1] + 0.5f) < 1e-3f);
      CHECK(std::abs(cost - boundActive(sol)) < 1e-5f);

      // The minimum needs a step in the negative direction from the start.
      auto behind = [](const VectorT<RealT> &X) -> RealT {
        return (X[0] + 0.5f) * (X[0] + 0.5f) + 2 * (X[1] + 0.25f) * (X[1] + 0.25f);
      };
      auto [sol2, cost2] = powell.minimise(domain, behind, VectorT<RealT>({{0.5}, {0.5}}));
      CHECK(std::abs(sol2[0] + 0.5f) < 1e-3f);
      CHECK(std::abs(sol2[1] + 0.25f) < 1e-3f);

      // An empty start means the domain's midpoint.
      auto [sol3, cost3] = powell.minimise(domain, behind, VectorT<RealT>());
      CHECK(std::abs(sol3[0] + 0.5f) < 1e-3f);
      (void)cost2;
      (void)cost3;

      // Random rotated quadratics, minimum inside or outside, start anywhere: every result is in
      // the domain and its cost is the function's value there.
      std::mt19937 rng(11);
      std::uniform_real_distribution<RealT> u(-1, 1);
      for(int trial = 0; trial < 200; ++trial) {
        const RealT cx = 3 * u(rng), cy = 3 * u(rng), a = u(rng), sx = 1 + 4 * (u(rng) + 1), sy = 1 + 4 * (u(rng) + 1);
        auto f = [=](const VectorT<RealT> &X) -> RealT {
          const RealT dx = X[0] - cx, dy = X[1] - cy;
          const RealT p = std::cos(a) * dx + std::sin(a) * dy, q = -std::sin(a) * dx + std::cos(a) * dy;
          return sx * p * p + sy * q * q;
        };
        auto [s, c] = powell.minimise(domain, f, VectorT<RealT>({{u(rng)}, {u(rng)}}));
        REQUIRE(domain.isInDomain(s));
        CHECK(std::abs(c - f(s)) <= 1e-4f * (1 + std::abs(c)));
      }
    }

    SECTION("bracketMinimum stays within its limits")
    {
      std::function<RealT(RealT)> f = [](RealT x) { return (x - 10) * (x - 10); };// minimum beyond hi
      const auto br = bracketMinimum<RealT>(-1, 2, f, 0, f(0));
      CHECK(br.a >= -1);
      CHECK(br.b <= 2);
      CHECK(br.x == 2);
      std::function<RealT(RealT)> g = [](RealT x) { return (x + 0.3f) * (x + 0.3f); };// minimum behind the start
      const auto br2 = bracketMinimum<RealT>(-1, 1, g, 0, g(0));
      CHECK(br2.a <= -0.3f);
      CHECK(br2.b >= -0.3f);
      CHECK(br2.a >= -1);
      CHECK(br2.fx <= g(0));
    }
  }
}
