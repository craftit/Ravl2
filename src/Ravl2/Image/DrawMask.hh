// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/ArrayIterZip.hh"

namespace RavlImageN {

  template<class PixelT,class MaskT>
  void DrawMask(Array2dC<PixelT> &img,const Array2dC<MaskT> &mask,const PixelT &value) {
    IndexRange<2> rng = img.range();
    rng.clipBy(mask.range());
    if(rng.area() < 1)
      return ;
    for(auto it = begin(img,mask,rng);it;it++) {
      if(it.Data2() != 0)
	it.Data1() = value;
    }
  }
  //: Draw a mask into an image.
  
}
