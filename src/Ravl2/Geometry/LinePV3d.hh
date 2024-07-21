// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_LINEPV3D_HEADER
#define RAVL_LINEPV3D_HEADER
/////////////////////////////////////////////////////////////////////////
//! userlevel=Normal
//! author="Radek Marik"
//! docentry="Ravl.API.Math.Geometry.3D"
//! rcsid="$Id$"
//! date="06/08/1995"
//! lib=RavlMath
//! file="Ravl/Math/Geometry/Euclidean/3D/LinePV3d.hh"

#include "Ravl/Point3d.hh"
#include "Ravl/Vector3d.hh"
#include "Ravl/Types.hh"

namespace RavlN {

  class LinePP3dC;
  class PlaneABCD3dC;
  
  //: Line determined by one point and a vector in 3D space 
  // The LinePV3dC class represents the line in 3 dimensional Euclidian
  // space. The line is represented by one point and its direction vector.
  
  class LinePV3dC
  {
  public:
  
    //:----------------------------------------------
    // Constructors, assigment, copy, and destructor.
    
    inline LinePV3dC()
      : point(0, 0, 0), direction(0, 0, 0)
    {}
    //: Creates the non-existing line (0,0,0) [0,0,0].
    
    inline LinePV3dC(const Point3dC & first, const Point3dC & second)
      : point(first), direction(second-first)
    {}
    //: Creates the line passing through the points 'first' and
    //: second.
    
    inline LinePV3dC(const Point3dC & a, const Vector3dC & v)
      : point(a), direction(v)
    {}
    //: Creates the line passing through the point 'a' and with
    //: the direction 'v'.
    
    //:-------------------------------
    // Access to elements of the line.
    
    inline const Point3dC & FirstPoint() const
    { return point; }
    //: Returns the point of the line.
    
    inline Point3dC SecondPoint() const
    { return point + direction; }
    //: Returns the point which is the end point of the direction
    //: vector placed in the first point of the line.
    
    inline Point3dC MiddlePoint() const
    { return PointT(0.5); }
    //: Returns the point which is in the middle of the FirstPoint()
    //: and the SecondPoint() of the line segment.
    
    Point3dC PointT(RealT lambda) const
    { return point + direction * lambda; }
    //: Returns the point FirstPoint() + lambda * direction.
    // Obsolete, use Point(l)
    
    Point3dC Point(RealT lambda) const
    { return point + direction * lambda; }
    //: Returns the point FirstPoint() + lambda * direction.
    
    RealT ParClosest(const Point3dC &pnt) const 
    { return direction.Dot(pnt - point) / direction.SumOfSqr(); }
    //: Return the paramiter of the closest point on the line to 'pnt'.
    // Use Point() to get to the corresponding point on the line.
    
    inline const Vector3dC & Vector() const
    { return direction; }
    //: Returns the direction vector of this line.
    
    inline const Vector3dC & Direction() const
    { return direction; }
    //: Returns the direction vector of this line.
    
    void MakeDirectionUnitVector()
    { direction.MakeUnit(); }
    //: Make the direction part of the line a unit vector.
    
    LinePP3dC LinePP() const;
    //: Returns the line segment represented by the start point and the end
    //: point. 
    // The start point is equal to the start point of this line.
    // The end point of the returned line is determined by the sum of the
    // start point and the direction vector of this line.
    
    //:-------------------------
    // Geometrical computations.
    
    RealT Distance(const LinePV3dC & line) const;
    //: Returns the shortest distance between the lines. 
    
    inline RealT Distance(const Point3dC & p) const
    { return  Vector().Cross(FirstPoint() - p).Magnitude() / Vector().Magnitude(); }
    //: Returns the distance of the point 'p' from this line.
    
    LinePV3dC ShortestLine(const LinePV3dC & line) const;
    //: Returns the line which passes through the closest points
    //: of both lines. 
    // The returned line has the first point on this line and 
    // the second point on the 'line'.
    
    Point3dC Intersection(const LinePV3dC & l) const;
    //: Returns the point which belongs to both lines. 
    // If the lines have no intersection, the function returns the point which
    // lies in the middle of the shortest line segment between both lines.
    
#if 0    
    LinePV3dC ProjectionInto(const PlaneABCD3dC & p) const;
    // Returns the line which is the orthogonal projection of this
    // line into the plane 'p'.
#endif    
  private:
    Point3dC  point;     // the first point of the line
    Vector3dC direction; // the direction of the line
    
    friend istream & operator>>(istream & inS, LinePV3dC & line);
  };
  
  ostream & operator<<(ostream & outS, const LinePV3dC & line);
  // Saves the 'line' into the output stream.
  
  istream & operator>>(istream & inS, LinePV3dC & line);
  // Sets the 'line' according to data read from the input stream.
  
}

#endif

