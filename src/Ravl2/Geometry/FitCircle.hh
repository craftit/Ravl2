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

  //! @brief Fit points to a circle.
  //! @param circle Circle to fit.
  //! @param points Points to fit.
  //! @return Residual from the least squares fit, or invalid option if it failed.
  //! 'residual' is from the least squares fit and can be used to assess
  //! the quality of the fit.  Returns false if fit failed.
  template <typename RealT>
  std::optional<RealT> fit(Circle<RealT> &circle, const std::vector<Point<RealT, 2>> &points)
  {
    size_t numSamples = points.size();
    if(numSamples < 3)// Under determined.
      return std::nullopt;

    MatrixT<RealT> A(numSamples, 3);
    VectorT<RealT> B(numSamples);
    IndexT i = 0;
    auto [mean, scale] = normalise<RealT, 2>(points, [&i, &A, &B](const Point<RealT, 2> &pnt) {
      const RealT X = pnt[0];
      const RealT Y = pnt[1];
      A(i, 0) = X;
      A(i, 1) = Y;
      A(i, 2) = 1;
      B(i) = -X * X - Y * Y;
      i++;
    });

    //auto [x, residual, rank, s] = xt::linalg::lstsq(A, B);
    //SPDLOG_INFO("A:{} \n B:{}", A,B);
    auto solver = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV);
    auto x = solver.solve(B);

    const RealT X = x[0] / -2;
    const RealT Y = x[1] / -2;
    const RealT radius = std::sqrt(((X * X) + (Y * Y)) - x[2]);
    scale = 1 / scale;
    circle = Circle<RealT>((toPoint<RealT>(X, Y) * scale) + mean, radius * scale);
    //SPDLOG_INFO("Circle2dC::FitLSQ() Center={} Radius={}", circle.Centre(), circle.Radius());
    return 0;//residual() * scale;
  }

}// namespace Ravl2

#undef DODEBUG
#undef ONDEBUG
