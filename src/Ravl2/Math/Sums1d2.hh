// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_SUMS1D2_HEADER
#define RAVL_SUMS1D2_HEADER 1
//! author="Charles Galambos"
//! date="26/8/2002"

#include "Ravl2/Math.hh"
#include "Ravl2/MeanVariance.hh"

namespace Ravl2 {
  
  //: Sums of a variable.
  // This class provides a way of calculating statistics about
  // a variable. 
  
  class Sums1d2C {
  public:
    Sums1d2C()
      : n(0),
	sum(0),
	sum2(0)
    {}
    //: Default constructor.
    // Sets sums to zero.
    
    Sums1d2C(unsigned nn,RealT nsum,RealT nsum2)
      : n(nn),
	sum(nsum),
	sum2(nsum2)
    {}
    //: Constructor from sum elements.
    
    static Sums1d2C CreateFromMeanVariance(int n,RealT mean,RealT variance,bool useSampleStatistics = true);
    //: Create a Sums1d2C from mean variance.

    void Reset()
    { n = 0; sum = 0; sum2 = 0; }
    //: Reset all counters.
    
    void operator+=(RealT val) {
      n++;
      sum += val;
      sum2 += Sqr(val);
    }
    //: Add a point.

    void operator-=(RealT val) {
      n--;
      sum -= val;
      sum2 -= Sqr(val);
    }
    //: Remove a point.
    
    void operator+=(const Sums1d2C &s) {
      n += s.n;
      sum += s.sum;
      sum2 += s.sum2;
    }
    //: Add another set of sums.

    void operator-=(const Sums1d2C &s) {
      RavlAssert(s.n < n);
      n += s.n;
      sum -= s.sum;
      sum2 -= s.sum2;
    }
    //: Subtract another set of sums.
    
    const unsigned &Size() const
    { return n; }
    //: Number of data points.
    
    const unsigned &N() const
    { return n; }
    //: Number of data points.
    
    const RealT &Sum() const
    { return sum; }
    //: Sum of all data points.

    const RealT &Sum2() const
    { return sum2; }
    //: Sum of squares of all data points.
    
    MeanVarianceC MeanVariance(bool sampleStatistics = true) const {
      return MeanVarianceC(n, Mean(), Variance(sampleStatistics));
    }
    //: Calculate the mean and variance for this sample.
    //!param: sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
    
    RealT Variance(bool sampleStatistics = true) const {
      RealT rn = (RealT) n;
      RealT sn = rn;
      if(sampleStatistics) sn--;
      RealT var = (sum2 - Sqr(sum)/rn)/sn;
      if (var < 0.0) var = 0.0;
      return var;
    }
    //: Compute the variance of the sample.
    
    RealT Mean() const {
      RealT rn = (RealT) n;
      return sum / rn;
    }
    //: Compute the mean of the sample.
    
    inline void AddRollingAverage(unsigned rollLength,RealT value) {
      if(rollLength < n) {
        RealT rollFraction = ((RealT) (rollLength-1)/((RealT) rollLength));
        sum *= rollFraction;
        sum2 *= rollFraction;
      } else
        n++;
      sum += value;
      sum2 += Sqr(value);      
    }
    //: Add value as part of a rolling average.
    //!param: rollLen - Length of rolling average.
    //!param: value   - Value to add.
    
    size_t Hash() const
    { return n; }
    //: Hash of value
  protected:
    unsigned n;
    RealT sum; // Sum of data.
    RealT sum2; // Sum of square data.
  };

  ostream& operator<<(std::ostream &s,const Sums1d2C &mv);
  istream& operator>>(std::istream &s, Sums1d2C &mv);
  BinOStreamC& operator<<(BinOStreamC &s,const Sums1d2C &mv);
  BinIStreamC& operator>>(BinIStreamC &s, Sums1d2C &mv);
  bool operator==(const Sums1d2C &v2,const Sums1d2C &v1);
  bool operator!=(const Sums1d2C &v2,const Sums1d2C &v1);

}


#endif
