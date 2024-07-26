// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_MEANVARIANCE_HEADER
#define RAVL_MEANVARIANCE_HEADER 1
/////////////////////////////////////////////////////////

#include <iostream>
#include <vector>
#include "Ravl2/Math.hh"

namespace Ravl2 {

  //: Mean and variance of a single variable.
  // If you want to build up statistics about a sample use the Sums1d2C
  // class to accumulate the information and create a MeanVarianceC from
  // there.

  template<typename RealT>
  class MeanVarianceC {
  public:
    MeanVarianceC(const std::vector<RealT> &data,bool sampleStatistics = true);
    //: Calculate the mean and variance from an array of numbers.
    //!param: sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
    
    MeanVarianceC()
      : n(0),
	mean(0),
	var(0)
    {}
    //: Default constructor.
    
    MeanVarianceC(size_t nn,RealT nmean,RealT nvar)
      : n(nn),
	mean(nmean),
	var(nvar)
    {}
    //: Constructor.
    
    RealT StdDeviation() const
    { return (var<0.0) ? 0.0 : std::sqrt(var); }
    //: Get the standard deviation.
    
    RealT Variance() const
    { return (var<0.0) ? 0.0 : var; }
    //: Access the variance.

    const RealT &RawVariance() const
    { return var; }
    //: Access the variance, this isn't limited to values zero or above.

    const size_t &Number() const
    { return n; }
    //: Access the number of samples.

    size_t &Number()
    { return n; }
    //: Access the number of samples.

    const RealT &Mean() const
    { return mean; }
    //: Access the mean.
    
    RealT Probability(RealT low,RealT high) const;
    //: Find the probability of getting a sample with a values between low and high.
    
    RealT ProbabilityOfHigherValue(RealT threshold,bool quickApprox = false) const;
    //: Compute the probability of a value higher than the threshold
    //!param: threshold - point in distribution.
    //!param: quickApprox - Use a table based approximation which makes this function 10x faster, but is not as accurate.
    
    RealT Gauss(RealT x) const;
    //: Value of the normal (Gaussian) distribution at x, using this mean and variance.
    
    MeanVarianceC &operator+=(const MeanVarianceC &mv);
    //: Add another MeanVariance to this one.
    
    MeanVarianceC &operator+=(const RealT &value);
    //: Add another sample

    MeanVarianceC &operator-=(const MeanVarianceC &mv);
    //: Remove another MeanVariance from this one.
    
    MeanVarianceC operator*(const MeanVarianceC &oth) const;
    //: Calculate the product of the two probability density functions.
    // (The number of samples is ignored)
    
  protected:
    size_t n;
    RealT mean;
    RealT var; 
  };

  template<typename RealT>
  std::ostream& operator<<(std::ostream &s,const MeanVarianceC &mv);

  template<typename RealT>
  std::istream& operator>>(std::istream &s, MeanVarianceC &mv);

  //: Calculate the mean and variance from an array of numbers.

  template<typename RealT>
  MeanVarianceC<RealT>::MeanVarianceC(const std::vector<RealT> &data,bool sampleStatistics) {
    n = data.size();
    var = 0;
    RealT sum = 0;
    for(SArray1dIterC<RealT> it(data);it;it++) {
      sum += *it;
      var += Sqr(*it);
    }
    RealT rn = (RealT) n;
    mean = sum / rn;
    RealT sn = rn;
    if(sampleStatistics) sn--;
    var = (var - Sqr(sum)/rn)/sn;
  }

  //: Add another MeanVariance to this one.

  template<typename RealT>
  MeanVarianceC<RealT> &MeanVarianceC<RealT>::operator+=(const MeanVarianceC &mv) {
    if(mv.Number() == 0)
      return *this;
    const RealT number1 = (RealT) Number();
    const RealT number2 = (RealT) mv.Number();
    const RealT nDen    = number1 + number2;
    const RealT p1 = number1 / nDen;
    const RealT p2 = number2 / nDen;

    var *= p1;
    var += mv.Variance() * p2;
    var += ((mv.Mean() - Mean()) * p1*p2);

    // Update the mean.
    mean = mean * p1 + mv.Mean() * p2;
    n += mv.Number();
    return *this;
  }

  //: Add another sample
  template<typename RealT>
  MeanVarianceC<RealT> &MeanVarianceC<RealT>::operator+=(const RealT &value) {
    n += 1;
    RealT rn = n;
    RealT delta = value - mean;
    mean += delta/rn;
    var = (var * (rn-1.0) + (delta*(value - mean)))/rn;
    return *this;
  }

  //: Remove another MeanVariance from this one.

  template<typename RealT>
  MeanVarianceC<RealT> &MeanVarianceC<RealT>::operator-=(const MeanVarianceC &mv) {
    if(mv.Number() == 0)
      return *this;
    const RealT number1 = (RealT) Number();
    const RealT number2 = (RealT) mv.Number();
    const RealT nDen    = number1 - number2;
    const RealT p1 = nDen / number1;
    const RealT p2 = number2 / number1;

    // Update the mean.
    mean = (mean - mv.Mean() * p2) / p1 ;

    // Update the variance.
    var -= ((mv.Mean() - Mean()) * p1*p2);
    var -= mv.Variance() * p2;
    var /= p1;

    n -= mv.Number();
    return *this;
  }

  //: Value of the gauss distribution at x.

  template<typename RealT>
  RealT MeanVarianceC<RealT>::Gauss(RealT x) const {
    RealT sig = std::sqrt(var);
    return Exp(-0.5 * Sqr((x-mean)/sig)) /(sig * RavlConstN::sqrt2Pi);
  }

  //: Find the probability of getting a sample with value 'at' +/- delta.

  template<typename RealT>
  RealT MeanVarianceC<RealT>::Probability(RealT low,RealT high) const {
    RealT sig = std::sqrt(var);
    return (StatNormalQ((low-mean)/sig) - StatNormalQ((high-mean)/sig));
  }

  //: Compute the probability of a value higher than a threshold

  template<typename RealT>
  RealT MeanVarianceC<RealT>::ProbabilityOfHigherValue(RealT threshold,bool quickApprox) const {
    // Deal with 0 variance case.
    if(var <= 0) {
      if(mean == threshold)
        return 0.5;
      if(mean < threshold)
        return 0;
      return 1.0;
    }
    // And the rest.
    RealT sig = std::sqrt(var);
    return StatNormalQ((threshold-mean)/sig,quickApprox);
  }

  //: Calculate the product of the two probability density functions.
  // (The number of samples is ignored)

  template<typename RealT>
  MeanVarianceC<RealT> MeanVarianceC<RealT>::operator*(const MeanVarianceC &oth) const {
    RealT sum = Variance() + oth.Variance();
    RealT newMean = (Variance() * oth.Mean() / sum) +
                    (oth.Variance() * Mean() / sum);
    RealT newVar = Variance() * oth.Variance() / sum;
    return MeanVarianceC(Number() + oth.Number(),newMean,newVar);
  }

  template<typename RealT>
  std::ostream& operator<<(std::ostream &s,const MeanVarianceC<RealT> &mv) {
    s << mv.Number() << ' ' << mv.Mean() << ' ' << mv.Variance();
    return s;
  }

  template<typename RealT>
  std::istream& operator>>(std::istream &s, MeanVarianceC<RealT> &mv) {
    size_t n;
    RealT v1,v2;
    s >> n >> v1 >> v2;
    mv = MeanVarianceC(n,v1,v2);
    return s;
  }


}


#endif
