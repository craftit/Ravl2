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
#include "Ravl2/Assert.hh"
#include "Ravl2/Image/Array2Sqr2Iter2.hh"
#include "Ravl2/Image/DrawFrame.hh"

namespace Ravl2
{

  //! Summed Area Table.
  //! This class allows the summing of any area in an image in constant time.
  //! The class builds the table with a single pass over the input image. Once
  //! this is done the sum of any area can be computed by looking up the corners
  //! of the rectangle.

  template <class DataT>
  class SummedAreaTableC : public Array<DataT, 2>
  {
  public:
    //! Default constructor.
    SummedAreaTableC() = default;

  private:
    //! Constructor
    SummedAreaTableC(Array<DataT, 2> &&arr, IndexRange<2> newClipRange)
      : Array<DataT, 2>(std::move(arr)) ,
        clipRange(newClipRange)
    {}
  public:

    //! Build table form an array of values.
    template <class InT>
    static SummedAreaTableC<DataT> BuildTable(const Array<InT, 2> &in)
    {
      IndexRange<2> rng(in.range());
      rng.min(1)--;
      rng.min(0)--;
      SummedAreaTableC<DataT> ret(Array<DataT, 2>(rng),in.range());
      DataT zero {};
      DrawFrame(ret, zero, rng);// We only really need the top row and left column cleared.
      Array<DataT, 2> work(ret, in.range());
      Array2dSqr2Iter2C<DataT, InT> it(work, in);
      // First pixel.
      it.DataTL1() = DataT(it.DataTL2());
      DataT rowSum = DataT(it.DataBL2());
      it.DataBL1() = it.DataTL1() + rowSum;
      // Do first row.
      do {
        it.DataTR1() = it.DataTL1() + DataT(it.DataTR2());
        rowSum += DataT(it.DataBR2());
        it.DataBR1() = it.DataTR1() + rowSum;
      } while(it.next());
      // Do rest of image.
      for(; it.valid();) {
        // Do beginning of row thing.
        rowSum = DataT(it.DataBL2());
        it.DataBL1() = it.DataTL1() + rowSum;
        // Do rest of row.
        do {
          rowSum += DataT(it.DataBR2());
          it.DataBR1() = it.DataTR1() + rowSum;
        } while(it.next());
      }
      return ret;
    }

    //! Calculate the sum of the pixel's in the rectangle 'range'.
    DataT Sum(IndexRange<2> range) const
    {
      range.clipBy(clipRange);
      if(range.area() == 0)
        return (*this)[this->range().min()];// Return 0.
      // Could speed this up by separating out row accesses ?
      auto top = range.range(0).min()-1;
      auto left = range.range(1).min()-1;
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      return (*this)(bottom,right) - (*this)(top,right) - (*this)(bottom,left) + (*this)(top,left);
    }

    //! Calculate the difference between two halfs of the rectangle split vertically.
    // This mid-point is an absolute row location and should be within the rectangle.
    DataT VerticalDifference2(IndexRange<2> range, int mid) const
    {
      auto top = range.range(0).min();
      auto left = range.range(1).min();
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      // Could speed this up by separating out row accesses ?
      return (*this)(top,left)
        + ((*this)(mid,right) - (*this)(mid,left)) * 2
        - (*this)(bottom,left)
        - (*this)(top,right)
        + (*this)(bottom,right);
    }

    //! Calculate the difference between two halves of the rectangle split horizontally.
    // This mid-point is an absolute column location and should be within the rectangle.
    DataT HorizontalDifference2(IndexRange<2> range, int mid) const
    {
      auto top = range.range(0).min();
      auto left = range.range(1).min();
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      // Could speed this up by separating out row accesses ?
      return (*this)(top,left)
        + ((*this)[range.max(0)][mid] - (*this)[range.min(0)][mid]) * 2
        - (*this)(bottom,left)
        + (*this)(top,right)
        - (*this)(bottom,right);
    }

    //! Calculate the difference between two halves of the rectangle split vertically.
    // This mid-point is an absolute row location and should be within the rectangle.
    DataT VerticalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(1).contains(rng));
      IndexRange<2> rng2(range.range(0), rng);
      return Sum(range) - Sum(rng2);
    }

    //! Calculate the difference between two rectangles one lying inside the other in the horizontal dimension.
    // This mid-point is an absolute column location and should be within the rectangle.
    DataT HorizontalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(0).contains(rng));
      IndexRange<2> rng2(rng, range.range(1));
      return Sum(range) - Sum(rng2);
    }

    //! Return range of value positions.
    [[nodiscard]] const IndexRange<2> &ClipRange() const
    {
      return clipRange;
    }

  protected:
    IndexRange<2> clipRange;
  };

}// namespace Ravl2

namespace fmt
{
  template <typename DataT>
  struct formatter<Ravl2::SummedAreaTableC<DataT>> : ostream_formatter {
  };
}// namespace fmt
