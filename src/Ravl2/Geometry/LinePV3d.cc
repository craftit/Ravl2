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

#include "Ravl2/StdMath.hh"     //::std::abs()
#include "Ravl2/Point3d.hh"
#include "Ravl2/Vector3d.hh"
#include "Ravl2/LinePV3d.hh"
#include "Ravl2/PlaneABCD3d.hh"

namespace Ravl2 {

#if 0
  RealT LinePV3dC::Distance(const LinePV3dC & line) const { 
    // more information in Rektorys: 
    // Prehled uzite matematiky, SNTL, Praha 1988, p. 205
    
    Vector<RealT,3> axb(Vector().Cross(line.Vector()));
    RealT modul = axb.Magnitude();
    if (modul == 0)
      return line.Distance(FirstPoint());
    return RavlN::std::abs(Vector<RealT,3>(line.FirstPoint() - FirstPoint()).Dot(axb))/modul;
  }
  
  LinePV3dC LinePV3dC::ShortestLine(const LinePV3dC & line) const {
    Vector<RealT,3> axb(Vector().Cross(line.Vector()));
    RealT     axbNorm = axb.SumOfSqr();
    
    if (isNearZero(axbNorm))
      throw ExceptionNumericalC("LinePV3dC::ShortestLine(): the lines are almost parallel");    
    Vector<RealT,3> pmr(FirstPoint() - line.FirstPoint());
    Point<RealT,3> p1(FirstPoint()
		+ Vector() * ((axb.Dot(line.Vector().Cross(pmr))) / axbNorm));
    Point<RealT,3> p2(line.FirstPoint()
		+ line.Vector() * ((axb.Dot(Vector().Cross(pmr))) / axbNorm));
    return LinePV3dC(p1, p2);
  }
  
  Point<RealT,3> LinePV3dC::Intersection(const LinePV3dC & l) const
  { return ShortestLine(l).MiddlePoint(); }
  
#if 0
  LinePV3dC LinePV3dC::ProjectionInto(const PlaneABCD3dC & p) const {
    return LinePV3dC(FirstPoint(), 
		     Vector() - Vector().ProjectionInto(p.Normal()));
  }
#endif
  
  std::ostream & 
  operator<<(std::ostream & outS, const LinePV3dC & line) {
    outS << line.FirstPoint() << ' ' << line.SecondPoint();
    return outS;
  }

  std::istream & 
  operator>>(std::istream & inS, LinePV3dC & line) {
    Point<RealT,3> secondPoint;
    inS >> line.point >> secondPoint;
    line.direction = secondPoint - line.point;
    return inS;
  }
#endif

}
