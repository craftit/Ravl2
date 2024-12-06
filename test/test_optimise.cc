
#include "catch2checks.hh"
#include "Ravl2/Optimise/OptimiseBrent.hh"
#include "Ravl2/Optimise/OptimisePowell.hh"
#include "Ravl2/Optimise/BracketMinimum.hh"

TEST_CASE("Optimise")
{
  using namespace Ravl2;
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

}
