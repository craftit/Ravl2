// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#include "Ravl2/SumsNd2.hh"
#include "Ravl2/BinStream.hh"

namespace Ravl2 {

  //: Compute mean and covariance of samples
  
  MeanCovarianceC SumsNd2C::MeanCovariance(bool sampleStatistics) const {
    VectorC mean = sum / n;
    MatrixRUTC cov = sum2.Copy();
    cov.SubtractOuterProduct(mean,n);
    RealT sn = n;
    if(sampleStatistics) sn--;
    cov /= sn;
    cov.MakeSymmetric();
    return MeanCovarianceC(n,mean,cov);
  }
  
  ostream& operator<<(std::ostream &s,const SumsNd2C &mv) {
    s << mv.N() << " " << mv.Sum() << " " << mv.Sum2();
    return s;
  }
  
  istream& operator>>(std::istream &s, SumsNd2C &mv) {
    RealT n;
    VectorC s1;
    MatrixRUTC s2;
    s >> n >> s1 >> s2;
    mv = SumsNd2C(n,s1,s2);
    return s;
  }

  BinOStreamC& operator<<(BinOStreamC &s,const SumsNd2C &mv) {
    s << mv.N() << mv.Sum() << mv.Sum2();
    return s;
  }
  
  BinIStreamC& operator>>(BinIStreamC &s, SumsNd2C &mv) {
    RealT n;
    VectorC s1;
    MatrixRUTC s2;
    s >> n >> s1 >> s2;
    mv = SumsNd2C(n,s1,s2);
    return s;
  }

}
