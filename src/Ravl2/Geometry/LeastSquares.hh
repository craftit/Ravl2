// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_LEASTSQUARES_HEADER
#define RAVL_LEASTSQUARES_HEADER 1
//! rcsid="$Id$"
//! lib=RavlMath
//! docentry="Ravl.API.Math.Linear Algebra.Equation Solvers"
//! userlevel=Normal
//! file="Ravl/Math/LinearAlgebra/General/LeastSquares.hh"

#include "Ravl/Matrix.hh"
#include "Ravl/Vector.hh"

namespace RavlN {
  
  VectorC LeastSquaresQR(const MatrixC &A,const VectorC &b,RealT &residual);
  //: Find a least squares solution to A*x = b
  // Uses the QR algorithm.  Returns false if the rank of 'A' is insufficient.
  // The residual is assigned to 'residual'.
  
  inline VectorC LeastSquaresQR(const MatrixC &A,const VectorC &b) { 
    RealT tmp;
    return LeastSquaresQR(A,b,tmp);
  }
  //: Find a least squares solution to A*x = b
  // Uses the QR algorithm.  Returns false if the rank of 'A' is insufficient.
  
  bool LeastSquaresQR_IP(MatrixC &A,VectorC &b,RealT &residual);
  //: Find a least squares solution to A*x = b
  // This destroys the contents of A, and replaces b with the solution vector.
  // The residual is assigned to 'residual'.
  // Returns false if the rank of 'A' is insufficient.
  
  VectorC LeastSquaresFixedRank(const MatrixC &A,const VectorC &b,UIntT rank);
  //: Find least squares solution for x where A*x = b, with reduced rank.
  //!param: A - Matrix
  //!param: b - Vector.
  //!param: rank - Limit to rank of solution.
  // From Appendix 5 of the Second edition of Multiple View Geometry by
  // R. Hartly and A.Zisserman.
  
  bool LeastSquaresEq0Mag1(const MatrixC &X,VectorC &rv);
  //: Find least squares solution to X * v = 0, contraining the solutions to |V] == 1 
  //!param: X - Equation matrix.
  //!param: rv - Result vector.
  //!return: true if solution found.
  
  
  
}

#endif
