// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2004, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_DRAWELLIPSE_HEADER
#define RAVLIMAGE_DRAWELLIPSE_HEADER 1
//! author="Charles Galambos"
//! lib=RavlImage
//! docentry="Ravl.API.Images.Drawing"
//! file="Ravl/Image/Base/DrawEllipse.hh"

#include "Ravl2/Geometry/Ellipse2d.hh"
#include "Ravl2/Image/DrawPolygon.hh"

namespace RavlImageN {
  using namespace RavlN;
  
  template<class DataT>
  void DrawEllipse(ImageC<DataT> &image,const DataT &value,const Ellipse2dC &ellipse,bool fill = false) {
    RealT maj,min;
    ellipse.Size(maj,min);
    if((maj + min) < 3) { // Very small ?
      Index2dC at = ellipse.Centre();
      if(image.Frame().Contains(at))
	image[at] = value;
      return ;
    }
    RealT step = 2*RavlConstN::pi/(maj + min);
    Polygon2dC poly;
    for(RealT a = 0;a < 2*RavlConstN::pi;a += step)
      poly.InsLast(ellipse.Point(a));
    DrawPolygon(image,value,poly,fill);
  }
  //: Draw an ellipse.
  //!param: image - Image to draw into.
  //!param: value - Value to draw.
  //!param: ellipse - Ellipse to draw.
  //!param: fill - Fill polygon if true.
  // Note, this currently just breaks the ellipse into 30 segments and draws as polygon. It could do
  // with a better way of choosing this number.
  
}

#endif
