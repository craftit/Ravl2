// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="13/9/2002"
//! author="Charles Galambos"

#pragma once

#include <optional>
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/LinePP.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{

  //! Line defined by 2 points in 2-dimensional space.

  template <typename RealT>
  class LinePP2dC : public LinePP<RealT, 2>
  {
  public:
    LinePP2dC() = default;
    //! Default constructor.
    // The contents of the line are undefined.

    explicit LinePP2dC(const LinePP<RealT, 2> &Base)
        : LinePP<RealT, 2>(Base)
    {}
    //! Constructor from base class

    LinePP2dC(const Point<RealT, 2> &Start, const Point<RealT, 2> &End)
        : LinePP<RealT, 2>(Start, End)
    {}
    //! Construct a line from two points.

    //! Create line from start and end points
    // cppcheck-suppress duplInheritedMember
    static LinePP2dC<RealT> fromPoints(const Point<RealT, 2> &start, const Point<RealT, 2> &end)
    {
      return {start, end};
    }

    //! Create line from start point and direction
    // cppcheck-suppress duplInheritedMember
    static LinePP2dC<RealT> fromStartAndDirection(const Point<RealT, 2> &start, const Vector<RealT, 2> &direction)
    {
      return {start, start + direction};
    }

    //! @brief Clip line by given rectangle.
    //! If no part of the line is in the rectangle:<ul>
    //! <li>line is <i>not</i> clipped</li>
    //! <li>method returns <code>false</code></li></ul>
    bool clipBy(const Range<RealT, 2> &rng);

    //! @brief Checks if this point is to the right of the line
    [[nodiscard]] bool IsPointToRight(const Point<RealT, 2> &Pt) const
    {
      return triangleArea2(Pt, this->P1(), this->P2()) < 0;
    }

    //! @brief Checks if this point is to the right of, or exactly on the line
    [[nodiscard]] bool IsPointToRightOn(const Point<RealT, 2> &Pt) const
    {
      return triangleArea2(Pt, this->P1(), this->P2()) <= 0;
    }

    //! @brief Checks if this point is to the left of the line
    [[nodiscard]] bool IsPointToLeft(const Point<RealT, 2> &Pt) const
    {
      return triangleArea2(Pt, this->P1(), this->P2()) > 0;
    }

    //! @brief Checks if this point is to the left of, or exactly on the line
    [[nodiscard]] bool IsPointToLeftOn(const Point<RealT, 2> &Pt) const
    {
      return triangleArea2(Pt, this->P1(), this->P2()) >= 0;
    }

    //! @brief Test if the point is on the 'inside' or on the line.
    //! @param Pt - the point to test
    //! @param orientation - the orientation of the boundary, is inside left or right
    //! @return true if the point is inside or on the line
    [[nodiscard]] bool IsPointInsideOn(const Point<RealT, 2> &Pt, BoundaryOrientationT orientation) const
    {
      auto area = triangleArea2(Pt, this->P1(), this->P2());
      if(orientation == BoundaryOrientationT::INSIDE_RIGHT) {
	return area <= 0;
      }
      return area >= 0;
    }

    //! @brief Checks if this point is exactly on the line within a tolerance
    [[nodiscard]] bool IsPointOn(const Point<RealT, 2> &Pt, RealT tolerance = std::numeric_limits<RealT>::epsilon()) const
    {
      return std::abs(triangleArea2(Pt, this->P1(), this->P2())) <= tolerance;
    }

    //! Checks if this point is exactly on the closed segment within a tolerance
    [[nodiscard]] bool IsPointIn(const Point<RealT, 2> &Pt, RealT tolerance = std::numeric_limits<RealT>::epsilon()) const;

    //! @brief Returns the parameter of the intersection point of 'l' with this line.
    //! If the parameter is equal to 0, the intersection is the starting
    //! point of this line, if the parameter is 1, the intersection is the
    //! end point. If the parameter is between 0 and 1 the intersection is
    //! inside of this line segment.
    //! @param l - another line
    //! @return the parameter of the intersection point on this line.
    [[nodiscard]] RealT ParIntersection(const LinePP2dC &l) const;

    //! @brief Returns true if the intersection of this line segment and the line 'l' is either inside of this line segment or one of the end points.
    [[nodiscard]] bool HasInnerIntersection(const LinePP2dC &l) const;

    // Returns the intersection of 2 lines.
    [[nodiscard]] Point<RealT, 2> Intersection(const LinePP2dC &l) const;

    //! Calculate the intersection point between this line and l
    //! @param l - another line
    //! @param here - the point of intersection
    //! @return true if the lines intersect or false if they are parallel
    [[nodiscard]] bool Intersection(const LinePP2dC &L, Point<RealT, 2> &Here) const;

    //! Calculate the intersection point between this line and l within this segment limits
    //! @param l - another line
    //! @return a point if the lines intersect within the segment limits, otherwise nullopt
    [[nodiscard]] std::optional<Point<RealT, 2> > innerIntersection(const LinePP2dC &l) const;

    //! Find the column position which intersects the given row.
    //! @param row - Row for which we want to find the intersecting column
    //! @param col - Place to store the intersecting col.
    //! @return True if position exists, false if there is no intersection
    [[nodiscard]] bool IntersectRow(RealT row, RealT &col) const;

    //! Return the direction of the line.
    [[nodiscard]] RealT Angle() const
    {
      auto dir = this->P2() - this->P1();
      return std::atan2(dir[1], dir[0]);
    }

    //! Returns the normal of the line.
    [[nodiscard]] Vector<RealT, 2> Normal() const
    {
      return toVector<RealT>(this->point[1][1] - this->point[0][1], this->point[0][0] - this->point[1][0]);
    }

    //! Returns the normal of the line normalized to have unit size.
    [[nodiscard]] Vector<RealT, 2> UnitNormal() const
    {
      auto normal = this->Normal();
      return normal / xt::linalg::norm(normal, 2);
    }

    //! Returns signed perpendicular distance of pt from this line
    [[nodiscard]] RealT SignedDistance(const Point<RealT, 2> Pt) const
    {
      auto dir = this->direction();
      return cross<RealT>(dir, Pt - this->P1()) / xt::linalg::norm(dir, 2);
    }

    //! Return unsigned perpendicular distance of pt from this line
    [[nodiscard]] RealT Distance(const Point<RealT, 2> Pt) const
    {
      return std::abs(SignedDistance(Pt));
    }

    //! Returns intersection of line with perpendicular from Pt to line
    [[nodiscard]] Point<RealT, 2> Projection(const Point<RealT, 2> &Pt) const
    {
      return Point<RealT, 2>(this->ParClosest(Pt));
    }

    //! Returns distance of pt to nearest point on the line within the segment limits
    [[nodiscard]] RealT DistanceWithin(const Point<RealT, 2> &pt) const;
  };

  //! @brief Clip line by given rectangle.
  //! Optional will be empty if the line is entirely outside the rectangle.
  template <typename RealT>
  [[nodiscard]]  inline std::optional<LinePP<RealT, 2> > clip(LinePP<RealT, 2> aLine,const Range<RealT, 2> &rng) {
    LinePP2dC<RealT> line(aLine);
    if (!line.clipBy(rng)) {
      return std::nullopt;
    }
    return line;
  }

  //! @brief Find the intersection of two lines.
  //! Optional will be empty if the lines are parallel.
  template <typename RealT>
  [[nodiscard]] inline std::optional<Point<RealT, 2> >  intersection(const LinePP2dC<RealT> &l1, const LinePP2dC<RealT> &l2)
  {
    Point<RealT, 2> here;
    if (!l1.Intersection(l2, here)) {
      return std::nullopt;
    }
    return here;
  }


  extern template class LinePP2dC<double>;
  extern template class LinePP2dC<float>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<Ravl2::LinePP2dC<float>> : fmt::ostream_formatter {
};
template <>
struct fmt::formatter<Ravl2::LinePP2dC<double>> : fmt::ostream_formatter {
};
#endif
