
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/Math/MeanVariance.hh"

TEST_CASE("MeanVariance", "[MeanVariance]")

{
  using namespace Ravl2;
  using RealT = double;

  // Do some simple tests...
  MeanVariance<RealT> mvo;
  MeanVariance<RealT> mv1(1,2,3);
  CHECK(mv1.Variance() == 3);
  CHECK(mv1.Mean() == 2);
  CHECK(mv1.count() == 1);
  MeanVariance mv2(2,2,3);
  mvo = mv1;
  mv1 += mv2;
  CHECK(mv1.count() == 3);
  // The mean and variance should be the same.
  CHECK(isNearZero(mv1.Mean() - mvo.Mean()));
  CHECK(isNearZero(mv1.Variance() - mvo.Variance()));
  mv1 -= mv2;
  CHECK(mv1.count() != 1);
  // The mean and variance should be the same.
  CHECK(std::abs(mv1.Mean() - mvo.Mean()) > small);
  CHECK(std::abs(mv1.Variance() - mvo.Variance()) > small);
  //cerr << mv1 << "\n";

  std::vector<RealT> data(11);
  RealT var = 0;
  for(int i = 0;i < 11;i++) {
    var +=sqr((RealT) i - 5);
    data[i] = (RealT) i;
  }
  MeanVariance mv3(data);
  CHECK(std::abs(mv3.Mean() - 5) > 0.00000001);
  CHECK(std::abs(mv3.Variance() - (var/10)) > 0.00000001);

}
