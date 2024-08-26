// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once

#include "Ravl2/Array.hh"

namespace Ravl2
{

  template <typename ArrayT, typename DataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT, DataT, 2>
  void DrawCross(ArrayT &dat, const DataT &value, const Index<2> &at, int pixelSize = 3)
  {
    // Cross entirely in the image ?
    if(dat.range().contains(IndexRange<2>(at, at).expand(pixelSize))) {
      // Its completely inside, we can just change the pixels.
      dat[at] = value;
      for(int i = 1; i < pixelSize; i++) {
        Index<2> off1 = toIndex(i, i);
        dat[at + off1] = value;
        dat[at - off1] = value;
        off1[0] = -i;
        dat[at + off1] = value;
        dat[at - off1] = value;
      }
    } else {
      // Cross is not entirely in the image, so check each pixel.
      // The crosses are typically small, so it is faster to check each pixel.
      if(dat.range().contains(at))
        dat[at] = value;
      for(int i = 1; i < pixelSize; i++) {
        Index<2> off1(i, i);
        Index<2> pat = at + off1;
        if(dat.range().contains(pat))
          dat[pat] = value;
        pat = at - off1;
        if(dat.range().contains(pat))
          dat[pat] = value;
        off1[0] = -i;
        pat = at + off1;
        if(dat.range().contains(pat))
          dat[pat] = value;
        pat = at - off1;
        if(dat.range().contains(pat))
          dat[pat] = value;
      }
    }
  }
  //: Draw a cross in an image.

}// namespace Ravl2
