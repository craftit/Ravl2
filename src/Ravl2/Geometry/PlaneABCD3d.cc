// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////////////////////

#include "Ravl2/Point3d.hh"
#include "Ravl2/Vector3d.hh"
#include "Ravl2/LinePV3d.hh"
#include "Ravl2/PlaneABCD3d.hh"

namespace Ravl2
{

  Point<RealT, 3> PlaneABCD3dC::Intersection(const LinePV3dC &l) const
  {
    RealT nu = normal.Dot(l.Vector());
    if(isNearZero(nu))
      throw ExceptionNumericalC("PlaneABCD3dC::Intersection(): the line is almost parallel to the plane:\n");
    return l.PointT(-Value(l.FirstPoint()) / nu);
  }

  Point<RealT, 3> PlaneABCD3dC::Intersection(const PlaneABCD3dC &planeB,
                                             const PlaneABCD3dC &planeC) const
  {
    Vector<RealT, 3> n1xn2(Normal().Cross(planeB.Normal()));
    RealT tripleSP = n1xn2.Dot(planeC.Normal());
    if(isNearZero(tripleSP))
      throw ExceptionNumericalC("PlaneABCD3dC::Intersection(): the planes are almost parallel");
    Vector<RealT, 3> n2xn3(planeB.Normal().Cross(planeC.Normal()));
    Vector<RealT, 3> n3xn1(planeC.Normal().Cross(Normal()));
    return Point<RealT, 3>(n2xn3 * D() + n3xn1 * planeB.D() + n1xn2 * planeC.D())
      / (-tripleSP);
  }

  LinePV3dC PlaneABCD3dC::Intersection(const PlaneABCD3dC &plane) const
  {
    Vector<RealT, 3> direction(Normal().Cross(plane.Normal()));
    RealT den = direction.SumOfSqr();
    if(isNearZero(den))
      throw ExceptionNumericalC("PlaneABCD3dC::Intersection(): the planes are almost parallel");
    Vector<RealT, 3> n212(plane.Normal().Cross(direction));
    Vector<RealT, 3> n121(direction.Cross(Normal()));
    return LinePV3dC((n212 * D() + n121 * plane.D()) / (-den), direction);
  }

}// namespace Ravl2
