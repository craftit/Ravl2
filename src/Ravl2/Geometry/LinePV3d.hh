// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! docentry="Ravl.API.Math.Geometry.3D"
//! date="06/08/1995"

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

//  class LinePP3dC;
//  class PlaneABCD3dC;

  //: Line determined by one point and a vector in 3D space
  // The LinePV3dC class represents the line in 3 dimensional Euclidian
  // space. The line is represented by one point and its direction vector.

  template<typename RealT>
  class LinePV3dC
  {
  public:
    //:----------------------------------------------
    // Constructors, assigment, copy, and destructor.

    //: Creates the non-existing line (0,0,0) [0,0,0].
    inline LinePV3dC() = default;


    inline LinePV3dC(const Point<RealT,3> &a, const Vector<RealT,3> &v)
        : point(a), direction(v)
    {}
    //: Creates the line passing through the point 'a' and with
    //: the direction 'v'.

//    //: Creates the line passing through the points 'first' and
//    //: second.
//    inline LinePV3dC(const Point<RealT,3> &first, const Point<RealT,3> &second)
//        : point(first), direction(second - first)
//    {}

    //:-------------------------------
    // Access to elements of the line.

    inline const Point<RealT,3> &FirstPoint() const
    {
      return point;
    }
    //: Returns the point of the line.

    inline Point<RealT,3> SecondPoint() const
    {
      return point + direction;
    }
    //: Returns the point which is the end point of the direction
    //: vector placed in the first point of the line.

    inline Point<RealT,3> MiddlePoint() const
    {
      return PointT(0.5);
    }
    //: Returns the point which is in the middle of the FirstPoint()
    //: and the SecondPoint() of the line segment.

    Point<RealT,3> PointT(RealT lambda) const
    {
      return point + direction * lambda;
    }
    //: Returns the point FirstPoint() + lambda * direction.
    // Obsolete, use Point(l)

    Point<RealT,3> at(RealT lambda) const
    {
      return point + direction * lambda;
    }
    //: Returns the point FirstPoint() + lambda * direction.

    RealT ParClosest(const Point<RealT,3> &pnt) const
    {
      return direction.Dot(pnt - point) / direction.SumOfSqr();
    }
    //: Return the paramiter of the closest point on the line to 'pnt'.
    // Use Point() to get to the corresponding point on the line.

//    inline const Vector<RealT,3> &Vector() const
//    {
//      return direction;
//    }
//    //: Returns the direction vector of this line.

    inline const Vector<RealT,3> &Direction() const
    {
      return direction;
    }
    //: Returns the direction vector of this line.

    void MakeDirectionUnitVector()
    {
      direction.MakeUnit();
    }
    //: Make the direction part of the line a unit vector.

//    LinePP3dC<RealT> LinePP() const;
    //: Returns the line segment represented by the start point and the end
    //: point.
    // The start point is equal to the start point of this line.
    // The end point of the returned line is determined by the sum of the
    // start point and the direction vector of this line.

    //:-------------------------
    // Geometrical computations.

    RealT Distance(const LinePV3dC &line) const;
    //: Returns the shortest distance between the lines.

    inline RealT Distance(const Point<RealT,3> &p) const
    {
      return Direction().Cross(FirstPoint() - p).Magnitude() / Direction().Magnitude();
    }
    //: Returns the distance of the point 'p' from this line.

#if 0
    LinePV3dC ShortestLine(const LinePV3dC &line) const;
    //: Returns the line which passes through the closest points
    //: of both lines.
    // The returned line has the first point on this line and
    // the second point on the 'line'.

    Point<RealT,3> Intersection(const LinePV3dC &l) const;
    //: Returns the point which belongs to both lines.
    // If the lines have no intersection, the function returns the point which
    // lies in the middle of the shortest line segment between both lines.

    LinePV3dC ProjectionInto(const PlaneABCD3dC & p) const;
    // Returns the line which is the orthogonal projection of this
    // line into the plane 'p'.
#endif
  private:
    Point<RealT,3> point;     // the first point of the line
    Vector<RealT,3> direction;// the direction of the line
  };

#if 0
  std::ostream &operator<<(std::ostream &outS, const LinePV3dC &line);
  // Saves the 'line' into the output stream.

  std::istream &operator>>(std::istream &inS, LinePV3dC &line);
  // Sets the 'line' according to data read from the input stream.
#endif

}// namespace Ravl2

