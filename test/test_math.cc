
#include <catch2/catch_test_macros.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include "Ravl2/Math.hh"
#include "Ravl2/Math/MeanVariance.hh"
#include "Ravl2/Math/FastFourierTransform.hh"
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

TEST_CASE("Sums1_2")
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

TEST_CASE("FastFourierTransform")
{
  using namespace Ravl2;
  using RealT = float;
  size_t arraySize = 10;

  std::vector<RealT> data(arraySize);
  std::vector<std::complex<RealT>> cdata(data.size());
  for(unsigned i = 0;i < data.size();i++) {
    data[i] = RealT(i);
    cdata[i] = std::complex<RealT>(RealT(i),0);
  }

  std::vector<std::complex<RealT>> result1(data.size(),std::complex<RealT>(-1,0));
  computeFFT(std::span(result1),std::span<const RealT>(data));

  std::vector<std::complex<RealT>> result2(data.size(),std::complex<RealT>(-1,0));
  computeFFT(std::span(result2),std::span<const std::complex<RealT> >(cdata));

  // Check the results are the same from the real and complex FFT.
  for(unsigned i = 0;i < data.size();i++) {
    CHECK(isNearZero(result1[i].real() - result2[i].real() ));
    CHECK(isNearZero(result1[i].imag() - result2[i].imag() ));
  }


  // Check the inverse FFT returns the original data.
  std::vector<std::complex<RealT>> result3(data.size(),std::complex<RealT>(-1,0));
  computeInverseFFT(std::span(result3),std::span<const std::complex<RealT> >(result1));
  for(unsigned i = 0;i < data.size();i++) {
    CHECK(isNearZero(result3[i].real() - data[i],1e-5f));
    CHECK(isNearZero(result3[i].imag(),1e-5f));
  }

  // Check the real inverse FFT is the same as the complex inverse FFT.
  std::fill(result1.begin(),result1.end(),std::complex<RealT>(0,0));
  computeInverseFFT(std::span(result1),std::span<const std::complex<RealT> >(cdata));
  std::fill(result2.begin(),result2.end(),std::complex<RealT>(0,0));
  computeInverseFFT(std::span(result2),std::span<const RealT >(data));
  for(unsigned i = 0;i < data.size();i++) {
    CHECK(isNearZero(result2[i].real() - result1[i].real()));
    CHECK(isNearZero(result2[i].imag() - result1[i].imag()));
  }
}
