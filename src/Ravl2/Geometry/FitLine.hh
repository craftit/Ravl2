//
// Created by charles on 11/08/24.
//


#pragma once

#include "Ravl2/Geometry/LineABC2d.hh"
#include "Ravl2/Geometry/Moments2.hh"

namespace Ravl2
{
  //! @brief Fit a line to a set of 2D points using least squares.
  //! @param line The line to fit.
  //! @param points The points to fit the line to.
  //! @return The residual of the fit.

  template <typename RealT,typename ContainerT>
  requires std::is_same_v<typename ContainerT::value_type,Point<RealT,2>>
  RealT fitLSQ(LineABC2dC<RealT> &line,const ContainerT &points)
  {
    Moments2<RealT> sums;
    for(const Point<RealT,2> &p : points)
      sums += p;
    RealT sxy = sums.M11()  - sums.M00() * sums.CentroidX() * sums.CentroidY();
    RealT sxx = sums.M20() - sums.M00() * sqr(sums.CentroidX());
    RealT syy = sums.M02() - sums.M00() * sqr(sums.CentroidY());
    RealT prod = (sxx * syy);
    if(sums.VarX() > sums.VarY()) {
      RealT b = sxy/sxx;
      RealT a = (sums.M01() - b * sums.M10())/sums.M00();
      line = LineABC2dC(-b, 1.0,-a);
    } else {
      RealT b = sxy/syy;
      RealT a = (sums.M10() - b * sums.M01())/sums.M00();
      line = LineABC2dC(1.0,-b,-a);
    }
    if(isNearZero(prod))
      return 0;
    return std::sqrt(sqr(sxy) / prod);
  }

} // Ravl2
