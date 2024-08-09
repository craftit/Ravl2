
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/Math.hh"
#include "Ravl2/Math/MeanVariance.hh"
#include "Ravl2/Math/Sums1d2.hh"

TEST_CASE("MeanVariance", "[MeanVariance]")
{
  using namespace Ravl2;
  using RealT = double;

  // Do some simple tests...
  MeanVariance<RealT> mvo;
  MeanVariance<RealT> mv1(1,2,3);
  CHECK(mv1.variance() == 3);
  CHECK(mv1.mean() == 2);
  CHECK(mv1.count() == 1);
  MeanVariance<RealT> mv2(2,2,3);
  mvo = mv1;
  mv1 += mv2;
  CHECK(mv1.count() == 3);
  // The mean and variance should be the same.
  CHECK(isNearZero(mv1.mean() - mvo.mean()));
  CHECK(isNearZero(mv1.variance() - mvo.variance()));
  mv1 -= mv2;
  CHECK(mv1.count() == 1);
  // The mean and variance should be the same.
  CHECK(isNearZero(mv1.mean() - mvo.mean(),1e-6));
  CHECK(isNearZero(mv1.variance() - mvo.variance()));
  //cerr << mv1 << "\n";

  std::vector<RealT> data(11);
  RealT var = 0;
  for(unsigned i = 0;i < 11;i++) {
    var +=sqr(RealT(i) - 5);
    data[i] = RealT(i);
  }
  MeanVariance<RealT> mv3 = computeMeanVariance(data);
  CHECK(isNearZero(mv3.mean() - 5));
  CHECK(isNearZero(mv3.variance() - (var/10)));

}

TEST_CASE("Sums1d2C", "[Sums1d2C]")
{
  using RealT = double;
  using namespace Ravl2;

  {
    auto sum = Sums1d2C<RealT>::fromMeanVariance(10,5.0,2.0);
    CHECK(isNearZero(sum.mean()-5));
    CHECK(isNearZero(sum.variance()-2));
  }


  {
    // Check sums work ok.
    Sums1d2C<RealT> sum;
    MeanVariance<RealT> mvinc;
    MeanVariance<RealT> mva;
    MeanVariance<RealT> mvb;
    std::vector<RealT> data;
    for(int i = 0;i < 11;i++) {
      RealT val = sqr(RealT(i)) - 10;
      sum += val;
      mvinc += val;
      if(i % 1) {
        mva += val;
      } else {
        mvb += val;
      }
      data.push_back(val);

      MeanVariance<RealT> added = mva;
      added += mvb;
      MeanVariance<RealT> comp = computeMeanVariance(data,false);
#if 0
      SPDLOG_INFO("Val:{} Sum: {} {}  added:{} {}  inc:{} {} comp:{} {}",
          val,
          sum.mean(),
          sum.variance(false),
          added.mean(),
          added.variance(),
          mvinc.mean(),
          mvinc.variance(),
          comp.mean(),
          comp.variance()
          );
#endif

      CHECK(isNearZero(sum.mean()-added.mean(),1e-6));
      CHECK(isNearZero(sum.variance(false)-added.variance(),1e-6));
      CHECK(isNearZero(sum.mean()-mvinc.mean(),1e-6));
      CHECK(isNearZero(sum.variance(false)-mvinc.variance(),1e-6));
      CHECK(isNearZero(sum.mean()-comp.mean(),1e-6));
      CHECK(isNearZero(sum.variance(false)-comp.variance(),1e-6));

    }
  }
}

