// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="8/8/2002"

#pragma once

#include <numbers>
#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/Circle.hh"
#include "Ravl2/Geometry/CircleIter.hh"
#include "Ravl2/Image/DrawPolygon.hh"

namespace Ravl2 
{

  //: Draw a circle in an image with an integer index and size.
  template<class DataT>
  void DrawCircle(ArrayAccess<DataT,2> &dat,const DataT &value,const Index<2> &center,unsigned radius ) 
  {
    Index<2> radius2({int(radius),int(radius)});
    if(dat.range().contains(center + radius2) && dat.range().contains(center - radius2)) {
      // If far diagonals are inside the image, all pixels in between are.
      for(CircleIterC it(radius,center);it;it++)
	dat[*it] = value;
      return ;
    }
    // Be careful, we need to check each pixel.
    for(CircleIterC it(radius,center);it;it++) {
      if(!dat.Contains(*it)) 
	continue;
      dat[*it] = value;
    }
  }

  //! Draw a filled circle in an image from real floating point coordinates.
  template<class DataT,typename CoordT = float>
  void DrawFilledCircle(ArrayAccess<DataT,2> &dat,const DataT &value,const Point<CoordT,2> &center,CoordT radius) {
    if(radius <= 1.0f) { // Very small ?
      Index<2> at({int_round(center[0]), int_round(center[1])});
      if(dat.Frame().Contains(at))
        dat[at] = value;
      return ;
    }
    auto step = std::numbers::pi_v<CoordT>/(radius+2.0f);
    Polygon2dC<CoordT> poly;
    Circle2dC<float> circle(center,radius);
    for(RealT a = 0;a < 2.0f*std::numbers::pi_v<CoordT>;a += step)
      poly.push_back(circle.Value(a));
    // Fill in...
    DrawFilledPolygon(dat,value,poly);
  }

  //! Draw a filled circle in an image from real floating point coordinates.
  template<class DataT,typename CoordT = float>
  void DrawCircle(ArrayAccess<DataT,2> &dat,const DataT &value,const Point<CoordT,2> &center,CoordT radius) {
    if(radius <= 1.0f) { // Very small ?
      Index<2> at = toIndex(center);
      if(dat.range().contains(at))
        dat[at] = value;
      return ;
    }
    auto step = std::numbers::pi_v<CoordT>/(radius+CoordT(2.0));
    Polygon2dC<CoordT> poly;
    Circle2dC<float> circle(center,radius);
    for(RealT a = 0;a < 2.0f*std::numbers::pi_v<CoordT>;a += step)
      poly.push_back(circle.Value(a));
    // Fill in...
    DrawPolygon(dat,value,poly);
  }

}

