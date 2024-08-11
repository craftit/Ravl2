// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_POLYLINE2D_HEADER
#define RAVL_POLYLINE2D_HEADER 1
//! author="Charles Galambos"
//! date="16/2/2006"
//! docentry="Ravl.API.Math.Geometry.2D"

#include "Ravl2/PointSet2d.hh"

namespace Ravl2 {
  
  //: A 2d curve consisting of straight line segments.
  
  class PolyLine2dC 
    : public PointSet2dC
  {
  public:
    PolyLine2dC() {}
    // Empty list of points.
    
    PolyLine2dC(const std::vector<Point<RealT,2>>& points)
      : PointSet2dC(points)
    {}
    // Construct from list of points
    
    bool IsSelfIntersecting() const;
    //: Returns true if the polygon is self intersecting, ie do any sides cross
    
    PolyLine2dC Approx(RealT distLimit) const;
    //: Generate an approximation to the given polyline within the given euclidean distance limit.
    // This routine generates the approximation by searching for the furthest point from the
    // line defined by the two end points and if it is further than the distance limit adding it 
    // to the approximation. The procedure is then repeated for each of the segments either side
    // of the furthest point.
    
    RealT Length() const;
    //: Measure the length of the poly line in euclidean space.
  };
}


#endif
