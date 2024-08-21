// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
/////////////////////////////////////////////////////////////

#include "Ravl2/PlanePVV3d.hh"
#include "Ravl2/LinePV3d.hh"
#include "Ravl2/Point2d.hh"
#include "Ravl2/PlaneABCD3d.hh"
#include "Ravl2/FMatrix.hh"
#include "Ravl2/CCMath.hh"

extern "C" {  double qrlsq(double *a,double *b,int m,int n,int *f) ; }

namespace Ravl2 {


  
  // Returns the point of intersection of this plane with the line 'l'.
  
  Point<RealT,3> PlanePVV3dC::Intersection(const LinePV3dC & l) const 
  { return PlaneABCD3d().Intersection(l); }
  
  // Find the closest point on the plane to 'p'
  
  Point<RealT,3> PlanePVV3dC::ClosestPoint(const Point<RealT,3> &p) const 
  { return PlaneABCD3d().ClosestPoint(p); }
  
  
  // Measure the euclidean distance to the plane.
  
  RealT PlanePVV3dC::EuclideanDistance(const Point<RealT,3> &point) const 
  { return PlaneABCD3d().EuclideanDistance(point); }
  
  //: Project a point into the coordinate system of the plane
  
  Point<RealT,2> PlanePVV3dC::Projection(const Point<RealT,3> &pointOnPlane) const {
    FTensor<RealT,2><3,2> a;
    a[0][0] = vector1[0];
    a[1][0] = vector1[1];
    a[2][0] = vector1[2];
    a[0][1] = vector2[0];
    a[1][1] = vector2[1];
    a[2][1] = vector2[2];
    Point<RealT,3> tmp = pointOnPlane;
    tmp -= Point();
    int f = 0;
    ::qrlsq(&(a[0][0]),&(tmp[0]),3,2,&f);
    return Point<RealT,2>(tmp[0],tmp[1]);
  }

  // Returns the coordinates (t1,t2) of the point projected onto
  // the plane. The coordinate system is determined by the point of
  // the plane and its two vectors.
  
  Point<RealT,2> PlanePVV3dC::ProjectedIntersection(const LinePV3dC & l) const 
  { return Projection(PlanePVV3dC::Intersection(l)); }
  

}



