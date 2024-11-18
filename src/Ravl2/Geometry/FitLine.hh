//
// Created by charles on 11/08/24.
//

#pragma once

#include "Ravl2/Geometry/Line2ABC.hh"
#include "Ravl2/Geometry/Moments2.hh"

namespace Ravl2
{
  //! @brief Fit a line to a set of 2D points using least squares.
  //! @param line The line to fit.
  //! @param points The points to fit the line to.
  //! @return The residual of the fit.

  template <typename RealT, typename ContainerT>
    requires std::is_same_v<typename ContainerT::value_type, Point<RealT, 2>>
  constexpr RealT fit(Line2ABC<RealT> &line, const ContainerT &points)
  {
    Moments2<RealT> sums;
    // Ideally we should normalize the points to the centroid and then fit the line
    for(const Point<RealT, 2> &p : points)
      sums += p;
    //SPDLOG_INFO("Line2ABC<RealT> Moments {} from {} points ",sums,points.size());
    RealT sxy = sums.M11() - sums.M00() * sums.template centroid<0>() * sums.template centroid<1>();
    RealT sxx = sums.M20() - sums.M00() * sqr(sums.template centroid<0>());
    RealT syy = sums.M02() - sums.M00() * sqr(sums.template centroid<1>());
    RealT prod = (sxx * syy);
    if(sums.varX() > sums.varY()) {
      RealT b = sxy / sxx;
      RealT a = (sums.M01() - b * sums.M10()) / sums.M00();
      line = Line2ABC<RealT>(-b, 1, -a);
    } else {
      RealT b = sxy / syy;
      RealT a = (sums.M10() - b * sums.M01()) / sums.M00();
      line = Line2ABC<RealT>(1, -b, -a);
    }
    //SPDLOG_INFO("Line2ABC<RealT> = {} Prod:{}",line,prod);
    if(isNearZero(prod))
      return 0;
    return std::sqrt(sqr(sxy) / prod);
  }

}// namespace Ravl2
