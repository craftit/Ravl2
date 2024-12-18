// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////////////////////

#include "Ravl2/Geometry/Plane3ABCD.hh"

namespace Ravl2
{

#if 0
  Point<RealT, 3> PlaneABCD3dC::Intersection(const Line3PV &l) const

  Point<RealT, 3> PlaneABCD3dC::Intersection(const PlaneABCD3dC &planeB,
                                             const PlaneABCD3dC &planeC) const

  Line3PV PlaneABCD3dC::Intersection(const PlaneABCD3dC &plane) const
#endif
  template class Plane3ABCD<float>;

}// namespace Ravl2
