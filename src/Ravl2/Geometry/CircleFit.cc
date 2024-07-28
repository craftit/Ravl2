// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="3/5/1997"
//! author="Charles Galambos"

#include "Ravl2/Geometry/Circle.hh"
#include "Ravl2/LeastSquares.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
  ////////////////////
  // Fit to some points.

  template<typename RealT>
  bool Circle2dC<RealT>::FitLSQ(const std::vector<Point<RealT,2>> &points,RealT &residual) {
    auto N = points.size();
    if(N < 3) // Under determined.
      return false;  
    MatrixT<RealT> A(N,3);
    VectorT<RealT> B(N);
    for(int i =0 ;i < N;i++) {
      const RealT X = points[i][0];
      const RealT Y = points[i][1];
      A[i][0] = X;
      A[i][1] = Y;
      A[i][2] = 1;
      B[i] = - X * X - Y * Y ;
    }
    if(!LeastSquaresQR_IP(A,B,residual))
      return false; // Fit failed.
    
    const RealT X = B[0] / -2;
    const RealT Y = B[1] / -2;
    Centre() = Point<RealT,2>({X,Y});
    Radius() = std::sqrt(((X * X) + (Y * Y)) - B[2]);
    ONDEBUG(SPDLOG_INFO("Circle2dC::FitLSQ() Center={} Radius={}", Centre(), Radius()));
    return true;
  }

}
