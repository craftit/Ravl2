// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="25/2/1997"

#pragma once

#include "Ravl2/Assert.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  //! Circle in 2-D space.

  template <typename RealT>
  class Circle2dC
  {
  public:
    //! Construct a circle with radius of 1 centered on the origin.
    inline constexpr Circle2dC() = default;

    //! Constructor.
    inline constexpr Circle2dC(const Point<RealT, 2> &at, RealT rad) noexcept
        : centre(at),
          radius(rad)
    {}

    //! Generate circle from 3 points on its circumference
    constexpr Circle2dC(const Point<RealT, 2> &p1, const Point<RealT, 2> &p2, const Point<RealT, 2> &p3)
    {
      Fit(p1, p2, p3);
    }

    //! Fit circle to a set of points.
    inline constexpr explicit Circle2dC(const std::vector<Point<RealT, 2>> &points)
    {
      (void)points;
      //RealT tmp;
      //FitLSQ(points, tmp);
      RavlAssertMsg(false, "not implemented");
    }

    //! Fit a circle from 3 points on its circumference
    //! Returns false if the points are collinear.
    constexpr bool Fit(const Point<RealT, 2> &p0, const Point<RealT, 2> &p1, const Point<RealT, 2> &p2)
    {
      Vector<RealT, 2> a1({p1[1] - p0[1], p0[0] - p1[0]});
      Vector<RealT, 2> a2({p2[1] - p1[1], p1[0] - p2[0]});
      RealT d = a2[0] * a1[1] - a2[1] * a1[0];
      if(isNearZero(d))
        return false;
      Vector<RealT, 2> np1 = (p0 + p1) / 2.0;
      Vector<RealT, 2> np2 = (p1 + p2) / 2.0;
      RealT m = (a1[0] * (np2[1] - np1[1]) - a1[1] * (np2[0] - np1[0])) / d;
      a2 *= m;
      centre = np2 + a2;
      radius = euclidDistance<RealT, 2>(centre, p0);
      return true;
    }

    //! Constant access to radius.
    [[nodiscard]] inline constexpr RealT &Radius()
    {
      return radius;
    }

    //! Constant access to radius.
    [[nodiscard]] inline constexpr RealT Radius() const
    {
      return radius;
    }

    //! Centre of circle.
    [[nodiscard]] inline constexpr Point<RealT, 2> &Centre()
    {
      return centre;
    }

    //! Constant access to centre of circle.
    [[nodiscard]] inline constexpr Point<RealT, 2> Centre() const
    {
      return centre;
    }

    //! Is point inside circle ?
    [[nodiscard]] inline constexpr bool IsInside(const Point<RealT, 2> &point) const
    {
      return squaredEuclidDistance<RealT,2>(centre, point) < (radius * radius);
    }

    //! Find the closest point on the circle to 'point'.
    [[nodiscard]] inline constexpr Point<RealT, 2> Projection(const Point<RealT, 2> &point) const
    {
      Vector<RealT, 2> dir = point - centre;
      return centre + ((radius / dir.norm()) * dir.array()).matrix();
    }

    //! Angle between origin and point p.
    [[nodiscard]] inline constexpr RealT Angle(const Point<RealT, 2> &p) const
    {
      return angle<RealT,2>(p, centre);
    }

    //! Get point on circle at given angle.
    [[nodiscard]] inline constexpr Point<RealT, 2> Value(RealT angle) const
    {
      return toPoint<RealT>(centre[0] + radius * std::cos(angle),
                            centre[1] + radius * std::sin(angle));
    }

    //! Distance to closest point on perimeter.
    [[nodiscard]] inline constexpr RealT Distance(const Point<RealT, 2> &p) const
    {
      return std::abs(euclidDistance(centre, p) - radius);
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("pnt", centre), cereal::make_nvp("r", radius));
    }

  private:
    Point<RealT, 2> centre  = Point<RealT,2>::Zero();
    RealT radius = 0;
  };

  //! @brief Fit points to a circle.
  //! 'residual' is from the least squares fit and can be used to assess
  //! the quality of the fit.  Returns false if fit failed.
  template <typename RealT>
  std::optional<RealT> fit(Circle2dC<RealT> &circle, const std::vector<Point<RealT, 2>> &points);

  // Let everyone know there's an implementation already generated for common cases
  extern template class Circle2dC<float>;

}// namespace Ravl2
