// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include <numbers>
#include <iostream>
#include <vector>
#include "Ravl2/Math.hh"

namespace Ravl2
{

  //! Mean and variance of a single variable.
  // If you want to build up statistics about a sample use the Sums1d2C
  // class to accumulate the information and create a MeanVariance<RealT> from
  // there.

  template<typename RealT>
  class MeanVariance
  {
  public:

    //! Default constructor.
    MeanVariance() = default;

    //! Copy constructor.
    MeanVariance(const MeanVariance<RealT> &) = default;

    //! Construct from parts
    MeanVariance(size_t nn,RealT nmean,RealT nvar)
      : mN(nn),
        mMean(nmean),
        mVar(nvar)
    {}

    //! Construct from a single value.
    MeanVariance &operator=(const MeanVariance<RealT> &) = default;

    //! Get the standard deviation.
    [[nodiscard]] RealT stdDeviation() const
    { return (mVar < 0) ? 0 : std::sqrt(mVar); }

    //! Access the variance.
    [[nodiscard]] RealT variance() const
    { return (mVar < 0) ? 0 : mVar; }

    //! Access the variance, this isn't limited to values zero or above.
    [[nodiscard]] RealT rawVariance() const
    { return mVar; }

    [[nodiscard]] const size_t &count() const
    { return mN; }
    //: Access the number of samples.

    [[nodiscard]] size_t &count()
    { return mN; }
    //: Access the number of samples.

    [[nodiscard]] RealT mean() const
    { return mMean; }
    //: Access the mean.
    
    [[nodiscard]] RealT Gauss(RealT x) const;
    //: Value of the normal (Gaussian) distribution at x, using this mean and variance.
    
    MeanVariance<RealT> &operator+=(const MeanVariance<RealT> &mv);
    //: Add another MeanVariance to this one.
    
    MeanVariance<RealT> &operator+=(const RealT &value);
    //: Add another sample

    MeanVariance<RealT> &operator-=(const MeanVariance<RealT> &mv);
    //: Remove another MeanVariance from this one.
    
    MeanVariance<RealT> operator*(const MeanVariance<RealT> &oth) const;
    //: Calculate the product of the two probability density functions.
    // (The number of samples is ignored)

  private:
    size_t mN = 0;
    RealT mMean = 0;
    RealT mVar = 0;
  };

  //! Calculate the mean and variance from an array of numbers.
  //!param: sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )

  template<typename RealT>
  MeanVariance<RealT> computeMeanVariance(const std::vector<RealT> &data,bool sampleStatistics = true)
  {
    auto n = data.size();
    RealT var = 0;
    RealT sum = 0;
    for(auto it : data) {
      sum += it;
      var += sqr(it);
    }
    RealT rn = RealT(n);
    RealT mean = sum / rn;
    RealT sn = rn;
    if(sampleStatistics) sn--;
    var = (var - sqr(sum)/rn)/sn;
    return MeanVariance<RealT>(n, mean, var);
  }

  //: Add another MeanVariance to this one.

  template<typename RealT>
  MeanVariance<RealT> &MeanVariance<RealT>::operator+=(const MeanVariance<RealT> &mv) {
    if(mv.count() == 0)
      return *this;
    const RealT number1 = RealT(count());
    const RealT number2 = RealT(mv.count());
    const RealT nDen    = number1 + number2;
    const RealT p1 = number1 / nDen;
    const RealT p2 = number2 / nDen;

    mVar *= p1;
    mVar += mv.variance() * p2;
    mVar += ((mv.mean() - mean()) * p1 * p2);

    // Update the mean.
    mMean = mMean * p1 + mv.mean() * p2;
    mN += mv.count();
    return *this;
  }

  //: Add another sample
  template<typename RealT>
  MeanVariance<RealT> &MeanVariance<RealT>::operator+=(const RealT &value) {
    mN += 1;
    RealT rn = RealT(mN);
    RealT delta = value - mMean;
    mMean += delta/rn;
    mVar = (mVar * (rn - 1) + (delta * (value - mMean))) / rn;
    return *this;
  }

  //: Remove another MeanVariance from this one.

  template<typename RealT>
  MeanVariance<RealT> &MeanVariance<RealT>::operator-=(const MeanVariance<RealT> &mv) {
    if(mv.count() == 0)
      return *this;
    const RealT number1 = RealT(count());
    const RealT number2 = RealT(mv.count());
    const RealT nDen    = number1 - number2;
    const RealT p1 = nDen / number1;
    const RealT p2 = number2 / number1;

    // Update the mean.
    mMean = (mMean - mv.mean() * p2) / p1 ;

    // Update the variance.
    mVar -= ((mv.mean() - mean()) * p1 * p2);
    mVar -= mv.variance() * p2;
    mVar /= p1;

    mN -= mv.count();
    return *this;
  }

  //: Value of the gauss distribution at x.

  template<typename RealT>
  RealT MeanVariance<RealT>::Gauss(RealT x) const {
    RealT sig = std::sqrt(mVar);
    return std::exp(RealT(-0.5) *sqr((x-mMean)/sig)) /(sig * std::sqrt(2 * std::numbers::pi_v<RealT>));
  }

  //: Calculate the product of the two probability density functions.
  // (The number of samples is ignored)

  template<typename RealT>
  MeanVariance<RealT> MeanVariance<RealT>::operator*(const MeanVariance<RealT> &oth) const {
    RealT sum = variance() + oth.variance();
    RealT newMean = (variance() * oth.mean() / sum) +
                    (oth.variance() * mean() / sum);
    RealT newVar = variance() * oth.variance() / sum;
    return MeanVariance(count() + oth.count(), newMean, newVar);
  }

  template<typename RealT>
  std::ostream& operator<<(std::ostream &s,const MeanVariance<RealT> &mv) {
    s << mv.count() << ' ' << mv.mean() << ' ' << mv.variance();
    return s;
  }

  template<typename RealT>
  std::istream& operator>>(std::istream &s, MeanVariance<RealT> &mv) {
    size_t n;
    RealT v1,v2;
    s >> n >> v1 >> v2;
    mv = MeanVariance(n,v1,v2);
    return s;
  }

  extern template class MeanVariance<double>;
  extern template class MeanVariance<float>;

}


