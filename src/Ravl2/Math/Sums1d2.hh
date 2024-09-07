// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="26/8/2002"

#include "Ravl2/Assert.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Math/MeanVariance.hh"

namespace Ravl2
{

  //! Sums of a variable.
  //! This class provides a way of calculating statistics about
  //! a variable.  Care should be taken when using this class
  //! particularly when the variable is a 'float' the precision is limited
  //! and we're summing potentially large numbers.

  template <typename RealT>
  class Sums1d2C
  {
  public:
    //! Default constructor.
    // Sets sums to zero.
    constexpr Sums1d2C() = default;

    //! Constructor from sum elements.
    constexpr Sums1d2C(unsigned nn, RealT nsum, RealT nsum2)
      : mN(nn),
	mSum(nsum),
	mSum2(nsum2)
    {}

    //! Constructor from sum elements.
    template<typename OtherT>
    explicit constexpr Sums1d2C(const Sums1d2C<OtherT> &other)
      : mN(other.size()),
	mSum(RealT(other.sum())),
	mSum2(RealT(other.sum2()))
    {}

    //! Create a Sums1d2C from mean variance.
    //! @param n - Number of data points.
    //! @param mean - Mean of the data.
    //! @param variance - Variance of the data.
    //! @param useSampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
    //! @return Sums1d2C
    [[nodiscard]] static constexpr Sums1d2C<RealT> fromMeanVariance(unsigned n, RealT mean, RealT variance, SampleStatisticsT useSampleStatistics = SampleStatisticsT::POPULATION)
    {
      RealT rn = RealT(n);
      RealT sum = mean * rn;
      return Sums1d2C(n, sum, variance * (rn - ((useSampleStatistics == SampleStatisticsT::SAMPLE) ? 1 : 0)) + sqr(sum) / rn);
    }

    //! Reset all counters.
    constexpr void reset()
    {
      mN = 0;
      mSum = 0;
      mSum2 = 0;
    }

    //! Add a point.
    constexpr void operator+=(RealT val)
    {
      mN++;
      mSum += val;
      mSum2 += sqr(val);
    }

    //! Remove a point.
    constexpr void operator-=(RealT val)
    {
      mN--;
      mSum -= val;
      mSum2 -= sqr(val);
    }

    //! Add another set of sums.
    constexpr void operator+=(const Sums1d2C &s)
    {
      mN += s.mN;
      mSum += s.mSum;
      mSum2 += s.mSum2;
    }

    //! Subtract another set of sums.
    constexpr void operator-=(const Sums1d2C &s)
    {
      RavlAssert(s.mN < mN);
      mN += s.mN;
      mSum -= s.mSum;
      mSum2 -= s.mSum2;
    }

    //! Number of data points.
    [[nodiscard]] constexpr auto size() const
    {
      return mN;
    }

    //! Number of data points.
    [[nodiscard]] constexpr auto N() const
    {
      return mN;
    }

    //! Number of data points.
    [[nodiscard]] constexpr auto count() const
    {
      return mN;
    }

    //! Sum of all data points.
    [[nodiscard]] constexpr RealT sum() const
    {
      return mSum;
    }

    //! Sum of squares of all data points.
    [[nodiscard]] constexpr RealT sum2() const
    {
      return mSum2;
    }

    //! Compute the variance of the sample.
    template<typename ResulT = RealT>
    [[nodiscard]] constexpr ResulT variance(SampleStatisticsT sampleStatistics) const
    {
      auto sn = mN;
      if(sampleStatistics == SampleStatisticsT::SAMPLE) sn--;
      ResulT var = ResulT((mSum2 * RealT(mN)) - sqr(mSum)) / ResulT(RealT(mN) * RealT(sn));
      if(var < 0) var = 0;
      return var;
    }

    //! Compute the standard deviation of the sample.
    template<typename ResulT = RealT>
    [[nodiscard]] constexpr RealT stdDeviation(SampleStatisticsT sampleStatistics) const
    {
      return std::sqrt(variance<ResulT>(sampleStatistics));
    }

    //! Compute the mean of the sample.
    template<typename ResulT = RealT>
    [[nodiscard]] constexpr ResulT mean() const
    {
      return ResulT(mSum) / ResulT(mN);
    }

    //! Add value as part of a rolling average.
    //!param: rollLen - Length of rolling average.
    //!param: value   - Value to add.
    inline constexpr void addRollingAverage(unsigned rollLength, RealT value)
    {
      if(rollLength < mN) {
        RealT rollFraction = (RealT(rollLength - 1) / (RealT(rollLength)));
        mSum *= rollFraction;
        mSum2 *= rollFraction;
      } else {
        mN++;
      }
      mSum += value;
      mSum2 += sqr(value);
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("n", mN),
         cereal::make_nvp("sum", mSum),
         cereal::make_nvp("sum2", mSum2));
    }

  private:
    unsigned mN = 0;
    RealT mSum = 0; // Sum of data.
    RealT mSum2 = 0;// Sum of square data.
  };

  template <typename RealT>
  std::ostream &operator<<(std::ostream &s, const Sums1d2C<RealT> &mv)
  {
    s << mv.size() << " " << mv.Sum() << " " << mv.Sum2();
    return s;
  }

  template <typename RealT>
  std::istream &operator>>(std::istream &s, Sums1d2C<RealT> &mv)
  {
    unsigned n;
    RealT s1, s2;
    s >> n >> s1 >> s2;
    mv = Sums1d2C(n, s1, s2);
    return s;
  }

  //! Calculate the mean and variance for this sample.
  //! @param sums - The sums of the data.
  //! @param sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
  //! @return MeanVariance
  template <typename RealT,typename OtherT>
  [[nodiscard]] constexpr MeanVariance<RealT> toMeanVariance(const Sums1d2C<OtherT> &sums, SampleStatisticsT sampleStatistics)
  {
    return MeanVariance<RealT>(sums.count(), sums.template mean<RealT>(), sums.template variance<RealT>(sampleStatistics));
  }

  //! Convert a MeanVariance to Sums1d2C.
  //! @param mv - The mean and variance of the data.
  //! @param sampleStatistic - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
  //! @return Sums1d2C
  template <typename RealT>
  [[nodiscard]] constexpr Sums1d2C<RealT> toSums(const MeanVariance<RealT> &mv, SampleStatisticsT sampleStatistic)
  {
    return Sums1d2C<RealT>::fromMeanVariance(mv.count(), mv.mean(), mv.variance(), sampleStatistic);
  }

  extern template class Sums1d2C<double>;
  extern template class Sums1d2C<float>;

}// namespace Ravl2
