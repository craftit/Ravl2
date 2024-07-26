
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
  CHECK(mv1.variance() == 3);
  CHECK(mv1.Mean() == 2);
  CHECK(mv1.count() == 1);
  MeanVariance<RealT> mv2(2,2,3);
  mvo = mv1;
  mv1 += mv2;
  CHECK(mv1.count() == 3);
  // The mean and variance should be the same.
  CHECK(isNearZero(mv1.Mean() - mvo.Mean()));
  CHECK(isNearZero(mv1.variance() - mvo.variance()));
  mv1 -= mv2;
  CHECK(mv1.count() == 1);
  // The mean and variance should be the same.
  CHECK(isNearZero(mv1.Mean() - mvo.Mean()));
  CHECK(isNearZero(mv1.variance() - mvo.variance()));
  //cerr << mv1 << "\n";

  std::vector<RealT> data(11);
  RealT var = 0;
  for(unsigned i = 0;i < 11;i++) {
    var +=sqr(RealT(i) - 5);
    data[i] = RealT(i);
  }
  MeanVariance<RealT> mv3 = computeMeanVariance(data);
  CHECK(isNearZero(mv3.Mean() - 5));
  CHECK(isNearZero(mv3.variance() - (var/10)));

}
