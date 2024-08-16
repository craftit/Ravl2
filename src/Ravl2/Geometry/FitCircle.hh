// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="3/5/1997"
//! author="Charles Galambos"

#pragma once

#include <spdlog/spdlog.h>
#include "Ravl2/Geometry/Circle.hh"
#include "Ravl2/Math/LeastSquares.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
#if 0
  template <typename RealT>
  bool Circle2dC<RealT>::FitLSQ(const std::vector<Point<RealT, 2>> &points, RealT &residual)
  {
    auto N = points.size();
    if(N < 3)// Under determined.
      return false;
    MatrixT<RealT> A(N, 3);
    VectorT<RealT> B(N);
    for(int i = 0; i < N; i++) {
      const RealT X = points[i][0];
      const RealT Y = points[i][1];
      A[i][0] = X;
      A[i][1] = Y;
      A[i][2] = 1;
      B[i] = -X * X - Y * Y;
    }
    if(!LeastSquaresQR_IP(A, B, residual))
      return false;// Fit failed.

    const RealT X = B[0] / -2;
    const RealT Y = B[1] / -2;
    Centre() = Point<RealT, 2>({X, Y});
    Radius() = std::sqrt(((X * X) + (Y * Y)) - B[2]);
    ONDEBUG(SPDLOG_INFO("Circle2dC::FitLSQ() Center={} Radius={}", Centre(), Radius()));
    return true;
  }
#endif

  //! @brief Fit points to a circle.
  //! @param circle Circle to fit.
  //! @param points Points to fit.
  //! @return Residual from the least squares fit, or invalid option if it failed.
  //! 'residual' is from the least squares fit and can be used to assess
  //! the quality of the fit.  Returns false if fit failed.
  template <typename RealT>
  std::optional<RealT> fit(Circle2dC<RealT> &circle,const std::vector<Point<RealT, 2>> &points)
  {
    size_t N = points.size();
    if(N < 3)// Under determined.
      return std::nullopt;

    typename MatrixT<RealT>::shape_type sh = {N, 3};
    MatrixT<RealT> A = xt::empty<RealT>(sh);
    VectorT<RealT> B = xt::empty<RealT>({N});
    size_t i = 0;
    auto [mean,scale] = normalise<RealT,2>(points,[&i,&A,&B](const Point<RealT,2> &pnt) {
      const RealT X = pnt[0];
      const RealT Y = pnt[1];
      A(i,0) = X;
      A(i,1) = Y;
      A(i,2) = 1;
      B(i) = -X * X - Y * Y;
      i++;
    });

    auto [x, residual, rank, s] = xt::linalg::lstsq(A, B);
    //SPDLOG_INFO("Rank:{} Residual:{}", int(rank), residual());
    if(rank < 3)
      return std::nullopt;// Fit failed.

    const RealT X = x[0] / -2;
    const RealT Y = x[1] / -2;
    const RealT radius = std::sqrt(((X * X) + (Y * Y)) - x[2]);
    scale = 1 / scale;
    circle = Circle2dC<RealT>((toPoint<RealT>(X, Y) * scale) + mean, radius * scale);
    //SPDLOG_INFO("Circle2dC::FitLSQ() Center={} Radius={}", circle.Centre(), circle.Radius());
    return residual() * scale;
  }


}// namespace Ravl2

#undef DODEBUG
#undef ONDEBUG