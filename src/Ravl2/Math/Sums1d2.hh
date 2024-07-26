// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="26/8/2002"

#include "Ravl2/Math.hh"
#include "Ravl2/Math/MeanVariance.hh"

namespace Ravl2 {
  
  //! Sums of a variable.
  // This class provides a way of calculating statistics about
  // a variable. 

  template<typename RealT>
  class Sums1d2C {
  public:
    //! Default constructor.
    // Sets sums to zero.
    Sums1d2C() = default;

    //! Constructor from sum elements.
    Sums1d2C(unsigned nn,RealT nsum,RealT nsum2)
      : mN(nn),
        sum(nsum),
        mSum2(nsum2)
    {}

    //! Create a Sums1d2C from mean variance.
    [[nodiscard]] static Sums1d2C fromMeanVariance(int n, RealT mean, RealT variance, bool useSampleStatistics = true);

    //! Reset all counters.
    void reset()
    { mN = 0; sum = 0; mSum2 = 0; }

    //! Add a point.
    void operator+=(RealT val) {
      mN++;
      sum += val;
      mSum2 += Sqr(val);
    }

    //! Remove a point.
    void operator-=(RealT val) {
      mN--;
      sum -= val;
      mSum2 -= Sqr(val);
    }

    //! Add another set of sums.
    void operator+=(const Sums1d2C &s) {
      mN += s.mN;
      sum += s.sum;
      mSum2 += s.mSum2;
    }

    //! Subtract another set of sums.
    void operator-=(const Sums1d2C &s) {
      RavlAssert(s.mN < mN);
      mN += s.mN;
      sum -= s.sum;
      mSum2 -= s.mSum2;
    }

    //! Number of data points.
    [[nodiscard]] auto size() const
    { return mN; }

    //! Number of data points.
    [[nodiscard]] auto N() const
    { return mN; }

    //! Sum of all data points.
    [[nodiscard]] RealT Sum() const
    { return sum; }

    //! Sum of squares of all data points.
    [[nodiscard]] RealT Sum2() const
    { return mSum2; }

    //! Calculate the mean and variance for this sample.
    //!param: sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
    [[nodiscard]] MeanVariance<RealT> MeanVariance(bool sampleStatistics = true) const {
      return MeanVariance<RealT>(mN, Mean(), Variance(sampleStatistics));
    }

    //! Compute the variance of the sample.
    [[nodiscard]] RealT Variance(bool sampleStatistics = true) const {
      RealT rn = RealT(mN);
      RealT sn = rn;
      if(sampleStatistics) sn--;
      RealT var = (mSum2 - Sqr(sum) / rn) / sn;
      if (var < 0.0) var = 0.0;
      return var;
    }

    //! Compute the mean of the sample.
    [[nodiscard]] RealT Mean() const {
      RealT rn = RealT(mN);
      return sum / rn;
    }

    //! Add value as part of a rolling average.
    //!param: rollLen - Length of rolling average.
    //!param: value   - Value to add.
    inline void AddRollingAverage(unsigned rollLength,RealT value) {
      if(rollLength < mN) {
        RealT rollFraction = (RealT(rollLength-1)/(RealT(rollLength)));
        sum *= rollFraction;
        mSum2 *= rollFraction;
      } else
        mN++;
      sum += value;
      mSum2 += Sqr(value);
    }

  private:
    unsigned mN = 0;
    RealT sum = 0; // Sum of data.
    RealT mSum2 = 0; // Sum of square data.
  };

  template<typename RealT>
  Sums1d2C<RealT> Sums1d2C<RealT>::fromMeanVariance(int n, RealT mean, RealT variance, bool useSampleStatistics)
  {
    RealT rn = n;
    RealT sum = mean * rn;
    return Sums1d2C(n,sum,variance * (rn -(useSampleStatistics ? 1.0 : 0)) + Sqr(sum)/rn);
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


