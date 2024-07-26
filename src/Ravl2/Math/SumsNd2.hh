// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_SUMSND2_HEADER
#define RAVL_SUMSND2_HEADER 1
//! author="Charles Galambos"
//! date="28/1/2004"

#include "Ravl2/MeanCovariance.hh"
#include "Ravl2/MatrixRUT.hh"

namespace Ravl2 {

  //: Sums of a set of variables.
  // This class provides a efficient way of calculating statistics about
  // a set of variables.
  
  class SumsNd2C {
  public:
    SumsNd2C()
      : n(0)
    {}
    //: Default constructor.

    SumsNd2C(unsigned dim)
      : n(0),
	sum(dim),
	sum2(dim)
    { sum.fill(0); sum2.fill(0); }
    //: Construct with a specific dimension.
    
    SumsNd2C(RealT nn,const VectorC &nsum,const MatrixRUTC &nsum2)
      : n(nn),
	sum(nsum),
	sum2(nsum2)
    {}
    //: Construct with precomputed sums
    
    void Reset() {
      n = 0;
      sum.fill(0);
      sum2.fill(0);
    }
    //: Reset counters to zero
    
    const SumsNd2C& operator+=(const VectorC &vec) {
      n++;
      sum += vec;
      sum2.AddOuterProduct(vec);
      return *this;
    }
    //: Add a sample

    const SumsNd2C& operator-=(const VectorC &vec) {
      n--;
      sum -= vec;
      sum2.AddOuterProduct(vec,-1);
      return *this;
    }
    //: Add a sample
    
    void add(const VectorC &vec,RealT weight) {
      n += weight;
      sum.MulAdd(vec,weight);
      sum2.AddOuterProduct(vec,weight);
    }
    //: Add a weighted sample.
    
    RealT N() const
    { return n; }
    //: Access number of samples.
    
    const VectorC &Sum() const
    { return sum; }
    //: Sum of elements.
    
    const MatrixRUTC &Sum2() const
    { return sum2; }
    //: Sum of outer products of elements.
    
    VectorC Mean() const
    { return sum/n; }
    //: Compute the mean of the sample.
    
    MeanCovarianceC MeanCovariance(bool sampleStatistics = true) const;
    //: Compute mean and covariance of samples
    //!param: sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
    
  protected:
    RealT n;        // Number of elements.
    VectorC sum;   // Sample mean
    MatrixRUTC sum2; // Covariance matrix.
  };
  
  ostream& operator<<(std::ostream &s,const SumsNd2C &mv);
  istream& operator>>(std::istream &s, SumsNd2C &mv);
  BinOStreamC& operator<<(BinOStreamC &s,const SumsNd2C &mv);
  BinIStreamC& operator>>(BinIStreamC &s, SumsNd2C &mv);
  
}


#endif
