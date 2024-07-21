// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_LINEPP2D_HEADER
#define RAVL_LINEPP2D_HEADER 1
//! rcsid="$Id$"
//! lib=RavlMath
//! date="13/9/2002"
//! author="Charles Galambos"
//! docentry="Ravl.API.Math.Geometry.2D"
//! file="Ravl/Math/Geometry/Euclidean/2D/LinePP2d.hh"

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/FLinePP.hh"
#include "Ravl2/Index.hh"

namespace Ravl2
{
  
  //! userlevel=Normal
  //: Line defined by 2 points in 2 dimensional space.

  template<typename RealT>
  class LinePP2dC 
    : public FLinePPC<RealT, 2>
  {
  public:
    LinePP2dC()
    {}
    //: Default constructor.
    // The contents of the line are undefined.

    LinePP2dC(const FLinePPC<RealT, 2> &Base)
      : FLinePPC<RealT, 2>(Base)
    {}
    //: Constructor from base class
    
    LinePP2dC(const Point<RealT,2> &Start,const Point<RealT,2> &End)
      : FLinePPC<2>(Start,End)
    {}
    //: Construct a line from two points.

    LinePP2dC(const Point<RealT,2> &Start,const Vector<RealT,2> &Direction)
      : FLinePPC<2>(Start,Direction)
    {}
    //: Construct a line from a start point and a direction
    
    bool ClipBy(const RealRange<RealT,2> &Rng);
    //: Clip line by given rectangle.
    // If no part of the line is in the rectangle:<ul>
    // <li>line is <i>not</i> clipped</li>
    // <li>method returns <code>false</code></li></ul>
    
    bool IsPointToRight(const Point<RealT,2>& Pt) const
    { return Pt.Area2(P1(),P2()) < 0; }
    //: Checks if this point is to the right of the line
    
    bool IsPointToRightOn(const Point<RealT,2>& Pt) const 
    { return Pt.Area2(P1(),P2()) <= 0; }
    //: Checks if this point is to the right of, or exactly on the line
    
    bool IsPointOn(const Point<RealT,2>& Pt) const
    { return Pt.Area2(P1(),P2()) == 0; }
    //: Checks if this point is exactly on the line

    bool IsPointIn(const Point<RealT,2>& Pt) const;
    //: Checks if this point is exactly on the closed segment
    
    RealT ParIntersection(const LinePP2dC & L) const;
    //: Returns the parameter of the intersection point of 'l' with this line.
    // If the parameter is equal to 0, the intersection is the starting
    // point of this line, if the parameter is 1, the intersection is the
    // end point. If the parameter is between 0 and 1 the intersection is
    // inside of this line segment.

    bool HasInnerIntersection(const LinePP2dC & L) const;
    // Returns true if the intersection of this line segment and the 
    // line 'l' is either inside of this line segment or one of the end points.
 
    Point<RealT,2> Intersection(const LinePP2dC & L) const;
    // Returns the intersection of 2 lines.

    bool Intersection(const LinePP2dC & L, Point<RealT,2>& Here) const;
    //: Calculate the intersection point between this line and l
    //!param: l - another line
    //!param: here - the point of intersection
    //!return: true if the lines intersect or false if they are parallel
    
    bool IntersectRow(RealT Row,RealT &Col) const;
    //: Find the column position which intersects the given row.
    //!param: row - Row for which we want to find the intersecting column
    //!param: col - Place to store the intersecting col.
    //!return: True if position exists, false if there is no intersection
    
    RealT Angle() const {
      Vector<RealT,2> dir = P2() - P1();
      return ATan2(dir[1],dir[0]);
    }
    //: Return the direction of the line.

    Vector<RealT,2> Normal() const
    { return Vector<RealT,2>(point[1][1]-point[0][1],point[0][0]-point[1][0]); }
    //: Returns the normal of the line.
    
    Vector<RealT,2> UnitNormal() const
    { 
      Vector<RealT,2> normal = Normal();
      return normal / normal.Magnitude();
    }
    //: Returns the normal of the line normalized to have unit size.
    
    RealT SignedDistance(const Point<RealT,2> Pt) const
    { return Vector<RealT,2>(Vector()).Cross(Vector<RealT,2>(Pt-P1()))/Length(); }
    //: Returns signed perpendicular distance of pt from this line
    
    RealT Distance(const Point<RealT,2> Pt) const
    { return Abs(SignedDistance(Pt)); }
    //: Return unsigned perpendicular distance of pt from this line

    Point<RealT,2> Projection(const Point<RealT,2> & Pt) const
    { 
      return Point<RealT,2>(ParClosest(Pt));
    }
    //: Returns intersection of line with perpendicular from Pt to line
    
    RealT DistanceWithin(const Point<RealT,2> & pt) const;
    //: Returns distance of pt to nearest point on the line within the segment limits 

  };
  
}

#endif
