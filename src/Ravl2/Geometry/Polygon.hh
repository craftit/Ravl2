// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26/9/2002"
#pragma once

#include <vector>
#include "Ravl2/LoopIter.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/PointSet.hh"
#include "Ravl2/Geometry/Line2PP.hh"

namespace Ravl2
{

  template <class RealT>
  class Moments2;

  //! A polygon in 2d space
  //! The Polygon class is a representation of a polygon in 2-dimensional
  //! space. A polygon is the region of a plane bounded by a finite collection
  //! of line segments forming a simple closed curve. <p>
  //! Ref.: -  O'Rourke,J.: Computational geometry in C;
  //!          Cambridge University Press, 1994, p. 1 <p>

  template <typename RealT>
  class Polygon : public PointSet<RealT, 2>
  {
  public:
    using PointT = Point<RealT, 2>;
    using PointArrayT = typename PointSet<RealT, 2>::PointArrayT;
    using interator = typename PointSet<RealT, 2>::iterator;
    using value_type = typename PointSet<RealT, 2>::value_type;

    //! Empty list of points.
    constexpr Polygon() = default;
    
    //! Move from a list of points
    constexpr Polygon(PointSet<RealT,2> &&pnts)
      : PointSet<RealT,2>(std::move(pnts))
    {}
    
    //! Assign from a list of points
    constexpr Polygon &operator=(PointSet<RealT,2> &&pnts)
    {
      PointSet<RealT,2>::operator=(std::move(pnts));
      return *this;
    }
    
    //! Construct from list of points
    constexpr explicit Polygon(const std::vector<Point<RealT, 2>> &points)
        : PointSet<RealT, 2>(points)
    {}

    //! Construct from list of points
    constexpr explicit Polygon(std::vector<Point<RealT, 2>> &&points)
        : PointSet<RealT, 2>(std::move(points))
    {}

    //! Construct from a fixed array of points
    template <size_t N>
    constexpr explicit Polygon(const std::array<Point<RealT, 2>, N> &points)
      : PointSet<RealT, 2>(points)
    {}

    //! From initializer list
    Polygon(std::initializer_list<Point<RealT,2>> list)
      : PointSet<RealT,2>(list)
    {}

    //! Constructor creates a rectangular polygon of the range
    //! @param: range - the range defining the rectangle for the polygon
    //! @param: orientation - the orientation of the boundary
    //! If BoundaryOrientationT::INSIDE_LEFT makes a counter clockwise polygon, with a positive area.
    explicit Polygon(const Range<RealT, 2> &range, BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT);
    
    //! Constructor creates a rectangular polygon of the index range
    //! @param: range - the range defining the rectangle for the polygon
    //! @param: orientation - the orientation of the boundary
    //! If BoundaryOrientationT::INSIDE_LEFT makes a counter clockwise polygon, with a positive area.
    explicit Polygon(const IndexRange<2> &range, BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT)
     : Polygon(toRange<RealT, 2>(range), orientation)
    {}
    
    //! @return: the signed area of this polygon
    //! @param: type - the orientation of the boundary inverts sign.
    [[nodiscard]] RealT area() const;

    //! @brief Test if the polygon is convex
    //! @return: true if the polygon is convex
    [[nodiscard]] bool isConvex(BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT) const;

    //! @brief Clips this polygon by a convex polygon
    //! @param: oth - a convex clipping polygon
    //! @return: the intersection of the two polygons
    //! Note that this only works if the other polygon is convex.
    //! Ref.: -  Foley. van Dam. Feiner. Hughes: Computer Graphics Principles and Practice
    //!         Addison Wesley Publishing Company, 1996, pp. 123-129
    [[nodiscard]] Polygon<RealT> clipByConvex(const Polygon<RealT> &oth, BoundaryOrientationT othOrientation = BoundaryOrientationT::INSIDE_LEFT) const;

    //! @brief Clips this polygon by the line
    //! @param: line - a line
    //! @return: the clipped polygon so that only the part on the right side of the
    [[nodiscard]] Polygon<RealT> clipByLine(const Line2PP<RealT> &line, BoundaryOrientationT lineOrientation = BoundaryOrientationT::INSIDE_LEFT) const;

    //! @brief Clips this polygon by the specified axis line through the given point
    //! @param: threshold - the threshold for the specified axis
    //! @param: axis - we will clip by point[axis]
    //! @param: isGreater - determines which side of the axis is accepted
    //! @return: the remains of the polygon after clipping, maybe empty
    [[nodiscard]] Polygon<RealT> clipByAxis(RealT threshold, unsigned axis, bool isGreater) const;

    //! @brief Clip polygon so it lies entirely within 'range'
    //! If adjacent points on the polygon map to the same place,
    //! one of the points will be removed.
    [[nodiscard]] Polygon<RealT> clipByRange(const Range<RealT, 2> &range) const;

    //! Returns true iff the point 'p' is an internal point of this polygon.
    [[nodiscard]] bool contains(const Point<RealT, 2> &p) const;

    //! Returns the centroid of this polygon.
    //! This computes the centroid of the area covered by the polygon
    [[nodiscard]] Point<RealT, 2> centroid() const;

    //! Returns true if the polygon is self intersecting, ie do any sides cross
    [[nodiscard]] bool isSelfIntersecting() const;

    //! @brief Measure the fraction of the polygons overlapping as a fraction of the area of 'poly'
    //! This requires that the polygons are convex.
    //! @return: 0= Not overlapping 1=This polygon is completely covered by 'poly'.
    [[nodiscard]] RealT overlap(const Polygon &poly) const;

    //! @brief Measure the fraction of the polygons overlapping as a fraction of the larger of the two polygons.
    //! This requires that the polygons are convex.
    //! @return: 0= Not overlapping 1=If the two polygons are identical.
    [[nodiscard]] RealT commonOverlap(const Polygon<RealT> &poly) const;

    //! @brief Generate an approximation to the given polygon within the given Euclidean distance limit.
    //! The approximation is computed by finding the furthest point from the start, and then
    //! the furthest point from that point. The two line segments are then approximated by searching for the
    //! furthest point from the line defined by the two end points and if it is further than the distance limit
    //! adding it to the approximation. The procedure is then repeated for each of the segments either side
    //! of furthest point.
    [[nodiscard]] Polygon<RealT> approx(RealT distLimit) const;

    //! @brief Make a reversed copy of the polygon
    [[nodiscard]] Polygon<RealT> reverse() const
    {
      Polygon<RealT> ret;
      ret.reserve(this->size());
      for(auto it = this->rbegin(); it != this->rend(); ++it)
        ret.push_back(*it);
      return ret;
    }

    //! Add a point checking it isn't a duplicate of the last one.
    void addBack(const Point<RealT, 2> &pnt);

  };

  //! Let the compiler know that we will use these classes with the following types
  extern template class Polygon<float>;
  extern template class Polygon<double>;

  //! @brief Compute the moments of a polygon
  //! see http://www9.in.tum.de/forschung/fgbv/tech-reports/1996/FGBV-96-04-Steger.pdf for details
  template <typename RealT>
  [[nodiscard]] Moments2<RealT> moments(const Polygon<RealT> &poly);

  //! Generate a convex hull from a set of points.
  template <typename RealT>
  [[nodiscard]] Polygon<RealT> ConvexHull(const std::vector<Point<RealT, 2>> &points);

  //! Generate a convex hull from a set of points
  //! The list 'points' is destroyed.
  template <typename RealT>
  [[nodiscard]] Polygon<RealT> ConvexHull(std::vector<Point<RealT, 2>> &&points);

  //! Convert a range to a polygon.
  //! @param: range - the range defining the rectangle for the polygon
  //! @param: orientation - the orientation of the boundary
  //! If BoundaryOrientationT::INSIDE_LEFT makes a counter clockwise polygon, with a positive area.
  template <typename RealT>
  [[nodiscard]] Polygon<RealT> toPolygon(const Range<RealT, 2> &range, BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT)
  {
    return Polygon<RealT>(range, orientation);
  }

  //! Convert a range to a polygon.
  //! @param: points - Fixed size array of points
  //! @param: orientation - the orientation of the boundary
  //! If BoundaryOrientationT::INSIDE_LEFT makes a counter clockwise polygon, with a positive area.
  template <typename RealT,size_t N>
  [[nodiscard]] Polygon<RealT> toPolygon(const std::array<Point<RealT, 2>, N> &points)
  {
    return Polygon<RealT>(points);
  }

  //! Convert an IndexRange to a polygon
  //! @param: range - the range defining the rectangle for the polygon
  //! @param: orientation - the orientation of the boundary
  //! If BoundaryOrientationT::INSIDE_LEFT makes a counter clockwise polygon, with a positive area.
  template <typename RealT>
  [[nodiscard]] Polygon<RealT> toPolygon(const IndexRange<2> &range, BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT)
  {
    return Polygon<RealT>(toRange<RealT>(range), orientation);
  }

  //! Put a polygon through a transformation
  template <typename RealT,typename TransformT>
   requires PointTransform<TransformT>
  [[nodiscard]] Polygon<RealT> operator*(const TransformT &trans, const Polygon<RealT> &poly)
  {
    Polygon<RealT> ret;
    ret.reserve(poly.size());
    for(const auto &p : poly)
      ret.push_back(trans(p));
    return ret;
  }
}// namespace Ravl2

#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<Ravl2::Polygon<float>> : fmt::ostream_formatter {
};
#endif
