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
#include "NSphere.hh"

namespace Ravl2
{
  //! Circle in 2-D space.

  template <typename RealT>
  class Circle
    : public NSphere<RealT,2>
  {
  public:
    //! Construct a circle with radius of 1 centered on the origin.
    constexpr Circle() = default;

    //! Constructor.
    constexpr Circle(const Point<RealT, 2> &at, RealT rad) noexcept
     : NSphere<RealT,2>(at, rad)
    {}

    //! Generate circle from 3 points on its circumference
    constexpr Circle(const Point<RealT, 2> &p1, const Point<RealT, 2> &p2, const Point<RealT, 2> &p3)
    {
      fit(p1, p2, p3);
    }

    //! Fit circle to a set of points.
    constexpr explicit Circle(const std::vector<Point<RealT, 2>> &points)
    {
      (void)points;
      //RealT tmp;
      //FitLSQ(points, tmp);
      RavlAssertMsg(false, "not implemented");
    }

    //! Fit a circle from 3 points on its circumference
    //! Fails if the points are collinear.
    constexpr static std::optional<Circle<RealT>> fit(const Point<RealT, 2> &p0, const Point<RealT, 2> &p1, const Point<RealT, 2> &p2)
    {
      Vector<RealT, 2> a1({p1[1] - p0[1], p0[0] - p1[0]});
      Vector<RealT, 2> a2({p2[1] - p1[1], p1[0] - p2[0]});
      RealT d = a2[0] * a1[1] - a2[1] * a1[0];
      if(isNearZero(d))
        return std::nullopt; // Points are collinear.
      Vector<RealT, 2> np1 = (p0 + p1) / 2.0;
      Vector<RealT, 2> np2 = (p1 + p2) / 2.0;
      RealT m = (a1[0] * (np2[1] - np1[1]) - a1[1] * (np2[0] - np1[0])) / d;
      a2 *= m;
      Point<RealT,2> centre = np2 + a2;
      RealT radius = euclidDistance<RealT, 2>(centre, p0);
      return Circle<RealT>(centre, radius);
    }

    //! Constant access to radius.
    [[nodiscard]] constexpr RealT &Radius()
    {
      return this->mRadius;
    }

    //! Constant access to radius.
    [[nodiscard]] inline constexpr RealT Radius() const
    {
      return this->mRadius;
    }

    //! Centre of circle.
    [[nodiscard]] constexpr Point<RealT, 2> &Centre()
    {
      return this->mCentre;
    }

    //! Constant access to centre of circle.
    [[nodiscard]] constexpr Point<RealT, 2> Centre() const
    {
      return this->mCentre;
    }

    //! Is point inside circle ?
    [[nodiscard]] inline constexpr bool IsInside(const Point<RealT, 2> &point) const
    {
      return squaredEuclidDistance<RealT,2>(this->mCentre, point) < (sqr(this->mRadius));
    }

    //! Find the closest point on the circle to 'point'.
    [[nodiscard]] constexpr Point<RealT, 2> Projection(const Point<RealT, 2> &point) const
    {
      Vector<RealT, 2> dir = point - this->mCentre;
      return this->mCentre + ((this->mRadius / dir.norm()) * dir.array()).matrix();
    }

    //! Angle between origin and point p.
    [[nodiscard]] constexpr RealT Angle(const Point<RealT, 2> &p) const
    {
      return angle<RealT,2>(p, this->mCentre);
    }

    //! Get point on circle at given angle.
    [[nodiscard]] constexpr Point<RealT, 2> Value(RealT angle) const
    {
      return toPoint<RealT>(this->mCentre[0] + this->mRadius * std::cos(angle),
                            this->mCentre[1] + this->mRadius * std::sin(angle));
    }

    //! Distance to closest point on perimeter.
    [[nodiscard]] constexpr RealT Distance(const Point<RealT, 2> &p) const
    {
      return std::abs(euclidDistance(this->mCentre, p) - this->mRadius);
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(cereal::base_class<NSphere<RealT,2>>(this));
    }

  };

  //! @brief Fit points to a circle.
  //! 'residual' is from the least squares fit and can be used to assess
  //! the quality of the fit.  Returns false if fit failed.
  template <typename RealT>
  std::optional<RealT> fit(Circle<RealT> &circle, const std::vector<Point<RealT, 2>> &points);

  // Let everyone know there's an implementation already generated for common cases
  extern template class Circle<float>;

}// namespace Ravl2
