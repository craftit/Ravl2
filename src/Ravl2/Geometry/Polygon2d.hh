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
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/PointSet.hh"
#include "Ravl2/Geometry/LinePP2d.hh"

namespace Ravl2
{

  //! Iterator that holds a position and allows for circular iteration, from the end, go to the beginning

  template<class Container>
  class CircularIter
  {
  public:
    //! Constructor
    explicit CircularIter(Container & c)
      : container(c),
        iter(c.begin())
    {
    }

    //! Constructor
    CircularIter(Container & c, typename Container::iterator it)
      : container(c),
        iter(it)
    {
    }

    //! Get a iterator at the last element
    //! The container must not be empty
    static CircularIter Last(Container & c)
    {
      auto iter = c.end();
      assert(c.empty() != c.end());
      iter--;
      return CircularIter(c, iter);
    }

    //! Get a iterator at the first element
    static CircularIter First(Container & c)
    {
      return CircularIter(c, c.begin());
    }

    void GotoLast() {
      iter = container.end();
      --iter;
    }

    void GotoFirst() {
      iter = container.begin();
    }

    //! Compare two iterators
    bool operator==(const CircularIter & other) const
    {
      assert(&container == &other.container);
      return iter == other.iter;
    }

    //! Compare two iterators
    bool operator==(const typename Container::iterator & other) const
    {
      return iter == other;
    }


    //! Compare two iterators
    bool operator!=(const CircularIter & other) const
    {
      assert(&container == &other.container);
      return iter != other.iter;
    }

    //! Access the current element
    auto &Data()
    {
      return *iter;
    }

    //! Circular get the element after the current one
    auto &NextCrcData() {
      auto next = iter;
      ++next;
      if (next == container.end())
        next = container.begin();
      return *next;
    }

    //! Get the data after the current one
    auto &NextData() {
      auto next = iter;
      ++next;
      assert(next != container.end());
      return *next;
    }

    auto &operator++()
    {
      ++iter;
      return *this;
    }

  protected:
    Container & container;
    typename Container::iterator iter;
  };

  //! Helper function to create a circular iterator
  //! This creates an iterator that points to the last valid element
  template<class Container>
  CircularIter<Container> beginCircularLast(Container & c)
  {
    return CircularIter<Container>::Last(c);
  }

  //! Helper function to create a circular iterator
  template<class Container>
  CircularIter<Container> beginCircularFirst(Container & c)
  {
    return CircularIter<Container>::First(c);
  }



  template<class RealT>
  class Moments2;

  //! A polygon in 2d space
  //! The Polygon2dC class is a representation of a polygon in 2-dimensional
  //! space. A polygon is the region of a plane bounded by a finite collection
  //! of line segments forming a simple closed curve. <p>
  //! Ref.: -  O'Rourke,J.: Computational geometry in C;
  //!          Cambridge University Press, 1994, p. 1 <p>

  template<typename RealT>
  class Polygon2dC
    : public PointSet<RealT,2>
  {
  public:
    using PointArrayT = typename PointSet<RealT,2>::PointArrayT;

    //! Empty list of points.
    Polygon2dC() = default;

    //! Construct from list of points
    explicit Polygon2dC(const std::vector<Point<RealT,2>>& points)
      : PointSet<RealT,2>(points)
    {}

    //! Construct from list of points
    explicit Polygon2dC(std::vector<Point<RealT,2>> && points)
      : PointSet<RealT,2>(std::move(points))
    {}

    //! Constructor creates a rectangular polygon of the range
    // The corners of the range are inserted into the polygon in clockwise order
    explicit Polygon2dC(const Range<RealT,2> &range);

    //!return: the signed area of this polygon
    [[nodiscard]] RealT area() const;

    //!return: the moments of the polygon
    [[nodiscard]] Moments2<RealT> Moments() const;

    //! Returns true if (a, b) is a proper internal or external (if allowExternal is true)
    // diagonal of this polygon. The edges incident to 'a' and 'b'
    // are ignored.
    // Ref.: -  O'Rourke,J.: Computational geometry in C;
    //          Cambridge University Press, 1994, pp. 35-36
    [[nodiscard]] bool IsDiagonal(const CircularIter<PointArrayT> & a, const CircularIter<PointArrayT> & b, bool allowExternal = false) const;

    //! Returns true iff the diagonal (a,b) is strictly internal
    // to this polygon in the neighbourhood of the 'a' endpoint.
    // Ref.: -  O'Rourke,J.: Computational geometry in C;
    //          Cambridge University Press, 1994, pp. 37-38
    [[nodiscard]] bool IsInCone(const CircularIter<PointArrayT> & a, const CircularIter<PointArrayT> & b) const;

    //! Clips this polygon by another convex polygon
    //!param: oth - a convex clipping polygon
    //!return: the intersection of the two polygons
    // Note that this only works if the other polygon is convex.
    // Ref.: -  Foley. van Dam. Feiner. Hughes: Computer Graphics Principles and Practice
    //          Addison Wesley Publishing Company, 1996, pp. 123-129
    [[nodiscard]] Polygon2dC<RealT> ClipByConvex(const Polygon2dC<RealT> &oth) const;

    //! Clips this polygon by the line
    //!param: line - a line
    //!return: the clipped polygon so that only the part on the right side of the
    //!return: line remains.
    [[nodiscard]] Polygon2dC<RealT> ClipByLine(const LinePP2dC<RealT> &line) const;

    //! Clips this polygon by the specified axis line through the given point
    //!param: threshold - the threshold for the specified axis
    //!param: axis - we will clip by point[axis]
    //!param: isGreater - determines which side of the axis is accepted
    //!return: the remains of the polygon after clipping
    [[nodiscard]] Polygon2dC<RealT> ClipByAxis(RealT threshold, unsigned axis, bool isGreater) const;

    //! Clip polygon so it lies entirely within 'range'
    // If adjacent points on the polygon map to the same place,
    // one of the points will be removed.
    [[nodiscard]] Polygon2dC<RealT> ClipByRange(const Range<RealT,2> &range) const;

    //! Returns true iff the point 'p' is an internal point of this polygon.
    [[nodiscard]] bool contains(const Point<RealT,2> & p) const;

    //! Returns the perimeter length of this polygon.
    [[nodiscard]] RealT Perimeter() const;

    //! Returns the centroid of this polygon.
    //! This computes the centroid of the area covered by the polygon
    [[nodiscard]]
    Point<RealT,2> Centroid() const;

    //! Return the length of the curve.
    [[nodiscard]] RealT Length() const
    { return Perimeter(); }

    //! Returns true if the polygon is self intersecting, ie do any sides cross
    [[nodiscard]] bool IsSelfIntersecting() const;

    //! Measure the fraction of the polygons overlapping as a fraction of the area of 'poly'
    //!return: 0= Not overlapping 1=This polygon is completely covered by 'poly'.
    [[nodiscard]] RealT Overlap(const Polygon2dC<RealT> &poly) const;

    //! Measure the fraction of the polygons overlapping as a fraction of the larger of the two polygons.
    //!return: 0= Not overlapping 1=If the two polygons are identical.
    [[nodiscard]] RealT CommonOverlap(const Polygon2dC<RealT> &poly) const;

    //! Generate an approximation to the given polygon within the given Euclidean distance limit.
    // The approximation is computed by finding the furthest point from the start, and then
    // the furthest point from that point. The two line segments are then approximated by searching for the
    // furthest point from the line defined by the two end points and if it is further than the distance limit
    // adding it to the approximation. The procedure is then repeated for each of the segments either side
    // of furthest point.
    [[nodiscard]] Polygon2dC<RealT> Approx(RealT distLimit) const;

  };

  //! Generate a convex hull from a set of points.
  template<typename RealT>
  [[nodiscard]] Polygon2dC<RealT> ConvexHull(const std::vector<Point<RealT,2>>& points);

  //! Generate a convex hull from a set of points
  //! The list 'points' is destroyed.
  template<typename RealT>
  [[nodiscard]] Polygon2dC<RealT> ConvexHull(std::vector<Point<RealT,2>>&& points);

}

