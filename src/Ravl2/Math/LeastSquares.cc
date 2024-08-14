// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/LinearAlgebra/General/LeastSquares.cc"

#include "Ravl2/Math/LeastSquares.hh"

//#include "ccmath/ccmath.h"
//#include "Ravl/Exception.hh"
//#include "Ravl/SVD.hh"

//namespace Ravl2 {

#if 0
  //: Find a least squares solution to A*x = b
  // Uses the QR algorithm.
  
  VectorC LeastSquaresQR(const MatrixC &xA,const VectorC &xb,RealT &residual) {
    MatrixC A = xA.Copy();
    VectorC b = xb.Copy();
    int f = 0;
    RavlAssert(A.Size1() == xb.Size());
    residual = qrlsq(&(A[0][0]),&(b[0]),A.Size1(),A.Size2(),&f);
    if(f != 0) 
      return VectorC();
    return VectorC(SArray1dC<RealT>(b,A.Size2()));
  }
  
  //: Find a least squares solution to A*x = b
  // This destroys the contents of A, and replaces b with the solution vector.
 
  bool LeastSquaresQR_IP(MatrixC &A,VectorC &b,RealT &residual) {
    int f = 0;
    residual = qrlsq(&(A[0][0]),&(b[0]),A.Size1(),A.Size2(),&f);
    b = VectorC(SArray1dC<RealT>(b,A.Size2()));
    return f == 0 && residual >= 0;
  }

  //: Find a least squares solution to A*x = b
  // Where the solution is of a known rank.
  // From Appendix 5 of the Second edition of Multiple View Geometry by
  // R. Hartly and A.Zisserman.
  
  VectorC LeastSquaresFixedRank(const MatrixC &A,const VectorC &b,UIntT rank) {
    MatrixC U,V;
    RavlAssertMsg(A.Rows() == b.Size(),"LeastSquaresFixedRank(), A should have the same number of rows as entries in b.");
    VectorC d = SVD(A,U,V);
    VectorC b1 = U.TMul(b);
    VectorC y(b1.Size());
    for(UIntT i = 0;i < rank;i++)
      y[i] = b1[i]/d[i];
    for(UIntT i = rank;i < y.Size();i++)
      y[i] = 0;
    VectorC result = V*y;
    return result;
  }
  
  //: Solve X * v = 0, contraining the solutions to |V] == 1 
  //!param: X - Equation matrix.
  //!param: rv - Result vector.
  
  bool LeastSquaresEq0Mag1(const MatrixC &X,VectorC &rv) {
    SVDC<RealT> SVD(X,false,true);
    MatrixC v = SVD.GetV();
    TVectorC<RealT> sv = SVD.SingularValues();
    rv = v.SliceColumn(v.Cols()-1);
    return true;
  }
#endif
  
//}
