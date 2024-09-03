// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Images.Scaling and Warping"
//! author="Charles Galambos"

#include "Ravl2/Array.hh"
#include "Ravl2/Array2dSqr3Iter.hh"

namespace Ravl2
{

  //! Subsamples the image with filtering by a factor of 2
  //! Uses a separable 3&times;3 filter with coeffs of &frac14;, &frac12;, &frac34;; sample points correspond to the middle of each sample.
  template <typename PixelT>
  Array<PixelT, 2> FilteredSubSample2(const Array<PixelT, 2> &img, Array<PixelT, 2> out = Array<PixelT, 2>())
  {
    typedef typename RavlN::NumericalTraitsC<PixelT>::AccumT AccumT;
    Index<2> origin(img.range().min()[0] / 2, img.range().min()[1] / 2);
    Index<2> size(((img.range().range(0).size() - 1) / 2) - 1,
                  ((img.range().range(1).size() - 1) / 2) - 1);

    if(size != out.range().End() - out.range().min()) {
      Index<2> end = origin + size;
      IndexRange<2> newRange(origin, end);
      out = Array<PixelT, 2>(newRange);
    }
    Array2dIterC<PixelT> oit(out);
    for(Array2dSqr3IterC<PixelT> it(img); it; it += 2, oit++) {
      AccumT val = static_cast<AccumT>(it.DataTM())
        + static_cast<AccumT>(it.DataBM())
        + static_cast<AccumT>(it.DataMR())
        + static_cast<AccumT>(it.DataML());
      val *= 2;
      val += static_cast<AccumT>(it.DataBL())
        + static_cast<AccumT>(it.DataBR())
        + static_cast<AccumT>(it.DataTR())
        + static_cast<AccumT>(it.DataTL());
      val += static_cast<AccumT>(it.DataMM()) * 4;
      *oit = static_cast<PixelT>(val / 16);
    }
    RavlAssert(!oit);
    return out;
  }

  //! Sub-samples the image by the given integer factor
  //! Pixel at top left-hand corner is always sampled first. <b>No</b> filtering is performed.
  template <class PixelT>
  Array<PixelT, 2> SubSample(const Array<PixelT, 2> &img, const unsigned factor = 2)
  {
    IndexRange<2> oldRect(img.range());
    IndexRange<2> newRect = oldRect / factor;
    Array<PixelT, 2> subSampled(newRect);
    int oldRow, newRow;
    for(oldRow = oldRect.min(0), newRow = newRect.min(0);
        (oldRow <= oldRect.max(0)) && (newRow <= newRect.max(0));
        oldRow += factor, ++newRow) {
      BufferAccessIterC<PixelT> newCol = subSampled[newRow];
      for(BufferAccessIterC<PixelT> oldCol = img[oldRow]; (oldCol.valid()) && (newCol.valid()); oldCol += factor, ++newCol)
        newCol.Data() = oldCol.Data();
    }
    return subSampled;
  }

  //! Up-Samples an image by the given integer factor.
  template <class PixelT>
  ImageC<PixelT> UpSample(const Array<PixelT, 2> &img, const unsigned factor = 2)
  {
    IndexRange<2> oldRect(img.range());
    IndexRange<2> newRect(oldRect * factor);
    Array<PixelT, 2> upSampled(newRect);

    // iterate through rows of original image
    int oldRow, newRow;
    unsigned counter;
    for(oldRow = oldRect.min(0), newRow = newRect.min(0); oldRow <= oldRect.max(0); ++oldRow) {
      // iterate through rows of up-sampled image
      for(unsigned rowCounter = 1; (newRow <= newRect.max(0)) && (rowCounter <= factor); ++newRow, ++rowCounter) {
        auto newCol = begin(upSampled[newRow]);
        auto oldCol = begin(img[oldRow]);
        // now iterate the cols and do the copy
        for(; oldCol.valid(); ++oldCol)// each pixel in the old row
          // iterate through cols of the new image
          for(counter = 1; newCol && counter <= factor; ++counter, ++newCol)
            newCol.Data() = oldCol.Data();
      }
    }
    return upSampled;
  }

}// namespace Ravl2
