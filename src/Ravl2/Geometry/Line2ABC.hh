// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.06.1994"

#pragma once

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/VectorOffset.hh"
#include "Range.hh"

namespace Ravl2
{
  //! @brief Line in 2D space - equation Ax+By+C = 0
  //! The class Line2ABC represents a line embedded in the 2D plane.
  //! The line is represented by the equation aa*x+bb*y+cc = 0.

  template <typename RealT>
  class Line2ABC
     : public VectorOffset<RealT, 2>
  {
  public:
    //! Creates a degenerate line (0,0,0).
    inline constexpr Line2ABC() = default;

    //! Creates the line determined by the equation a*x+b*y+c = 0.
    inline constexpr Line2ABC(const VectorOffset<RealT, 2> &vecOff)
      : VectorOffset<RealT, 2>(vecOff)
    {}

    //! Creates the line determined by the equation a*x+b*y+c = 0.
    inline constexpr Line2ABC(RealT a, RealT b, RealT c)
        : VectorOffset<RealT, 2>({a, b},c)
    {}

    //! Creates the line determined by the equation norm[0]*x+norm[1]*y+c = 0.
    inline constexpr Line2ABC(Vector<RealT, 2> norm, RealT vd)
        : VectorOffset<RealT, 2>(norm,vd)
    {}

    //! Creates the line passing through two points 'end' and 'start'.
    inline constexpr Line2ABC(const Point<RealT, 2> &start, const Point<RealT, 2> &end)
    {
      this->mNormal = perpendicular(Vector<RealT, 2>(end - start));
      this->mD = -this->mNormal.dot(start);
    }

    //! Creates the line passing through two points 'end' and 'start'.
    [[nodiscard]] static constexpr Line2ABC<RealT> fromPoints(const Point<RealT, 2> &start, const Point<RealT, 2> &end)
    {
      return Line2ABC<RealT>(start, end);
    }

    //! Creates the line passing through 'pt' with the normal 'norm'.
    [[nodiscard]] static constexpr Line2ABC<RealT> fromNormal(const Vector<RealT, 2> &normal, const Point<RealT, 2> &pt)
    {
      return Line2ABC<RealT>(normal, RealT(-normal.dot(pt)));
    }

    //! Creates the line passing through 'pt' with the direction 'vec'
    [[nodiscard]] static constexpr Line2ABC<RealT> fromDirection(const Point<RealT, 2> &pt, const Vector<RealT, 2> &vec)
    {
      auto normal = perpendicular(vec);
      return Line2ABC<RealT>(normal, RealT(-normal.dot(pt)));
    }
    
    //! Returns the normal of the line normalized to have unit size.
    [[nodiscard]] inline constexpr Vector<RealT, 2> unitNormal() const
    {
      return this->mNormal / this->mNormal.norm();
    }

    //! Returns the distance of the line from the origin of the coordinate
    //! system.
    [[nodiscard]] inline constexpr RealT rho() const
    {
      return this->mD / this->mNormal.norm();
    }

    //! Returns parameter a.
    [[nodiscard]] inline constexpr RealT A() const
    {
      return this->mNormal[0];
    }

    //! Returns parameter b.
    [[nodiscard]] inline constexpr RealT B() const
    {
      return this->mNormal[1];
    }

    //! Returns parameter c.
    [[nodiscard]] inline constexpr RealT C() const
    {
      return this->mD;
    }

    //! Returns the value of x coordinate if the y coordinate is known.
    //! If the parameter A() is zero, the zero is returned.
    [[nodiscard]] inline constexpr RealT valueX(const RealT y) const
    {
      return isNearZero(A()) ? 0 : (-B() * y - C()) / A();
    }

    //! Returns the value of y coordinate if the x coordinate is known.
    //! If the parameter B() is zero, the zero is returned.
    [[nodiscard]] inline constexpr RealT valueY(const RealT x) const
    {
      return isNearZero(B()) ? 0 : (-A() * x - C()) / B();
    }
    
    //! Returns true if the lines are parallel.
    [[nodiscard]] inline constexpr bool isParallel(const Line2ABC &line) const
    {
      RealT crossSize = cross(this->normal(), line.normal());
      return isNearZero(crossSize);
    }

    //! Find the intersection of two lines.
    //! If the intersection doesn't exist, the function returns false.
    //! The intersection is assigned to 'here'.
    inline constexpr bool intersection(const Line2ABC &line, Point<RealT, 2> &here) const
    {
      RealT crossSize = cross(this->normal(), line.normal());
      if(isNearZero(crossSize))
        return false;
      here = toPoint<RealT>((line.C() * B() - line.B() * C()) / crossSize,
                            (line.A() * C() - line.C() * A()) / crossSize);
      return true;
    }

    //! Returns the intersection of both lines.
    //! If the intersection
    //! doesn't exist, the function returns Point<RealT,2>(0,0).
    [[nodiscard]] inline constexpr Point<RealT, 2> intersection(const Line2ABC &line) const
    {
      RealT crossSize = cross(this->normal(), line.normal());
      if(isNearZero(crossSize))
        return toPoint<RealT>(0.0, 0.0);
      return toPoint<RealT>((line.C() * B() - line.B() * C()) / crossSize,
                            (line.A() * C() - line.C() * A()) / crossSize);
    }

    //! Returns the squared Euclidean distance of the 'point' from the line.
    [[nodiscard]] inline constexpr RealT sqrEuclidDistance(const Point<RealT, 2> &point) const
    {
      RealT t = this->residuum(point);
      return sqr(t) / sumOfSqr(this->mNormal);
    }
    
    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(cereal::base_class<VectorOffset<RealT, 2>>(this));
    }
  };

  template <typename RealT>
  std::ostream &operator<<(std::ostream &outS, const Line2ABC<RealT> &line)
  {
    outS << line.A() << ' ' << line.B() << ' ' << line.C();
    return (outS);
  }

  template <typename RealT>
  std::istream &operator>>(std::istream &inS, Line2ABC<RealT> &line)
  {
    RealT a, b, c;
    inS >> a >> b >> c;
    line = Line2ABC<RealT>(a, b, c);
    return (inS);
  }

  //! Test if a line intersects a range (or rectangle)
  template<typename RealT>
  bool intersects(const Line2ABC<RealT> &line, const Range<RealT, 2> &rng1)
  {
    Point<RealT, 2> here;
    // This could be faster...
    Line2ABC<RealT> lineMin1({0, 1}, rng1.range(1).min());
    if(line.intersection(lineMin1, here)) {
      if(rng1.range(0).contains(here(0)))
        return true;
    }
    Line2ABC<RealT> lineMax1({0, 1}, rng1.range(1).max());
    if(line.intersection(lineMax1, here)) {
      if(rng1.range(0).contains(here(0)))
        return true;
    }
    Line2ABC<RealT> lineMin0({1, 0}, rng1.range(0).min());
    if(line.intersection(lineMin0, here)) {
      if(rng1.range(1).contains(here(1)))
        return true;
    }
    Line2ABC<RealT> lineMax0({1, 0}, rng1.range(0).max());
    if(line.intersection(lineMax0, here)) {
      if(rng1.range(1).contains(here(1)))
        return true;
    }
    return false;
  }

  // Let everyone know there's an implementation already generated for common cases
  extern template class Line2ABC<float>;

//  //! Construct a line from two points
//  template <typename RealT>
//  [[nodiscard]] inline constexpr Line2ABC<RealT> toLine(Point<RealT, 2> const &start, Point<RealT, 2> const &end)
//  {
//    return Line2ABC<RealT>(start, end);
//  }
}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT>
struct fmt::formatter<Ravl2::Line2ABC<RealT>> : fmt::ostream_formatter {
};
#endif
