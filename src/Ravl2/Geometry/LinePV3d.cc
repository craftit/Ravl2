// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
// IAPS - Image analysis program system
// 
// File name  : Line3dPV.cc
// Description: line determined by 2 points in 3D space
// Last change: 26.10.1992
// Author     : R. Marik
// Libraries  : iaps
// ------------------------------------------------------------------
//
// Modifications:
//
//
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Geometry/Euclidean/3D/LinePV3d.cc"

#include "Ravl/StdMath.hh"     //::Abs()
#include "Ravl/Point3d.hh"
#include "Ravl/Vector3d.hh"
#include "Ravl/LinePV3d.hh"
#include "Ravl/PlaneABCD3d.hh"
#include "Ravl/Stream.hh"

namespace RavlN {
  
  RealT LinePV3dC::Distance(const LinePV3dC & line) const { 
    // more information in Rektorys: 
    // Prehled uzite matematiky, SNTL, Praha 1988, p. 205
    
    Vector3dC axb(Vector().Cross(line.Vector()));
    RealT modul = axb.Magnitude();
    if (modul == 0)
      return line.Distance(FirstPoint());
    return RavlN::Abs(Vector3dC(line.FirstPoint() - FirstPoint()).Dot(axb))/modul;
  }
  
  LinePV3dC LinePV3dC::ShortestLine(const LinePV3dC & line) const {
    Vector3dC axb(Vector().Cross(line.Vector()));
    RealT     axbNorm = axb.SumOfSqr();
    
    if (IsAlmostZero(axbNorm))
      throw ExceptionNumericalC("LinePV3dC::ShortestLine(): the lines are almost parallel");    
    Vector3dC pmr(FirstPoint() - line.FirstPoint());
    Point3dC p1(FirstPoint()
		+ Vector() * ((axb.Dot(line.Vector().Cross(pmr))) / axbNorm));
    Point3dC p2(line.FirstPoint()
		+ line.Vector() * ((axb.Dot(Vector().Cross(pmr))) / axbNorm));
    return LinePV3dC(p1, p2);
  }
  
  Point3dC LinePV3dC::Intersection(const LinePV3dC & l) const
  { return ShortestLine(l).MiddlePoint(); }
  
#if 0
  LinePV3dC LinePV3dC::ProjectionInto(const PlaneABCD3dC & p) const {
    return LinePV3dC(FirstPoint(), 
		     Vector() - Vector().ProjectionInto(p.Normal()));
  }
#endif
  
  ostream & 
  operator<<(ostream & outS, const LinePV3dC & line) {
    outS << line.FirstPoint() << ' ' << line.SecondPoint();
    return outS;
  }

  istream & 
  operator>>(istream & inS, LinePV3dC & line) {
    Point3dC secondPoint;
    inS >> line.point >> secondPoint;
    line.direction = secondPoint - line.point;
    return inS;
  }
  
}
