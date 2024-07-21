// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_DRAWCIRCLE_HEADER
#define RAVLIMAGE_DRAWCIRCLE_HEADER 1
///////////////////////////////////////////////////////////////////
//! author="Charles Galambos"
//! date="8/8/2002"
//! docentry="Ravl.API.Images.Drawing"
//! lib=RavlImage
//! userlevel=Normal
//! file="Ravl/Image/Base/DrawCircle.hh"

#include <numbers>
#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/Circle2d.hh"
#include "Ravl2/Geometry/CircleIter.hh"
#include "Ravl2/Image/DrawPolygon.hh"

namespace Ravl2 
{
  
  template<class DataT>
  void DrawCircle(ArrayAccess<DataT,2> &dat,const DataT &value,const Index<2> &center,unsigned radius ) 
  {
    if(dat.Frame().Contains(center + Index<2>(radius,radius)) && dat.Frame().Contains(center - Index<2>(radius,radius))) {
      // If far diagonals are inside the image, all pixels in between are.
      for(CircleIterC it(radius,center);it;it++)
	dat[*it] = value;
      return ;
    }
    for(CircleIterC it(radius,center);it;it++) {
      if(!dat.Contains(*it)) 
	continue;
      dat[*it] = value;
    }
  }
  //: Draw a circle in an image.

  template<class DataT>
  void DrawCircle(ArrayAccess<DataT,2> &dat,const DataT &value,const Point<float,2> &center,float radius,bool filled) {
    if(radius <= 1.0f) { // Very small ?
      Index<2> at({int_round(center[0]), int_round(center[1])});
      if(dat.Frame().Contains(at))
        dat[at] = value;
      return ;
    }
    auto step = std::numbers::pi_v<float>/(radius+2.0f);
    Polygon2dC poly;
    Circle2dC circle(center,radius);
    for(RealT a = 0;a < 2*RavlConstN::pi;a += step)
      poly.InsLast(circle.Value(a));
    // Fill in...
    DrawPolygon(dat,value,poly,filled);
  }
  //: Draw a filled circle in an image from real floating point coordinates.

}

#endif
