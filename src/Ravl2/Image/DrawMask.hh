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

namespace Ravl2
{

  //! Draw a mask into an image.
  //! Where the mask is non-zero, the image is set to the 'value'.

  template <typename ArrayTargetT,
    typename ArrayMaskT,
    typename PixelTestT,
    typename DataT = typename ArrayTargetT::value_type,
    typename MaskT = typename ArrayMaskT::value_type,
    unsigned N = ArrayTargetT::dimensions>
  requires WindowedArray<ArrayTargetT, DataT, N>
  void DrawMask(ArrayTargetT &img, const ArrayMaskT &mask, const DataT &value,
		PixelTestT test = [](const MaskT &mask) { return mask > 0; })
  {
    IndexRange<N> rng = img.range();
    rng.clipBy(mask.range());
    if(rng.area() < 1)
      return;
    for(auto it = begin(img, mask, rng); it; it++) {
      if(test(it.template data<1>()))
	it.template data<0>() = value;
    }
  }

}// namespace Ravl2
