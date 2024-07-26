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
  // This class provides a way of calculating statistics about
  // a variable.  Care should be taken when using this class
  // particularly when the variable is a 'float' the precision is limited
  // and we're summing potentially large numbers.

  template<typename RealT>
  class Sums1d2C {
  public:
    //! Default constructor.
    // Sets sums to zero.
    Sums1d2C() = default;

    //! Constructor from sum elements.
    Sums1d2C(unsigned nn,RealT nsum,RealT nsum2)
      : mN(nn),
        mSum(nsum),
        mSum2(nsum2)
    {}

    //! Create a Sums1d2C from mean variance.
    [[nodiscard]] static Sums1d2C<RealT> fromMeanVariance(unsigned n, RealT mean, RealT variance, bool useSampleStatistics = true);

    //! Reset all counters.
    void reset()
    { mN = 0; mSum = 0; mSum2 = 0; }

    //! Add a point.
    void operator+=(RealT val) {
      mN++;
      mSum += val;
      mSum2 +=sqr(val);
    }

    //! Remove a point.
    void operator-=(RealT val) {
      mN--;
      mSum -= val;
      mSum2 -=sqr(val);
    }

    //! Add another set of sums.
    void operator+=(const Sums1d2C &s) {
      mN += s.mN;
      mSum += s.mSum;
      mSum2 += s.mSum2;
    }

    //! Subtract another set of sums.
    void operator-=(const Sums1d2C &s) {
      RavlAssert(s.mN < mN);
      mN += s.mN;
      mSum -= s.mSum;
      mSum2 -= s.mSum2;
    }

    //! Number of data points.
    [[nodiscard]] auto size() const
    { return mN; }

    //! Number of data points.
    [[nodiscard]] auto N() const
    { return mN; }

    //! Number of data points.
    [[nodiscard]] auto count() const
    { return mN; }

    //! Sum of all data points.
    [[nodiscard]] RealT sum() const
    { return mSum; }

    //! Sum of squares of all data points.
    [[nodiscard]] RealT sum2() const
    { return mSum2; }

    //! Calculate the mean and variance for this sample.
    //!param: sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
    [[nodiscard]] MeanVariance<RealT> toMeanVariance(bool sampleStatistics = true) const {
      return MeanVariance<RealT>(mN, mean(), variance(sampleStatistics));
    }

    //! Compute the variance of the sample.
    [[nodiscard]] RealT variance(bool sampleStatistics = true) const {
      RealT rn = RealT(mN);
      RealT sn = rn;
      if(sampleStatistics) sn--;
      RealT var = (mSum2 -sqr(mSum) / rn) / sn;
      if (var < 0) var = 0;
      return var;
    }

    //! Compute the mean of the sample.
    [[nodiscard]] RealT mean() const {
      RealT rn = RealT(mN);
      return mSum / rn;
    }

    //! Add value as part of a rolling average.
    //!param: rollLen - Length of rolling average.
    //!param: value   - Value to add.
    inline void AddRollingAverage(unsigned rollLength,RealT value) {
      if(rollLength < mN) {
        RealT rollFraction = (RealT(rollLength-1)/(RealT(rollLength)));
        mSum *= rollFraction;
        mSum2 *= rollFraction;
      } else
        mN++;
      mSum += value;
      mSum2 +=sqr(value);
    }

  private:
    unsigned mN = 0;
    RealT mSum = 0; // Sum of data.
    RealT mSum2 = 0; // Sum of square data.
  };

  template<typename RealT>
  Sums1d2C<RealT> Sums1d2C<RealT>::fromMeanVariance(unsigned n, RealT mean, RealT variance, bool useSampleStatistics)
  {
    RealT rn = RealT(n);
    RealT sum = mean * rn;
    return Sums1d2C(n,sum,variance * (rn -(useSampleStatistics ? 1 : 0)) +sqr(sum)/rn);
  }

  //: Create a Sums1d2C from mean variance.

  template<typename RealT>
  std::ostream& operator<<(std::ostream &s,const Sums1d2C<RealT> &mv) {
    s << mv.size() << " " << mv.Sum() << " " << mv.Sum2();
    return s;
  }

  template<typename RealT>
  std::istream& operator>>(std::istream &s, Sums1d2C<RealT> &mv) {
    unsigned n;
    RealT s1,s2;
    s >> n >> s1 >> s2;
    mv = Sums1d2C(n,s1,s2);
    return s;
  }

  extern template class Sums1d2C<double>;
  extern template class Sums1d2C<float>;

}


