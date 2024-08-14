// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlMath
//! docentry="Ravl.API.Math.Linear Algebra.Equation Solvers"
//! userlevel=Normal
//! file="Ravl/Math/LinearAlgebra/General/LeastSquares.hh"

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wparentheses"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wimplicit-float-conversion"
#endif
#include <xtensor/xmath.hpp>
#include <xtensor-blas/xlinalg.hpp>
#pragma GCC diagnostic pop

#include "Ravl2/Math/LinearAlgebra.hh"

namespace Ravl2
{

  //! @brief Normalise an array of points.
  //! This finds the mean and variation of euclidean point position. It corrects the mean to zero
  //! and the average variation to 1.
  //!param: raw - Points to be normalised
  //!param: func - Function/lambda to accept normalised point.
  //! @return The mean subtracted from the points, and scale applied.
  template <typename RealT, unsigned N,typename Container1T, typename FuncT>
   requires std::is_floating_point_v<RealT>
  std::tuple<Point<RealT,2>, RealT> normalise(const Container1T &raw,FuncT func)
  {
    Point<RealT,N> mean = 0;
//    for(unsigned i =0;i < N;i++)
//      mean[i] = 0;
    for(auto it : raw) {
      mean += it;
    }
    size_t pnts = raw.size();
    if(pnts == 0) {
      return { toPoint<RealT>(0,0), 1};
    }
    auto realSize = static_cast<RealT>(pnts);
    mean /= realSize;
    RealT d = 0;
    for(auto it : raw) {
      d += std::hypot(it[0] - mean[0],it[1] - mean[1]);
    }
    d = isNearZero(d) ? RealT(1) : (realSize/d);
    for(auto it : raw) {
      Point<RealT,N> pnt = (it - mean) * d;
      func(pnt);
    }
//    return Matrix<RealT,3,3>(
//      {{ d,0,-mean[0] * d},
//        {0,d,-mean[1] * d},
//        { 0,0,1}});
    return {mean,d};
  }

#if 0
  //! @brief Find a least squares solution to A*x = b
  //! This destroys the contents of A, and replaces b with the solution vector.
  //! The residual is assigned to 'residual'.
  //! Returns false if the rank of 'A' is insufficient.
  template<typename RealT>
  bool LeastSquaresQR_IP(MatrixC &A, VectorC &b, RealT &residual)
  {
    int f = 0;
    residual = qrlsq(&(A[0][0]),&(b[0]),A.Size1(),A.Size2(),&f);
    b = VectorC(SArray1dC<RealT>(b,A.Size2()));
    return f == 0 && residual >= 0;
  }


  template <class Container1T>
  bool TemplNormalise(const Container1T &raw, DListC<Point2dC> &norm,Matrix3dC &normMat) {
    if(raw.Size() == 0)
      return true;
    norm = DListC<Point2dC>();
    Point2dC mean(0,0);

    for(typename Container1T::IteratorT rawIter(raw);rawIter;rawIter++) {
      RavlAssert(!IsNan(rawIter.Data()[0]) && !IsNan(rawIter.Data()[1]));
      norm.Append(rawIter.Data());
      mean += rawIter.Data();

    }
    mean /= static_cast<RealT>(raw.Size());
    RealT d = 0;
    for(DListC<Point2dC>::IteratorT it(norm);it;it++) {
      (*it) -= mean;
      d += Hypot((*it)[0],(*it)[1]);
    }
    d = IsSmall(d) ? 1.0 : (static_cast<RealT>(raw.Size())/d);
    for(DListC<Point2dC>::IteratorT it(norm);it;it++)
      (*it) *= d;
    normMat = Matrix3dC(d,0,-mean[0] * d,
			0,d,-mean[1] * d,
			0,0,1);
    return true;
  }
  //: Normalise an array of points.
  // This finds the mean and variation of euclidean point position. It corrects the mean to zero
  // and the average variation to 1.
  //!param: raw - Points to be normalised
  //!param: norm - Array to write normalised points into. A new array is always created as assigned to this handle. Note: If the array is of the required size it may be resused.
  //!param: normMat - Normalisation matrix. Multiplying the normalised points by this matrix will map them to the original space.
  //!return: true if normalisation found and applied.



  template <class Container1T, class Container2T> // Both Container types must be containers of Point2dC. Both must be iterable and Container2T must be able to be initialized with the size of the container.
  bool normalise(const Container1T &raw,Container2T &norm,Matrix<RealT,3,3> &normMat) {
    if(raw.Size() == 0)
      return true;
    norm = Container2T(raw.Size());

    typename Container1T::IteratorT rawIter(raw);
    typename Container2T::IteratorT normIter(norm);

    Point2dC mean(0,0);

    for(rawIter.First(), normIter.First();rawIter;rawIter++, normIter++) {
      RavlAssert(normIter);
      RavlAssert(!IsNan(rawIter.Data()[0]) && !IsNan(rawIter.Data()[1]));
      normIter.Data() = rawIter.Data();
      mean += rawIter.Data();
    }

    mean /= static_cast<RealT>(raw.Size());
    RealT d = 0;
    for(normIter.First();normIter;normIter++) {
      (*normIter) -= mean;
      d += Hypot((*normIter)[0],(*normIter)[1]);
    }
    d = IsSmall(d) ? 1.0 : (static_cast<RealT>(raw.Size())/d);
    for(normIter.First();normIter;normIter++)
      (*normIter) *= d;
    normMat = Matrix3dC(d,0,-mean[0] * d,
                        0,d,-mean[1] * d,
                        0,0,1);
    return true;
  }
  //: Normalise an array of points.
  // This finds the mean and variation of euclidean point position. It corrects the mean to zero
  // and the average variation to 1.
  //!param: raw - Points to be normalised
  //!param: norm - Array to write normalised points into. A new array is always created as assigned to this handle. Note: If the array is of the required size it may be resused.
  //!param: normMat - Normalisation matrix. Multiplying the normalised points by this matrix will map them to the original space.
  //!return: true if normalisation found and applied.



  VectorC LeastSquaresQR(const MatrixC &A, const VectorC &b, RealT &residual);
  //: Find a least squares solution to A*x = b
  // Uses the QR algorithm.  Returns false if the rank of 'A' is insufficient.
  // The residual is assigned to 'residual'.

  inline VectorC LeastSquaresQR(const MatrixC &A, const VectorC &b)
  {
    RealT tmp;
    return LeastSquaresQR(A, b, tmp);
  }
  //: Find a least squares solution to A*x = b
  // Uses the QR algorithm.  Returns false if the rank of 'A' is insufficient.

  VectorC LeastSquaresFixedRank(const MatrixC &A, const VectorC &b, UIntT rank);
  //: Find least squares solution for x where A*x = b, with reduced rank.
  //!param: A - Matrix
  //!param: b - Vector.
  //!param: rank - Limit to rank of solution.
  // From Appendix 5 of the Second edition of Multiple View Geometry by
  // R. Hartly and A.Zisserman.

  bool LeastSquaresEq0Mag1(const MatrixC &X, VectorC &rv);
  //: Find least squares solution to X * v = 0, contraining the solutions to |V] == 1
  //!param: X - Equation matrix.
  //!param: rv - Result vector.
  //!return: true if solution found.

#endif
}// namespace RavlN

