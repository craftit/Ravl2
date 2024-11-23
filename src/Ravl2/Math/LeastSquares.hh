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
#include "Ravl2/Math/LinearAlgebra.hh"

namespace Ravl2
{
  //! @brief Compute the normalisation for an array of points.
  //! This finds the mean and variation of euclidean point position. It corrects the mean to zero
  //! and the average variation to 1.
  //! @param: raw - Points to be normalised
  //! @return The mean to be subtracted from the points, and scale they should be multiplied by.
  template <typename RealT, IndexSizeT N, typename Container1T>
    requires std::is_floating_point_v<RealT>
  std::tuple<Point<RealT, N>, RealT> meanAndScale(const Container1T &raw)
  {
    Point<RealT, N> mean = Point<RealT, N>::Zero();
    for(auto it : raw) {
      mean += it;
    }
    size_t pnts = raw.size();
    if(pnts == 0) {
      const Point<RealT, N> zero = Point<RealT, N>::Zero();
      return {zero, 1};
    }
    auto realSize = static_cast<RealT>(pnts);
    mean /= realSize;
    RealT d = 0;
    for(auto it : raw) {
      if constexpr(N == 2) {
	d += std::hypot(it[0] - mean[0], it[1] - mean[1]);
      } else {
	d +=  (it - mean).norm();
      }
    }
    d = isNearZero(d) ? RealT(1) : (realSize / d);
    return {mean, d};
  }

  template <typename RealT, IndexSizeT N>
  Point<RealT, N> normalisePoint(const Point<RealT, N> &pnt, const Point<RealT, N> &mean, RealT scale)
  {
    return (pnt - mean) * scale;
  }

  //! @brief Normalise an array of points.
  //! This finds the mean and variation of euclidean point position. It corrects the mean to zero
  //! and the average variation to 1.
  //! @param: raw - Points to be normalised
  //! @param: func - Function/lambda to accept normalised point.
  //! @return The mean subtracted from the points, and scale applied.
  template <typename RealT, IndexSizeT N, typename Container1T, typename FuncT>
    requires std::is_floating_point_v<RealT>
  std::tuple<Point<RealT, N>, RealT> normalise(const Container1T &raw, FuncT func)
  {
    auto [mean, scale] = meanAndScale<RealT,N>(raw);
    for(auto it : raw) {
      Point<RealT, N> pnt = normalisePoint<RealT,N>(it, mean, scale);
      func(pnt);
    }
    return {mean, scale};
  }


  //! @brief Find least squares solution to X * v = 0, constraining the solutions to |V] == 1
  //! @param: X - Equation matrix.
  //! @param: rv - Result vector.
  //! @return: true if solution found.
  template <typename RealT>
  bool LeastSquaresEq0Mag1(const MatrixT<RealT> &X, VectorT<RealT> &rv)
  {
    auto svd = X.jacobiSvd(Eigen::ComputeFullV);

    //auto [U, S, V] = xt::linalg::svd(X);
    // The result matrix V is transposed from what we would expect.
    // rv = xt::view(V, xt::all(), -1);
    auto V = svd.matrixV();
    // Get the last column of V.
    rv = V.col(V.cols() - 1);
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
