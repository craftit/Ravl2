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

namespace Ravl2
{
  //! @brief Line in 2D space - equation Ax+By+C = 0
  //! The class LineABC2dC represents a line embedded in the 2D plane.
  //! The line is represented by the equation aa*x+bb*y+cc = 0.

  template <typename RealT>
  class LineABC2dC
  {
  public:
    //! Creates a degenerate line (0,0,0).
    inline constexpr LineABC2dC() = default;

    //! Creates the line determined by the equation a*x+b*y+c = 0.
    inline constexpr LineABC2dC(RealT a, RealT b, RealT c)
        : normal({a, b}), d(c)
    {}

    //! Creates the line determined by the equation norm[0]*x+norm[1]*y+c = 0.
    inline constexpr LineABC2dC(Vector<RealT, 2> norm, RealT vd)
        : normal(norm), d(vd)
    {}

    //! Creates the line passing through two points 'end' and 'start'.
    inline constexpr LineABC2dC(const Point<RealT, 2> &start, const Point<RealT, 2> &end)
        : normal(perpendicular(Vector<RealT, 2>(end - start))), d(-dot(this->normal, start)())
    {}

    //! Creates the line passing through two points 'end' and 'start'.
    [[nodiscard]] static constexpr LineABC2dC<RealT> fromPoints(const Point<RealT, 2> &start, const Point<RealT, 2> &end)
    {
      return LineABC2dC<RealT>(start, end);
    }

    //! Creates the line passing through 'pt' with the normal 'norm'.
    [[nodiscard]] static constexpr LineABC2dC<RealT> fromNormal(const Vector<RealT, 2> &norm, const Point<RealT, 2> &pt)
    {
      return LineABC2dC<RealT>(norm, -dot(norm, pt)());
    }

    //! Creates the line passing through 'pt' with the direction 'vec'
    [[nodiscard]] static constexpr LineABC2dC<RealT> fromDirection(const Point<RealT, 2> &pt, const Vector<RealT, 2> &vec)
    {
      auto normal = perpendicular(vec);
      return LineABC2dC<RealT>(normal, -dot(normal, pt));
    }

    //! Returns the normal of the line.
    [[nodiscard]] inline constexpr Vector<RealT, 2> Normal() const
    {
      return this->normal;
    }

    //! Returns the normal of the line normalized to have unit size.
    [[nodiscard]] inline constexpr Vector<RealT, 2> UnitNormal() const
    {
      return this->normal / Ravl2::norm_l2(this->normal);
    }

    //! Returns the distance of the line from the origin of the coordinate
    //! system.
    [[nodiscard]] inline constexpr RealT Rho() const
    {
      return this->d / Ravl2::norm_l2(this->normal);
    }

    //! Returns parameter a.
    [[nodiscard]] inline constexpr RealT A() const
    {
      return this->normal[0];
    }

    //! Returns parameter b.
    [[nodiscard]] inline constexpr RealT B() const
    {
      return this->normal[1];
    }

    //! Returns parameter c.
    [[nodiscard]] inline constexpr RealT C() const
    {
      return this->d;
    }

    //! Returns the value of x coordinate if the y coordinate is known.
    //! If the parameter A() is zero, the zero is returned.
    [[nodiscard]] inline constexpr RealT ValueX(const RealT y) const
    {
      return isNearZero(A()) ? 0 : (-B() * y - C()) / A();
    }

    //! Returns the value of y coordinate if the x coordinate is known.
    //! If the parameter B() is zero, the zero is returned.
    [[nodiscard]] inline constexpr RealT ValueY(const RealT x) const
    {
      return isNearZero(B()) ? 0 : (-A() * x - C()) / B();
    }

    //! Returns the value of the function A()*p[0]+B()*p[1]+C() often
    //! used in geometrical computations.
    [[nodiscard]] inline constexpr RealT Residuum(const Point<RealT, 2> &p) const
    {
      return (this->normal[0] * p[0] + this->normal[1] * p[1]) + this->d;
    }

    //! Normalizes the equation so that the normal vector is unit.
    inline constexpr LineABC2dC &MakeUnitNormal()
    {
      RealT size = Ravl2::norm_l2(this->normal);
      this->normal /= size;
      this->d /= size;
      return *this;
    }

    //! Returns true if the lines are parallel.
    [[nodiscard]] inline constexpr bool AreParallel(const LineABC2dC &line) const {
      RealT crossSize = cross(Normal(), line.Normal());
      return isNearZero(crossSize);
    }

    //! Find the intersection of two lines.
    //! If the intersection doesn't exist, the function returns false.
    //! The intersection is assigned to 'here'.
    inline constexpr bool Intersection(const LineABC2dC &line, Point<RealT, 2> &here) const
    {
      RealT crossSize = cross(Normal(),line.Normal());
      if(isNearZero(crossSize))
        return false;
      here = toPoint<RealT>((line.C() * B() - line.B() * C()) / crossSize,
                             (line.A() * C() - line.C() * A()) / crossSize);
      return true;
    }

    //! Returns the intersection of both lines.
    //! If the intersection
    //! doesn't exist, the function returns Point<RealT,2>(0,0).
    [[nodiscard]] inline constexpr Point<RealT, 2> Intersection(const LineABC2dC &line) const
    {
      RealT crossSize = cross(Normal(), line.Normal());
      if(isNearZero(crossSize))
        return toPoint<RealT>(0.0, 0.0);
      return toPoint<RealT>((line.C() * B() - line.B() * C()) / crossSize,
                             (line.A() * C() - line.C() * A()) / crossSize);
    }

    //! Returns the squared Euclidean distance of the 'point' from the line.
    [[nodiscard]] inline constexpr RealT SqrEuclidDistance(const Point<RealT, 2> &point) const
    {
      RealT t = Residuum(point);
      return sqr(t) / sumOfSqr(normal);
    }


    //! Returns the signed distance of the 'point' from the line.
    //! The return value is greater than 0 if the point is on the left
    //! side of the line. The left side of the line is determined
    //! by the direction of the normal.
    [[nodiscard]] inline constexpr RealT SignedDistance(const Point<RealT, 2> &point) const
    {
      return Residuum(point) / norm_l2(normal);
    }

    //! Returns the distance of the 'point' from the line.
    [[nodiscard]] inline constexpr RealT Distance(const Point<RealT, 2> &point) const
    {
      return std::abs(SignedDistance(point));
    }

    //! Returns the point which is the orthogonal projection of the 'point' to the line.
    //! It is the same as intersection of this line with
    //! the perpendicular line passing through the 'point'.
    [[nodiscard]] inline constexpr Point<RealT, 2> Projection(const Point<RealT, 2> &point) const
    {
      return point - normal * (Residuum(point) / sumOfSqr(normal));
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(normal[0], normal[1], d);
    }

  private:
    Vector<RealT, 2> normal {};
    // The normal of the line.

    RealT d = 0.0;
    // The distance of the line from the origin of the coordinate system
    // multiplied by the size of the normal vector of the line.
  };

  template <typename RealT>
  std::ostream &operator<<(std::ostream &outS, const LineABC2dC<RealT> &line) {
    outS << line.A() << ' ' << line.B() << ' ' << line.C();
    return(outS);
  }

  template <typename RealT>
  std::istream &operator>>(std::istream &inS, LineABC2dC<RealT> &line)  {
    RealT a, b, c;
    inS >> a >> b >> c;
    line = LineABC2dC<RealT>(a, b, c);
    return (inS);
  }


  // Let everyone know there's an implementation already generated for common cases
  extern template class LineABC2dC<float>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT>
struct fmt::formatter<Ravl2::LineABC2dC<RealT> > : fmt::ostream_formatter {
};
#endif
