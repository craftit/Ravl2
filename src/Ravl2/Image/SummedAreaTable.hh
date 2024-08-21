// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="3/4/2002"

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Array2dSqr2Iter2.hh"
#include "Ravl2/Image/DrawFrame.hh"

namespace Ravl2
{

  //: Summed Area Table.
  // This class allows the summing of any area in an image in constant time.
  // The class builds the table with a single pass over the input image. Once
  // this is done the sum of any area can be computed by looking up the corners
  // of the rectangle.

  template <class DataT>
  class SummedAreaTableC : public Array<DataT, 2>
  {
  public:
    //! Default constructor.
    SummedAreaTableC() = default;

    //! Build table form an array of values.
    template <class InT>
    SummedAreaTableC(const Array<InT, 2> &in)
    {
      BuildTable(in);
    }

    template <class InT>
    void BuildTable(const Array<InT, 2> &in)
    {
      clipRange = in.range();
      IndexRange<2> rng(in.range());
      rng.min(1)--;
      rng.min(0)--;
      if(this->Frame() != rng) {
        DataT zero;
        SetZero(zero);// We can't rely on '= 0' working.
        (*this).Array<DataT, 2>::operator=(Array<DataT, 2>(rng));
        DrawFrame((*this), zero, rng);// We only really need the top row and left column cleared.
      }
      Array<DataT, 2> work((*this), in.range());
      Array2dSqr2Iter2C<DataT, InT> it(work, in);
      // First pixel.
      it.DataTL1() = (DataT)it.DataTL2();
      DataT rowSum = (DataT)it.DataBL2();
      it.DataBL1() = it.DataTL1() + rowSum;
      // Do first row.
      do {
        it.DataTR1() = it.DataTL1() + (DataT)it.DataTR2();
        rowSum += (DataT)it.DataBR2();
        it.DataBR1() = it.DataTR1() + rowSum;
      } while(it.next());
      // Do rest of image.
      for(; it;) {
        // Do beginning of row thing.
        rowSum = (DataT)it.DataBL2();
        it.DataBL1() = it.DataTL1() + rowSum;
        // Do rest of row.
        do {
          rowSum += (DataT)it.DataBR2();
          it.DataBR1() = it.DataTR1() + rowSum;
        } while(it.next());
      }
    }
    //: Build table form an array of values.

    DataT Sum(IndexRange<2> range) const
    {
      range.clipBy(clipRange);
      if(range.area() == 0)
        return (*this)[this->Frame().min()];// Return 0.
      range.min(1)--;
      range.min(0)--;
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.End()] - (*this)[range.TopRight()] - (*this)[range.BottomLeft()] + (*this)[range.TopLeft()];
    }
    //: Calculate the sum of the pixel's in the rectangle 'range'.

    DataT VerticalDifference2(IndexRange<2> range, IntT mid) const
    {
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.TopLeft()]
        + ((*this)[mid][range.max(1)] - (*this)[mid][range.min(1)]) * 2
        - (*this)[range.BottomLeft()]
        - (*this)[range.TopRight()]
        + (*this)[range.End()];
    }
    //: Calculate the diffrence between two halfs of the rectangle split vertically.
    // This mid point is an absolute row location and should be within the rectangle.

    DataT HorizontalDifference2(IndexRange<2> range, IntT mid) const
    {
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.TopLeft()]
        + ((*this)[range.max(0)][mid] - (*this)[range.min(0)][mid]) * 2
        - (*this)[range.BottomLeft()]
        + (*this)[range.TopRight()]
        - (*this)[range.End()];
    }
    //: Calculate the diffrence between two halfs of the rectangle split horizontally.
    // This mid point is an absolute column location and should be within the rectangle.

    DataT VerticalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(1).contains(rng));
      IndexRange<2> rng2(range.range(0), rng);
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two halfs of the rectangle split vertially.
    // This mid point is an absolute row location and should be within the rectangle.

    DataT HorizontalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(0).contains(rng));
      IndexRange<2> rng2(rng, range.range(1));
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two rectangles one lying inside the other in the horizontal dimention.
    // This mid point is an absolute column location and should be within the rectangle.

    const IndexRange<2> &ClipRange() const
    {
      return clipRange;
    }
    //: Return range of value positions.

  protected:
    IndexRange<2> clipRange;
  };

}// namespace Ravl2
