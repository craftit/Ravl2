// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_LINE3DPP_HEADER
#define RAVL_LINE3DPP_HEADER
///////////////////////////////////////////////////////////////////////////
//! userlevel=Normal
//! author="Radek Marik"
//! docentry="Ravl.API.Math.Geometry.3D"
//! date="26/10/1992"
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Geometry/Euclidean/3D/LinePP3d.hh"

#include "Ravl/Types.hh"
#include "Ravl/Point3d.hh"
#include "Ravl/Vector3d.hh"
#include "Ravl/FLinePP.hh"

#if RAVL_COMPILER_MIPSPRO
#include "Ravl/BinStream.hh"
#endif 
namespace RavlN {

  class Line3dPVC;
  class Vector3dC;
  
  //: Line determined by two points in 3D space 
  // The class LinePP3dC represents an oriented line segment in 3 dimensional
  // Euclidian space. Furthermore, it has the same features as any line
  // in Euclidian space. A line is represented by 2 points.
  
  class LinePP3dC
    : public FLinePPC<3>
  {
  public:
    LinePP3dC()
    {}
    //: Default constructor.
    // The line created is not defined.
    
    LinePP3dC(const Point3dC & first, const Point3dC & second)
      : FLinePPC<3>(first,second)
    {}
    //: Creates the line segment connecting the point 'first' with the
    //: point 'second'.
    
    LinePP3dC(const Point3dC & a, const Vector3dC & v) 
      : FLinePPC<3>(a,v)
    {}
    //: Creates the line segment connecting the point 'a' and the point
    //: a+v.
    
    Line3dPVC LinePV() const;
    //: Returns the line represented by the start point and the vector.
    // The start point is equal to the start point of this line
    // segment. The vector of the returned line is determined by the
    // start point and the end point of this line segment.
    
    //:-------------------------
    // Geometrical computations.
    
    RealT Distance(const LinePP3dC & line);
    //: Returns the shortest distance between this line
    //: and the segment 'line'.
    
    RealT Distance(const Point3dC & p) const;
    //: Returns the distance between the point 'p' and this line.
    
    LinePP3dC ShortestLine(const LinePP3dC & line);
    //: Returns the shortest line connecting this to 'line'.
    // The returned line has the first point on this
    // line and the second point on the 'line'.
    
    Vector3dC Perpendicular(const Point3dC & p) const
    { return Vector().Cross(p-P1()); }
    //: Returns the vector that is perpendicular to the plane containing
    //: the line segment and the point 'p'. 
    // The direction of the return vector is determined by the cross 
    // product (P2-P1) % (p-P1) which is equivalent to (P1-p) % (P2-p).
    
    Vector3dC Vector() const
    { return FLinePPC<3>::Vector(); } 
    //: Returns the line segment as a free vector.
    
  private:
    friend RealT Distance(const Point3dC & point, const LinePP3dC & line);
  };
  
  inline RealT Distance(const Point3dC & point, const LinePP3dC & line) 
  { return line.Distance(point); }
  
  inline
  ostream & operator<<(ostream & s, const LinePP3dC & line)
  { return s << ((const FLinePPC<3> &) line); }
  
  inline
  istream & operator>>(istream & s, LinePP3dC & line)
  { return s >> ((FLinePPC<3> &) line); }

  inline
  BinOStreamC & operator<<(BinOStreamC & s, const LinePP3dC & line)
  { return s << ((const FLinePPC<3> &) line); }
  
  inline
  BinIStreamC & operator>>(BinIStreamC & s, LinePP3dC & line)
  { return s >> ((FLinePPC<3> &) line); }
  
  
}
#endif


