// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Assert.hh"

namespace Ravl2
{

  //! Line in N dimensional space define by two points.

  template <typename RealT, unsigned int N>
  class FLinePPC
  {
  public:
    FLinePPC() = default;

    //! Create line from start and end points
    FLinePPC(const Point<RealT, N> &start, const Point<RealT, N> &end)
    {
      point[0] = start;
      point[1] = end;
    }
    //: Construct from two points.

    //! Create line from start and end points
    static FLinePPC<RealT, N> fromPoints(const Point<RealT, N> &start, const Point<RealT, N> &end)
    {
      return FLinePPC<RealT, N>(start, end);
    }

    //! Create line from start point and direction
    static FLinePPC<RealT, N> fromStartAndDirection(const Point<RealT, N> &start, const Vector<RealT, N> &direction)
    {
      return FLinePPC<RealT, N>(start, start + direction);
    }

    //! Returns the start point of the line segment.
    const Point<RealT, N> &FirstPoint() const
    {
      return point[0];
    }

    //! Returns the end point of the line segment.
    const Point<RealT, N> &SecondPoint() const
    {
      return point[1];
    }

    //! Returns the start point of the line segment.
    Point<RealT, N> &FirstPoint()
    {
      return point[0];
    }

    //! Returns the end point of the line segment.
    Point<RealT, N> &SecondPoint()
    {
      return point[1];
    }

    //! Returns the mid point of the line segment.
    Point<RealT, N> MidPoint() const
    {
      return (point[1] + point[0]) / 2.0;
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function FirstPoint().
    const Point<RealT, N> &P1() const
    {
      return point[0];
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function SecondPoint().
    const Point<RealT, N> &P2() const
    {
      return point[1];
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function FirstPoint().
    Point<RealT, N> &P1()
    {
      return point[0];
    }

    //! Returns the start point of the line segment.
    // It is equivalent to the function SecondPoint().
    Point<RealT, N> &P2()
    {
      return point[1];
    }

    const Point<RealT, N> &operator[](const unsigned i) const
    {
      RavlAssertMsg(i == 0 || i == 1, "Index out of range 0..1");
      return point[i];
    }
    //: Returns the ith point.

    Point<RealT, N> &operator[](const unsigned i)
    {
      RavlAssertMsg(i == 0 || i == 1, "Index out of range 0..1");
      return point[i];
    }
    //: Returns the ith point.

    //:-------------------------
    // Geometrical computations.

    FLinePPC<RealT, N> operator+(const Vector<RealT, N> &v) const
    {
      return FLinePPC<RealT, N>(Point<RealT, N>(P1() + v), Point<RealT, N>(P2() + v));
    }
    //: Returns the line segment translated into the new position.

    FLinePPC<RealT, N> &operator+=(const Vector<RealT, N> &v)
    {
      point[0] += v;
      point[1] += v;
      return *this;
    }
    //: Moves the line segment into the new position.
    // The operator is equivalent to the member function Translate().

    FLinePPC<RealT, N> &Translate(const Vector<RealT, N> &v)
    {
      return operator+=(v);
    }
    //: Moves the line segment into the new position.
    // The member function is equivalent to the operator+=.

    void Swap()
    {
      Point<RealT, N> tmp = point[0];
      point[0] = point[1];
      point[1] = tmp;
    }
    //: Swaps the end points of this

    FLinePPC<RealT, N> Swapped() const
    {
      return FLinePPC<RealT, N>(P2(), P1());
    }
    //: Returns a line with swapped endpoints

    //! Get the direction of the line segment as a free vector.
    //! The magnitude of the vector is the length of the line segment.
    //! This was called 'Vector()' in the original code.
    Vector<RealT, N> direction() const
    {
      return point[1] - point[0];
    }

    //! Translates the line segment to start in the point 'p'.
    FLinePPC<RealT, N> &FixStart(const Point<RealT, N> &p)
    {
      Ravl2::Vector<RealT, N> vec = point[1] - point[0];
      point[0] = p;
      point[1] = p + vec;
      return *this;
    }

    //! Translates the line segment to end in the point 'p'.
    FLinePPC<RealT, N> &FixEnd(const Point<RealT, N> &p)
    {
      Ravl2::Vector<RealT, N> vec = point[1] - point[0];
      point[0] = p - vec;
      point[1] = p;
      return *this;
    }

    //! Returns the length of the line in euclidian space.
    RealT Length() const
    {
      return euclidDistance(point[0], point[1]);
    }

    //! Returns the point of the line: FirstPoint() + t * direction().
    Point<RealT, N> PointAt(const RealT t) const
    {
      return FirstPoint() + direction() * t;
    }

    //! Returns the parameter of the closest point on the line to 'pnt'.
    // Where 0 is at the start point and 1 is at the end.
    RealT ParClosest(const Point<RealT, N> &pnt) const
    {
      auto v = direction();
      RealT l2 = sqr(v[0]) + sqr(v[1]);
      if(l2 == RealT(0)) throw std::underflow_error("FLinePPC::ParClosest(): Cannot find line parameter for zero-length line");
      return dot(v, pnt - point[0])() / l2;
    }

  protected:
    std::array<Point<RealT, N>, 2> point;
  };

  template <typename RealT, unsigned int N>
  inline std::ostream &operator<<(std::ostream &s, const FLinePPC<RealT, N> &dat)
  {
    s << dat.P1() << ' ' << dat.P2();
    return s;
  }

  template <typename RealT, unsigned int N>
  inline std::istream &operator>>(std::istream &s, FLinePPC<RealT, N> &dat)
  {
    s >> dat.P1() >> dat.P2();
    return s;
  }

}// namespace Ravl2
