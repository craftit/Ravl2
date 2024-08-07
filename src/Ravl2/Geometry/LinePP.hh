// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include <array>
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Assert.hh"

namespace Ravl2
{

  //! Line in N dimensional space define by two points.

  template <typename RealT, unsigned int N>
  class LinePP
  {
  public:
    constexpr LinePP() = default;

    //! Create line from start and end points
    constexpr LinePP(const Point<RealT, N> &start, const Point<RealT, N> &end)
    {
      point[0] = start;
      point[1] = end;
    }
    //: Construct from two points.

    //! Create line from start and end points
    static constexpr LinePP<RealT, N> fromPoints(const Point<RealT, N> &start, const Point<RealT, N> &end)
    {
      return LinePP<RealT, N>(start, end);
    }

    //! Create line from start point and direction
    static constexpr LinePP<RealT, N> fromStartAndDirection(const Point<RealT, N> &start, const Vector<RealT, N> &direction)
    {
      return LinePP<RealT, N>(start, start + direction);
    }

    //! Returns the start point of the line segment.
    constexpr const Point<RealT, N> &FirstPoint() const
    {
      return point[0];
    }

    //! Returns the end point of the line segment.
    constexpr const Point<RealT, N> &SecondPoint() const
    {
      return point[1];
    }

    //! Returns the start point of the line segment.
    constexpr Point<RealT, N> &FirstPoint()
    {
      return point[0];
    }

    //! Returns the end point of the line segment.
    constexpr Point<RealT, N> &SecondPoint()
    {
      return point[1];
    }

    //! Returns the mid point of the line segment.
    constexpr Point<RealT, N> MidPoint() const
    {
      return (point[1] + point[0]) / 2.0;
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function FirstPoint().
    constexpr const Point<RealT, N> &P1() const
    {
      return point[0];
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function SecondPoint().
    constexpr const Point<RealT, N> &P2() const
    {
      return point[1];
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function FirstPoint().
    constexpr Point<RealT, N> &P1()
    {
      return point[0];
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function SecondPoint().
    constexpr Point<RealT, N> &P2()
    {
      return point[1];
    }

    //! Returns the ith point.
    constexpr const Point<RealT, N> &operator[](const unsigned i) const
    {
      RavlAssertMsg(i == 0 || i == 1, "Index out of range 0..1");
      return point[i];
    }

    //! Returns the ith point.
    constexpr Point<RealT, N> &operator[](const unsigned i)
    {
      RavlAssertMsg(i == 0 || i == 1, "Index out of range 0..1");
      return point[i];
    }

    //! Returns the line segment translated into the new position.
    constexpr LinePP<RealT, N> operator+(const Vector<RealT, N> &v) const
    {
      return LinePP<RealT, N>(Point<RealT, N>(P1() + v), Point<RealT, N>(P2() + v));
    }

    //! Moves the line segment into the new position.
    // The operator is equivalent to the member function Translate().
    constexpr LinePP<RealT, N> &operator+=(const Vector<RealT, N> &v)
    {
      point[0] += v;
      point[1] += v;
      return *this;
    }

    //! Moves the line segment into the new position.
    // The member function is equivalent to the operator+=.
    constexpr LinePP<RealT, N> &Translate(const Vector<RealT, N> &v)
    {
      return operator+=(v);
    }

    //! Swaps the end points of this
    constexpr void Swap()
    {
      Point<RealT, N> tmp = point[0];
      point[0] = point[1];
      point[1] = tmp;
    }

    //! Returns a line with swapped endpoints
    constexpr LinePP<RealT, N> Swapped() const
    {
      return LinePP<RealT, N>(P2(), P1());
    }

    //! Get the direction of the line segment as a free vector.
    //! The magnitude of the vector is the length of the line segment.
    //! This was called 'Vector()' in the original code.
    constexpr Vector<RealT, N> direction() const
    {
      return point[1] - point[0];
    }

    //! Translates the line segment to start in the point 'p'.
    constexpr LinePP<RealT, N> &FixStart(const Point<RealT, N> &p)
    {
      Ravl2::Vector<RealT, N> vec = point[1] - point[0];
      point[0] = p;
      point[1] = p + vec;
      return *this;
    }

    //! Translates the line segment to end in the point 'p'.
    constexpr LinePP<RealT, N> &FixEnd(const Point<RealT, N> &p)
    {
      Ravl2::Vector<RealT, N> vec = point[1] - point[0];
      point[0] = p - vec;
      point[1] = p;
      return *this;
    }

    //! Returns the length of the line in euclidian space.
    constexpr RealT Length() const
    {
      return euclidDistance(point[0], point[1]);
    }

    //! Returns the point of the line: FirstPoint() + t * direction().
    constexpr Point<RealT, N> PointAt(const RealT t) const
    {
      return FirstPoint() + direction() * t;
    }

    //! Returns the parameter of the closest point on the line to 'pnt'.
    // Where 0 is at the start point and 1 is at the end.
    constexpr RealT ParClosest(const Point<RealT, N> &pnt) const
    {
      auto v = direction();
      RealT l2 = sqr(v[0]) + sqr(v[1]);
      if(l2 == RealT(0)) throw std::underflow_error("FLinePPC::ParClosest(): Cannot find line parameter for zero-length line");
      return dot(v, pnt - point[0])() / l2;
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
      cereal::size_type size = 2;
      archive(cereal::make_size_tag(size));
      if(size != 2) {
        throw std::runtime_error("Size mismatch");
      }
      ar(point[0], point[1]);
    }

  protected:
    std::array<Point<RealT, N>, 2> point;
  };

  template <typename RealT, unsigned int N>
  inline std::ostream &operator<<(std::ostream &s, const LinePP<RealT, N> &dat)
  {
    s << dat.P1() << ' ' << dat.P2();
    return s;
  }

  template <typename RealT, unsigned int N>
  inline std::istream &operator>>(std::istream &s, LinePP<RealT, N> &dat)
  {
    s >> dat.P1() >> dat.P2();
    return s;
  }

  // Let everyone know there's an implementation already generated for common cases
  extern template class LinePP<float, 2>;
  extern template class LinePP<float, 3>;

}// namespace Ravl2
