
#include <catch2/catch_test_macros.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include "Ravl2/Math.hh"
#include "Ravl2/Math/MeanVariance.hh"
#include "Ravl2/Math/FastFourierTransform.hh"
#include "Ravl2/Math/Sums1d2.hh"
#include "Ravl2/Math/PrimeFactors.hh"
#include <array>
#include <complex>
#include <span>
#include <vector>

namespace Ravl2
{
  TEST_CASE("MeanVariance")
  {
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
    MeanVariance<RealT> mv3 = computeMeanVariance(data, SampleStatisticsT::SAMPLE);
    CHECK(isNearZero(mv3.mean() - 5));
    CHECK(isNearZero(mv3.variance() - (var/10)));

  }

  TEST_CASE("MathFuncs")
  {
    using RealT = double;

    CHECK(isNearZero(deg2rad(180.0) - std::numbers::pi_v<RealT>));
    CHECK(isNearZero(rad2deg(std::numbers::pi_v<RealT>) - 180.0));
    CHECK(isNearZero(sign(1.0) - 1.0));
    CHECK(isNearZero(sign(-1.0) + 1.0));
    CHECK(isNearZero(sqr(2.0) - 4.0));
    CHECK(isPow2(2));
    CHECK_FALSE(isPow2(5));
    CHECK(isPow2(8));
    CHECK_FALSE(isPow2(9));
    CHECK(isPow2(16));
  }

  TEST_CASE("Sums1_2")
  {
    using RealT = double;

    {
      auto sum = Sums1d2C<RealT>::fromMeanVariance(10,5.0,2.0,SampleStatisticsT::POPULATION);
      CHECK(isNearZero(sum.mean()-5));
      CHECK(isNearZero(sum.variance(SampleStatisticsT::POPULATION)-2));
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
        if(i % 2) {
          mva += val;
        } else {
          mvb += val;
        }
        data.push_back(val);

        MeanVariance<RealT> added = mva;
        added += mvb;
        MeanVariance<RealT> comp = computeMeanVariance(data,SampleStatisticsT::POPULATION);
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
        CHECK(isNearZero(sum.variance(SampleStatisticsT::POPULATION)-added.variance(),1e-6));
        CHECK(isNearZero(sum.mean()-mvinc.mean(),1e-6));
        CHECK(isNearZero(sum.variance(SampleStatisticsT::POPULATION)-mvinc.variance(),1e-6));
        CHECK(isNearZero(sum.mean()-comp.mean(),1e-6));
        CHECK(isNearZero(sum.variance(SampleStatisticsT::POPULATION)-comp.variance(),1e-6));

      }
    }
  }

  TEST_CASE("FastFourierTransform")
  {
    using RealT = float;
    size_t arraySize = 10;

    std::vector<RealT> data(arraySize);
    std::vector<std::complex<RealT>> cdata(data.size());
    for(unsigned i = 0;i < data.size();i++) {
      data[i] = RealT(i);
      cdata[i] = std::complex<RealT>(RealT(i),0);
    }

    std::vector<std::complex<RealT>> result1(data.size(),std::complex<RealT>(-1,0));
    fastFourierTransform(std::span(result1), std::span<const RealT>(data));

    std::vector<std::complex<RealT>> result2(data.size(),std::complex<RealT>(-1,0));
    fastFourierTransform(std::span(result2), std::span<const std::complex<RealT>>(cdata));

    // Check the results are the same from the real and complex FFT.
    for(unsigned i = 0;i < data.size();i++) {
      CHECK(isNearZero(result1[i].real() - result2[i].real() ));
      CHECK(isNearZero(result1[i].imag() - result2[i].imag() ));
    }


    // Check the inverse FFT returns the original data.
    std::vector<std::complex<RealT>> result3(data.size(),std::complex<RealT>(-1,0));
    inverseFastFourierTransform(std::span(result3), std::span<const std::complex<RealT>>(result1));
    for(unsigned i = 0;i < data.size();i++) {
      CHECK(isNearZero(result3[i].real() - data[i],1e-5f));
      CHECK(isNearZero(result3[i].imag(),1e-5f));
    }

    // Check the real inverse FFT is the same as the complex inverse FFT.
    std::fill(result1.begin(),result1.end(),std::complex<RealT>(0,0));
    inverseFastFourierTransform(std::span(result1), std::span<const std::complex<RealT>>(cdata));
    std::fill(result2.begin(),result2.end(),std::complex<RealT>(0,0));
    inverseFastFourierTransform(std::span(result2), std::span<const RealT>(data));
    for(unsigned i = 0;i < data.size();i++) {
      CHECK(isNearZero(result2[i].real() - result1[i].real()));
      CHECK(isNearZero(result2[i].imag() - result1[i].imag()));
    }
  }

  TEST_CASE("MeanVariance merge and remove groups with different means")
  {
    // {0,0} merged with {2,2}: mean 1, population variance 1. The mean term used to be unsquared.
    MeanVariance<double> a(2, 0.0, 0.0);
    MeanVariance<double> b(2, 2.0, 0.0);
    MeanVariance<double> ab = a;
    ab += b;
    CHECK(isNearZero(ab.mean() - 1.0));
    CHECK(isNearZero(ab.variance() - 1.0));
    MeanVariance<double> ba = b;
    ba += a;
    CHECK(isNearZero(ba.variance() - 1.0));

    // Removing b again gives back a.
    ab -= b;
    CHECK(ab.count() == 2);
    CHECK(isNearZero(ab.mean()));
    CHECK(isNearZero(ab.variance()));
  }

  TEST_CASE("Sums1d2 subtraction reduces the count")
  {
    Sums1d2C<double> all;
    for(double v : {1.0, 2.0, 3.0})
      all += v;
    Sums1d2C<double> three;
    three += 3.0;
    all -= three;
    CHECK(all.size() == 2);
    CHECK(isNearZero(all.mean() - 1.5));
  }

  TEST_CASE("FFT of length 1 and prime factors of small sizes")
  {
    std::vector<float> data {3.0f};
    std::vector<std::complex<float>> result(1);
    // Used to hang in pfac().
    fastFourierTransform(std::span(result), std::span<const float>(data));
    CHECK(result[0] == std::complex<float>(3.0f, 0.0f));

    std::array<int, 32> kk {};
    CHECK(pfac(size_t(1), kk.data(), 'o') == 1);
    CHECK(kk[0] == 0);
    CHECK(pfac(size_t(12), kk.data(), 'o') == 12);
    CHECK(kk[0] == 3);// 2 * 2 * 3
  }
}