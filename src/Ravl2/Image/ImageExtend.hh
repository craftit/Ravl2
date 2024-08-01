// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2004, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawFrame.hh"

namespace Ravl2
{

  template <class DataT>
  void ExtendImageFill(const ImageC<DataT> &image, int n, ImageC<DataT> &result, const DataT &borderValue)
  {
    assert(n > 0);
    IndexRange<2> rect = image.Frame().Expand(n);
    if(!result.Frame().Contains(rect))// Is result rectangle big enough.
      result = ImageC<DataT>(rect);
    // Copy centre of image
    ImageC<DataT> orgImage(result, image.Frame());
    for(BufferAccess2dIter2C<DataT, DataT> it(orgImage, orgImage.Range2(), image, image.Range2()); it; it++)
      it.Data1() = it.Data2();
    // Take care of border
    DrawFrame(result, borderValue, n, rect);
  }
  //: <a name="ExtendImage">Extend an image by n pixels in all directions by filling new pixels with 'borderValue'</a>
  // If 'result' image is large enough it will be used for results, otherwise it will
  // be replaced with an image of a suitable size.

  template <class DataT>
  void ExtendImageCopy(const ImageC<DataT> &image, int n, ImageC<DataT> &result)
  {
    assert(n > 0);
    IndexRange<2> rect = image.Frame().Expand(n);
    if(!result.Frame().Contains(rect))// Is result rectangle big enough.
      result = ImageC<DataT>(rect);
    // Copy centre of image
    ImageC<DataT> orgImage(result, image.Frame());
    for(BufferAccess2dIter2C<DataT, DataT> it(orgImage, orgImage.Range2(), image, image.Range2()); it; it++)
      it.Data1() = it.Data2();
    // Take care of border
    // Extend rows first.
    for(int r = image.Frame().TRow(); r <= image.Frame().BRow(); r++) {
      DataT value1 = result[r][image.LCol()];
      DataT value2 = result[r][image.RCol()];
      DataT *at1 = &(result[r][rect.LCol()]);
      DataT *end1 = &(at1[n]);
      DataT *at2 = &(result[r][image.RCol() + 1]);
      for(; at1 < end1; at1++, at2++) {
        *at1 = value1;
        *at2 = value2;
      }
    }
    // Take care of top of image.
    for(int r = rect.TRow(); r < image.Frame().TRow(); r++) {
      for(BufferAccessIter2C<DataT, DataT> it(result[r], result[image.Frame().TRow()]); it; it++)
        it.Data1() = it.Data2();
    }
    // Take care of bottom of image.
    for(int r = image.Frame().BRow() + 1; r <= result.BRow(); r++) {
      for(BufferAccessIter2C<DataT, DataT> it(result[r], result[image.Frame().BRow()]); it; it++)
        it.Data1() = it.Data2();
    }
  }
  //: Extend an image by n pixels in all directions using a copy of its border pixel
  // If 'result' image is large enough it will be used for results, otherwise it will
  // be replaced with an image of a suitable size.

  template <class DataT>
  void ExtendImageMirror(const ImageC<DataT> &image, int n, ImageC<DataT> &result)
  {
    assert(n > 0);
    assert(image.Frame().Rows() > 1);
    assert(image.Frame().Cols() > 1);
    IndexRange<2> rect = image.Frame().Expand(n);
    if(!result.Frame().Contains(rect))// Is result rectangle big enough.
      result = ImageC<DataT>(rect);
    // Copy centre of image
    ImageC<DataT> orgImage(result, image.Frame());
    for(BufferAccess2dIter2C<DataT, DataT> it(orgImage, orgImage.Range2(), image, image.Range2()); it; it++)
      it.Data1() = it.Data2();
    // Take care of border
    // Extend rows first.
    for(int r = image.Frame().TRow(); r <= image.Frame().BRow(); r++) {
      DataT *org1 = &(result[r][image.LCol()]);
      DataT *org2 = &(result[r][image.RCol()]);
      DataT *at1 = org1 - 1;
      DataT *at2 = org2 + 1;
      org1++;
      org2--;
      DataT *end2 = &(at2[n]);
      for(; at2 < end2; at1--, at2++, org1++, org2--) {
        *at1 = *org1;
        *at2 = *org2;
      }
    }
    // Take care of top of image.
    int ra1 = image.TRow();
    int ra2 = ra1 - 1;
    int rb1 = image.BRow();
    int rb2 = rb1 + 1;
    for(ra1++, rb1--; rb2 <= rect.BRow(); ra1++, ra2--, rb1--, rb2++) {
      for(BufferAccessIter2C<DataT, DataT> it(result[ra2], result[ra1]); it; it++)
        it.Data1() = it.Data2();
      for(BufferAccessIter2C<DataT, DataT> it(result[rb2], result[rb1]); it; it++)
        it.Data1() = it.Data2();
    }
  }
  //: Extend an image by n pixels in all directions by mirroring the border region
  // If 'result' image is large enough it will be used for results, otherwise it will
  // be replaced with an image of a suitable size.

}// namespace Ravl2
