// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Sums1d2.hh"
#include "Ravl2/BinStream.hh"

namespace Ravl2 {

  Sums1d2C Sums1d2C::CreateFromMeanVariance(int n,RealT mean,RealT variance,bool useSampleStatistics)
  {
    RealT rn = n;
    RealT sum = mean * rn;
    return Sums1d2C(n,sum,variance * (rn -(useSampleStatistics ? 1.0 : 0)) + Sqr(sum)/rn);
  }

  //: Create a Sums1d2C from mean variance.

  ostream& operator<<(std::ostream &s,const Sums1d2C &mv) {
    s << mv.size() << " " << mv.Sum() << " " << mv.Sum2();
    return s;
  }
  
  istream& operator>>(std::istream &s, Sums1d2C &mv) {
    unsigned n;
    RealT s1,s2;
    s >> n >> s1 >> s2;
    mv = Sums1d2C(n,s1,s2);
    return s;
  }

  BinOStreamC& operator<<(BinOStreamC &s,const Sums1d2C &mv) {
    s << mv.size() << mv.Sum() << mv.Sum2();
    return s;
  }
  
  BinIStreamC& operator>>(BinIStreamC &s, Sums1d2C &mv) {
    unsigned n;
    RealT s1,s2;
    s >> n >> s1 >> s2;
    mv = Sums1d2C(n,s1,s2);
    return s;
  }

  bool operator==(const Sums1d2C &v2,const Sums1d2C &v1) {
    return &v1 == &v2;
  }

  bool operator!=(const Sums1d2C &v2,const Sums1d2C &v1) {
    return &v1 != &v2;
  }

}
