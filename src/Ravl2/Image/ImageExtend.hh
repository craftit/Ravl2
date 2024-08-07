// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2004, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawFrame.hh"

namespace Ravl2
{
  //! @brief In place create or fill in a result array to ensure it covers a target index range.
  //! @param image The target image that should be covered, this image will be modified to have the given range.
  //! @param range The index range that should be covered.
  //! @return true if the image was extended, false if it was already large enough.
  //! This leaves any new pixels in the result image with undefined values, this maybe
  //! the whole image.
  //! This slightly abuses the copyPolicy parameter as it doesn't copy the image content.
  //! Here's its effect:
  //!    CopyModeT::Never - Never reallocate the image, throw an exception if it's not large enough.
  //!    CopyModeT::Auto - Reallocate the image if it's not large enough, just change its size if it is.
  //!    CopyModeT::Always - Always reallocate the image, even if it's large enough.

  template <CopyModeT copyPolicy = CopyModeT::Auto, typename Array1T, typename InT = typename Array1T::value_type, unsigned N = Array1T::dimensions>
   requires WindowedArray<Array1T, InT, N>
  bool resizeArray(Array1T &image, const IndexRange<N> &range)
  {
    if constexpr(copyPolicy == CopyModeT::Never) {
      if(!image.range().contains(range)) {
	// Cannot resize in place.
	throw std::runtime_error("Image not large enough and cannot resize.");
      }
      image.clipBy(range);
      return false;
    } else {
      // Is this an image we can resize in place?
      static_assert(std::is_same_v<Array1T, Array<InT, N>>, "Cannot resize in place if image is not an Array.");
      if constexpr(copyPolicy == CopyModeT::Auto) {
	if(image.range().contains(range)) {
	  image.clipBy(range);
	  return false;
	}
      }
      // Yes we can resize in place.
      image = Array<InT, N>(range);
      return true;
    }
  }


  //! @brief Extend an image by n pixels in all directions by filling new pixels with 'borderValue'.
  //! If 'result' image is large enough it will be used for results, otherwise it will
  //! be replaced with an image of a suitable size.
  //! @param result The image to extend.
  //! 

  template <typename Array1T, typename InT = typename Array1T::value_type,
    typename Array2T = Array1T, typename OutT = Array2T::value_type,unsigned N = Array1T::dimensions>
    requires WindowedArray<Array1T, InT, N> && WindowedArray<Array2T, OutT, N>
  void extendImageFill(Array2T &result, const Array1T &image, unsigned n, const OutT &borderValue)
  {
    IndexRange<2> rect = image.range().expand(int(n));
    resizeArray(result, rect);
    // Copy centre of image
    copy(clip(result, image.range()),image);
    // Take care of border
    DrawFrame(result, borderValue, n, rect);
  }

  //! @brief Extend an image by n pixels in all directions using a copy of its border pixel
  //! If 'result' image is large enough it will be used for results, otherwise it will
  //! be replaced with an image of a suitable size.

  template <typename Array1T, typename InT = typename Array1T::value_type,
    typename Array2T = Array1T, typename OutT = Array2T::value_type,unsigned N = Array1T::dimensions>
    requires WindowedArray<Array1T, InT, N> && WindowedArray<Array2T, OutT, N>
  void extendImageCopy(Array1T &result, const Array2T &image, unsigned n)
  {
    const IndexRange<2> rect = image.range().expand(int(n));
    resizeArray(result, rect);
    // Copy centre of image
    copy(clip(result, image.range()),image);
    if(n <= 0) {
      return; // Nothing to do.
    }
    // Take care of border
    // Extend rows first.
    const auto minCol = image.range(1).min();
    const auto maxCol = image.range(1).max();
    const IndexRange<1> leftRect = IndexRange<1>(rect.min(1), minCol-1);
    const IndexRange<1> rightRect = IndexRange<1>(maxCol+1, rect.max(1));
    for(int r : image.range(0)) {
      const OutT value1 = image[r][minCol];
      const OutT value2 = image[r][maxCol];
      fill(clip(result[r], leftRect), value1);
      fill(clip(result[r], rightRect), value2);
    }

    // Take care of top of image.
    const auto minRow = image.range(0).min();
    for(int r = rect.min(0); r < image.range(0).min(); r++) {
      copy(result[r], result[minRow]);
    }
    // Take care of bottom of image.
    const auto maxRow = image.range(0).max();
    for(int r = image.range(0).max() + 1; r <= result.range(0).max(); r++) {
      copy(result[r], result[maxRow]);
    }
  }

  //! @brief  Extend an image by n pixels in all directions by mirroring the border region
  //! If 'result' image is large enough it will be used for results, otherwise it will
  //! be replaced with an image of a suitable size.

  template <class DataT>
  void extendImageMirror(Array<DataT,2> &result, const Array<DataT,2> &image, unsigned n)
  {
    if(image.range(0).size() < int(n) || image.range(1).size() < int(n)) {
      SPDLOG_WARN("Image too small to mirror.");
      throw std::runtime_error("Image too small to mirror.");
    }
    const IndexRange<2> rect = image.range().expand(int(n));
    resizeArray(result, rect);
    // Copy centre of image
    copy(clip(result, image.range()),image);
    // Take care of border
    // Extend rows first.
    for(int r = image.range().min(0); r <= image.range().max(0); r++) {
      DataT *org1 = &(result[r][image.range().min(1)]);
      DataT *org2 = &(result[r][image.range().max(1)]);
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
    // Take care of top and bottom of image.
    int ra1 = image.range(0).min();
    int ra2 = ra1 - 1;
    int rb1 = image.range(0).max();
    int rb2 = rb1 + 1;
    for(ra1++, rb1--; rb2 <= rect.max(0); ra1++, ra2--, rb1--, rb2++) {
      copy(result[ra2], result[ra1]);
      copy(result[rb2], result[rb1]);
    }
  }

  // Instantiate the most common types.
  extern template bool resizeArray<CopyModeT::Auto>(Array<uint8_t , 2> &, const IndexRange<2> &);
  extern template void extendImageFill(Array<uint8_t, 2> &, const Array<uint8_t, 2> &, unsigned, const uint8_t &);
  extern template void extendImageCopy(Array<uint8_t, 2> &, const Array<uint8_t, 2> &, unsigned);
  extern template void extendImageMirror(Array<uint8_t, 2> &, const Array<uint8_t, 2> &, unsigned);

}// namespace Ravl2
