// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="22/04/2002"
#pragma once

#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! @brief Draw a filled rectangle in an image.
  //! @param dat The image to draw the rectangle in.
  //! @param value The value to set the pixels to.
  //! @param rect The rectangle to draw.

  template <typename ArrayT, typename DataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT, DataT, 2>
  void DrawFilledFrame(ArrayT &dat, const DataT &value, const IndexRange<2> &rect)
  {
    IndexRange<2> dr = rect.clip(dat.range());
    if(dr.empty())
      return;// Nothing to draw around.
    auto arr = clipUnsafe(dat, dr);
    std::ranges::fill(arr.begin(), arr.end(), value);
  }

  //! Draw a rectangle in an image.
  template <typename ArrayT, typename DataT = ArrayT::value_type, typename PixelT>
    requires WindowedArray<ArrayT, DataT, 2> && std::is_convertible_v<PixelT, DataT>
  void DrawFrame(ArrayT &dat, const PixelT &value, const IndexRange<2> &rect)
  {
    IndexRange<2> dr = rect.clip(dat.range());
    if(dr.empty())
      return;// Nothing to draw around.

    DataT *it1 = nullptr;
    const DataT *eor = nullptr;
    int ColN = dr.range(1).size();                                                            // Number of columns.
    if(rect.range(0).min() == dr.range(0).min() && rect.range(0).max() == dr.range(0).max()) {// The rectangle wasn't clipped.
      // Do horizontal lines.
      DataT *it2 = nullptr;
      it1 = &(dat[dr.range(0).min()][dr.range(1).min()]);
      it2 = &(dat[dr.range(0).max()][dr.range(1).min()]);
      eor = &(it1[ColN]);
      for(; it1 != eor;) {
        *(it1++) = DataT(value);
        *(it2++) = DataT(value);
      }
    } else {
      // Do top and bottom lines separately
      if(rect.range(0).min() == dr.range(0).min()) {
        // Do top horizontal line.
        it1 = &(dat[dr.range(0).min()][dr.range(1).min()]);
        eor = &(it1[ColN]);
        for(; it1 != eor;)
          *(it1++) = DataT(value);
      }
      if(rect.range(0).max() == dr.range(0).max()) {
        // Do bottom horizontal line.
        it1 = &(dat[dr.range(0).max()][dr.range(1).min()]);
        eor = &(it1[ColN]);
        for(; it1 != eor;)
          *(it1++) = DataT(value);
      }
    }
    // Do vertical lines.
    ColN--;
    if(dr.range(1).min() == rect.range(1).min() && dr.range(1).max() == rect.range(1).max()) {// Not clipped.
      for(auto r = dr.range(0).min() + 1; r < dr.range(0).max(); r++) {
        it1 = &(dat[r][dr.range(1).min()]);
        it1[0] = DataT(value);
        it1[ColN] = DataT(value);
      }
    } else {// Clipped.
      if(dr.range(1).min() == rect.range(1).min()) {
        for(int r = dr.range(0).min() + 1; r < dr.range(0).max(); r++)
          dat[r][dr.range(1).min()] = DataT(value);
      }
      if(dr.range(1).max() == rect.range(1).max()) {
        for(int r = dr.range(0).min() + 1; r < dr.range(0).max(); r++)
          dat[r][dr.range(1).max()] = DataT(value);
      }
    }
  }

  //! Draw a rectangle in an image of given width
  //! The rectangle is assumed to be the outer one and the image will be filled inside it by 'width' pixels.
  //! Parts outside the image are clipped. Rows are dimension 0 and columns dimension 1.
  template <typename ArrayT, typename DataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT, DataT, 2>
  void DrawFrame(ArrayT &dat, const DataT &value, unsigned width, const IndexRange<2> &outerRect)
  {
    if(width == 0 || outerRect.empty())
      return;
    const int w = int(width);
    const IndexRange<1> rows = outerRect.range(0);
    const IndexRange<1> cols = outerRect.range(1);
    // Top and bottom bands span the full width; left and right bands fill the rows between them.
    DrawFilledFrame(dat, value, IndexRange<2>(IndexRange<1>(rows.min(), rows.min() + w - 1), cols));
    DrawFilledFrame(dat, value, IndexRange<2>(IndexRange<1>(rows.max() - w + 1, rows.max()), cols));
    const IndexRange<1> middleRows(rows.min() + w, rows.max() - w);
    if(middleRows.empty())
      return;
    DrawFilledFrame(dat, value, IndexRange<2>(middleRows, IndexRange<1>(cols.min(), cols.min() + w - 1)));
    DrawFilledFrame(dat, value, IndexRange<2>(middleRows, IndexRange<1>(cols.max() - w + 1, cols.max())));
  }

}// namespace Ravl2
