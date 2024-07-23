// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="13/9/2002"
//! author="Charles Galambos"

#pragma once

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/FLinePP.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{
  
  //: Line defined by 2 points in 2-dimensional space.

  template<typename RealT>
  class LinePP2dC 
    : public FLinePPC<RealT, 2>
  {
  public:
    LinePP2dC()
    = default;
    //: Default constructor.
    // The contents of the line are undefined.

    explicit LinePP2dC(const FLinePPC<RealT, 2> &Base)
      : FLinePPC<RealT, 2>(Base)
    {}
    //: Constructor from base class
    
    LinePP2dC(const Point<RealT,2> &Start,const Point<RealT,2> &End)
      : FLinePPC<RealT, 2>(Start,End)
    {}
    //: Construct a line from two points.

    //! Create line from start and end points
    static LinePP2dC<RealT> fromPoints(const Point<RealT,2> &start,const Point<RealT,2> &end)
    { return {start, end}; }

    //! Create line from start point and direction
    static LinePP2dC<RealT> fromStartAndDirection(const Point<RealT,2> &start,const Vector<RealT,2> &direction)
    { return {start,start + direction}; }

    bool clipBy(const Range<RealT,2> &Rng);
    //: Clip line by given rectangle.
    // If no part of the line is in the rectangle:<ul>
    // <li>line is <i>not</i> clipped</li>
    // <li>method returns <code>false</code></li></ul>

    [[nodiscard]] bool IsPointToRight(const Point<RealT,2>& Pt) const
    { return triangleArea2(Pt,this->P1(),this->P2()) < 0; }
    //: Checks if this point is to the right of the line

    [[nodiscard]] bool IsPointToRightOn(const Point<RealT,2>& Pt) const
    { return triangleArea2(Pt,this->P1(),this->P2()) <= 0; }
    //: Checks if this point is to the right of, or exactly on the line

    [[nodiscard]] bool IsPointOn(const Point<RealT,2>& Pt) const
    { return triangleArea2(Pt,this->P1(),this->P2()) == 0; }
    //: Checks if this point is exactly on the line

    [[nodiscard]] bool IsPointIn(const Point<RealT,2>& Pt) const;
    //: Checks if this point is exactly on the closed segment

    [[nodiscard]] RealT ParIntersection(const LinePP2dC & L) const;
    //: Returns the parameter of the intersection point of 'l' with this line.
    // If the parameter is equal to 0, the intersection is the starting
    // point of this line, if the parameter is 1, the intersection is the
    // end point. If the parameter is between 0 and 1 the intersection is
    // inside of this line segment.

    [[nodiscard]] bool HasInnerIntersection(const LinePP2dC & L) const;
    // Returns true if the intersection of this line segment and the 
    // line 'l' is either inside of this line segment or one of the end points.

    [[nodiscard]] Point<RealT,2> Intersection(const LinePP2dC & L) const;
    // Returns the intersection of 2 lines.

    [[nodiscard]] bool Intersection(const LinePP2dC & L, Point<RealT,2>& Here) const;
    //: Calculate the intersection point between this line and l
    //!param: l - another line
    //!param: here - the point of intersection
    //!return: true if the lines intersect or false if they are parallel

    [[nodiscard]] bool IntersectRow(RealT Row,RealT &Col) const;
    //: Find the column position which intersects the given row.
    //!param: row - Row for which we want to find the intersecting column
    //!param: col - Place to store the intersecting col.
    //!return: True if position exists, false if there is no intersection

    [[nodiscard]] RealT Angle() const {
      auto dir = this->P2() - this->P1();
      return std::atan2(dir[1],dir[0]);
    }
    //: Return the direction of the line.

    [[nodiscard]] Vector<RealT,2> Normal() const
    { return toVector<RealT>(this->point[1][1]-this->point[0][1],this->point[0][0]-this->point[1][0]); }
    //: Returns the normal of the line.

    [[nodiscard]] Vector<RealT,2> UnitNormal() const
    { 
      auto normal = this->Normal();
      return normal / xt::linalg::norm(normal,2);
    }
    //: Returns the normal of the line normalized to have unit size.

    [[nodiscard]] RealT SignedDistance(const Point<RealT,2> Pt) const
    {
      auto dir = this->direction();
      return cross<RealT>(dir,Pt - this->P1())/xt::linalg::norm(dir,2);
    }
    //: Returns signed perpendicular distance of pt from this line

    [[nodiscard]] RealT Distance(const Point<RealT,2> Pt) const
    { return std::abs(SignedDistance(Pt)); }
    //: Return unsigned perpendicular distance of pt from this line

    [[nodiscard]] Point<RealT,2> Projection(const Point<RealT,2> & Pt) const
    { 
      return Point<RealT,2>(this->ParClosest(Pt));
    }
    //: Returns intersection of line with perpendicular from Pt to line

    [[nodiscard]] RealT DistanceWithin(const Point<RealT,2> & pt) const;
    //: Returns distance of pt to nearest point on the line within the segment limits 

  };

  extern template class LinePP2dC<double>;
  extern template class LinePP2dC<float>;

}

