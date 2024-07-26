// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_MEANVARIANCE_HEADER
#define RAVL_MEANVARIANCE_HEADER 1
/////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! userlevel=Normal
//! docentry="Ravl.API.Math.Statistics"
//! lib=RavlMath
//! file="Ravl/Math/Statistics/MeanCovariance/MeanVariance.hh"

#include "Ravl/Math.hh"
#include "Ravl/SArray1d.hh"

namespace RavlN {

  //! userlevel=Normal
  //: Mean and variance of a single variable.
  // If you want to build up statistics about a sample use the Sums1d2C
  // class to accumulate the information and create a MeanVarianceC from
  // there.
  
  class MeanVarianceC {
  public:
    MeanVarianceC(const SArray1dC<RealT> &data,bool sampleStatistics = true);
    //: Calculate the mean and variance from an array of numbers.
    //!param: sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
    
    MeanVarianceC()
      : n(0),
	mean(0),
	var(0)
    {}
    //: Default constructor.
    
    MeanVarianceC(SizeT nn,RealT nmean,RealT nvar)
      : n(nn),
	mean(nmean),
	var(nvar)
    {}
    //: Constructor.
    
    RealT StdDeviation() const
    { return (var<0.0) ? 0.0 : Sqrt(var); }
    //: Get the standard deviation.
    
    RealT Variance() const
    { return (var<0.0) ? 0.0 : var; }
    //: Access the variance.

    const RealT &RawVariance() const
    { return var; }
    //: Access the variance, this isn't limited to values zero or above.

    const SizeT &Number() const
    { return n; }
    //: Access the number of samples.

    SizeT &Number()
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
    
    size_t Hash() const
    { return n + StdHash(mean) + StdHash(var); }
    //: Provided for compatibility with templates.
  protected:
    SizeT n;
    RealT mean;
    RealT var; 
  };
  
  ostream& operator<<(ostream &s,const MeanVarianceC &mv);
  istream& operator>>(istream &s, MeanVarianceC &mv);
  BinOStreamC& operator<<(BinOStreamC &s,const MeanVarianceC &mv);
  BinIStreamC& operator>>(BinIStreamC &s, MeanVarianceC &mv);
}


#endif
