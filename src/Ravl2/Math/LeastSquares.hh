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
  template<typename RealT, unsigned N, typename Container1T, typename FuncT>
  requires std::is_floating_point_v<RealT>
  std::tuple<Point<RealT, 2>, RealT> normalise(const Container1T &raw, FuncT func)
  {
    Point<RealT, N> mean;
    for (unsigned i = 0; i < N; i++)
      mean[i] = 0;
    for (auto it: raw) {
      mean += it;
    }
    size_t pnts = raw.size();
    if (pnts == 0) {
      return {toPoint<RealT>(0, 0), 1};
    }
    auto realSize = static_cast<RealT>(pnts);
    mean /= realSize;
    RealT d = 0;
    for (auto it: raw) {
      if constexpr(N == 2) {
        d += std::hypot(it[0] - mean[0], it[1] - mean[1]);
      } else {
        RealT sum = 0;
        for (unsigned i = 0; i < N; i++) {
          sum += std::pow(it[i] - mean[i], 2);
        }
        d += std::sqrt(sum);
      }
    }
    d = isNearZero(d) ? RealT(1) : (realSize / d);
    for (auto it: raw) {
      Point<RealT, N> pnt = (it - mean) * d;
      func(pnt);
    }
    return {mean, d};
  }

  //: Find least squares solution to X * v = 0, contraining the solutions to |V] == 1
  //!param: X - Equation matrix.
  //!param: rv - Result vector.
  //!return: true if solution found.
  template<typename RealT>
  bool LeastSquaresEq0Mag1(const Tensor<RealT, 2> &X, VectorT<RealT> &rv)
  {
    auto [U, S, V] = xt::linalg::svd(X);
    // The result matrix V is transposed from what we would expect.
    // rv = xt::view(V, xt::all(), -1);
    rv = xt::view(V, -1, xt::all());
    return true;
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


#endif

}// namespace Ravl2
