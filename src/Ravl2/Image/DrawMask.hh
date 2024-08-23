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
            typename ValueT,
            typename DataT = typename ArrayTargetT::value_type,
            unsigned N = ArrayTargetT::dimensions>
    requires WindowedArray<ArrayTargetT, DataT, N> && std::is_convertible_v<ValueT, DataT>
  void DrawMask(ArrayTargetT &img, const ArrayMaskT &mask, const ValueT &value, Index<N> offset, PixelTestT test)
  {
    IndexRange<2> drawRect = mask.range() + offset;// Get rectangle.
    drawRect.clipBy(img.range());
    if(drawRect.area() <= 0)
      return;
    IndexRange<2> maskRect = drawRect - offset;
    for(auto it = zip(clip(img, drawRect), clip(mask, maskRect)); it.valid(); ++it) {
      if(test(it.template data<1>()))
        it.template data<0>() = value;
    }
  }

  template <typename ArrayTargetT,
            typename ArrayMaskT,
            typename ValueT,
            typename DataT = typename ArrayTargetT::value_type,
            typename MaskT = typename ArrayMaskT::value_type,
            unsigned N = ArrayTargetT::dimensions>
    requires WindowedArray<ArrayTargetT, DataT, N> && std::is_convertible_v<ValueT, DataT>
  void DrawMask(ArrayTargetT &img, const ArrayMaskT &mask, const ValueT &value, Index<N> offset = {})
  {
    DrawMask(img, mask, value, offset, [](const MaskT &maskValue) -> bool { return maskValue > 0; });
  }

}// namespace Ravl2
