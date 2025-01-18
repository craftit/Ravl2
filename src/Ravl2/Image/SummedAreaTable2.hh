// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="28/11/2002"

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Assert.hh"
#include "Ravl2/Image/Array2Sqr2Iter2.hh"
#include "Ravl2/Image/DrawFrame.hh"

namespace Ravl2
{

  //! @brief Summed area and sum of squares table.
  //! This class allows the summing of any area in an image in constant time.
  //! The class builds the table with a single pass over the input image. Once
  //! this is done the sum of any area can be computed by looking up the corners
  //! of the rectangle. <br>
  //! Note this class is more prone to overflow problems than normal SummedAreaTable's so
  //! care must be taken when selecting types to use for the template.

  template <class DataT, class RealT = DataT>
  class SummedAreaTable2C : public Array<Vector<DataT, 2>, 2>
  {
  public:
    //: Default constructor
    SummedAreaTable2C() = default;

    template <class InT>
    explicit SummedAreaTable2C(const Array<InT, 2> &in)
    {
      (*this) = BuildTable(in);
    }

  protected:
    //: Default constructor
    SummedAreaTable2C(IndexRange<2> rect, IndexRange<2> clipRect)
      : Array<Vector<DataT, 2>, 2>(rect),
        clipRange(clipRect)
    {}

    //: Create vector containing value and its square.
    template <class InT>
    inline static Vector<DataT, 2> SumAndSqr(const InT &data)
    {
      auto val = static_cast<DataT>(data);
      return { val, sqr(val)};
    }

  public:
    //: Build table form an array of values.
    template <class InT>
    static SummedAreaTable2C<DataT> BuildTable(const Array<InT, 2> &in)
    {
      SummedAreaTable2C<DataT> ret(in.range().expand(1), in.range());
      Vector<DataT, 2> zero  = Vector<DataT, 2>::Zero();
      DrawFrame(ret, zero, ret.range());// We only really need the top row and left column cleared.

      Array2dSqr2Iter2C<Vector<DataT, 2>, InT> it(clipUnsafe(ret,in.range()), in);
      // First pixel.
      it.DataTL1() = SumAndSqr(it.DataTL2());
      Vector<DataT, 2> rowSum = SumAndSqr(it.DataBL2());
      it.DataBL1() = it.DataTL1() + rowSum;
      // Do first row.
      do {
        it.DataTR1() = it.DataTL1() + SumAndSqr(it.DataTR2());
        rowSum += SumAndSqr(it.DataBR2());
        it.DataBR1() = it.DataTR1() + rowSum;
      } while(it.next());
      // Do rest of image.
      for(; it.valid();) {
        // Do beginning of row thing.
        rowSum = SumAndSqr(it.DataBL2());
        it.DataBL1() = it.DataTL1() + rowSum;
        // Do rest of row.
        do {
          rowSum += SumAndSqr(it.DataBR2());
          it.DataBR1() = it.DataTR1() + rowSum;
        } while(it.next());
      }
      return ret;
    }

    //: Calculate the sum and sum of squares of the pixel values in the rectangle 'range'.
    [[nodiscard]] Vector<DataT, 2> Sum(IndexRange<2> range) const
    {
      auto top = range.range(0).min();
      auto left = range.range(1).min();
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      range.clipBy(clipRange);
      if(range.area() == 0)
        return (*this)[this->range().min()];// Return 0.
      range.min(1)--;
      range.min(0)--;
      // Could speed this up by separating out row accesses ?
      return (*this)(bottom,right) - (*this)(top,right) - (*this)(bottom,left) + (*this)(top,left);
    }

    //: Calculate the sum of the pixel's in the rectangle 'range'.
    [[nodiscard]] RealT Sum1(IndexRange<2> range) const
    {
      auto top = range.range(0).min();
      auto left = range.range(1).min();
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      range.clipBy(clipRange);
      if(range.area() == 0)
        return (*this)[this->range().min()][0];// Return 0.
      range.min(1)--;
      range.min(0)--;
      // Could speed this up by seperating out row accesses ?
      return (*this)(bottom,right)[0] - (*this)(top,right)[0] - (*this)(bottom,left)[0] + (*this)(top,left)[0];
    }

    //: Calculate variance of the image in 'range'.
    [[nodiscard]] RealT Variance(IndexRange<2> range) const
    {
      Vector<DataT, 2> sum = Sum(range);
      RealT area = RealT(range.area());
      return (RealT(sum[1]) - sqr(RealT(sum[0])) / area) / (area - 1);
    }

    //: Calculate mean, variance and area of the image in 'range'.
    int MeanVariance(IndexRange<2> range, RealT &mean, RealT &var) const
    {
      Vector<DataT, 2> sum = Sum(range);
      int area = range.area();
      mean = RealT(sum[0]) / area;
      var = (RealT(sum[1]) - sqr(RealT(sum[0])) / area) / (area - 1);
      return area;
    }

    //: Calculate the difference between two halves of the rectangle split vertically.
    // This mid point is an absolute row location and should be within the rectangle.
    [[nodiscard]] Vector<DataT, 2> VerticalDifference2(IndexRange<2> range, int mid) const
    {
      auto top = range.range(0).min();
      auto left = range.range(1).min();
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      // Could speed this up by separating out row accesses ?
      return (*this)(top,left)
        + ((*this)[mid][range.max(1)] - (*this)[mid][range.min(1)]) * 2
        - (*this)(bottom,left)
        - (*this)(top,right)
        + (*this)(bottom,right);
    }

    //: Calculate the difference between two halves of the rectangle split horizontally.
    // This mid point is an absolute column location and should be within the rectangle.
    [[nodiscard]] Vector<DataT, 2> HorizontalDifference2(IndexRange<2> range, int mid) const
    {
      auto top = range.range(0).min();
      auto left = range.range(1).min();
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      // Could speed this up by seperating out row accesses ?
      return (*this)(top,left)
        + ((*this)[range.max(0)][mid] - (*this)[range.min(0)][mid]) * 2
        - (*this)(bottom,left)
        + (*this)(top,right)
        - (*this)(bottom,right);
    }

    //: Calculate the difference between two halves of the rectangle split vertically.
    // This mid-point is an absolute row location and should be within the rectangle.
    [[nodiscard]] Vector<DataT, 2> VerticalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(1).contains(rng));
      IndexRange<2> rng2(range.range(0), rng);
      return Sum(range) - Sum(rng2);
    }

    //: Calculate the difference between two rectangles one lying inside the other in the horizontal dimention.
    // This mid-point is an absolute column location and should be within the rectangle.
    [[nodiscard]] Vector<DataT, 2> HorizontalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(0).contains(rng));
      IndexRange<2> rng2(rng, range.range(1));
      return Sum(range) - Sum(rng2);
    }

    //: Set the clip range.
    void SetClipRange(IndexRange<2> &nClipRange)
    {
      clipRange = nClipRange;
    }

    //: Return range of value positions.
    [[nodiscard]] const IndexRange<2> &ClipRange() const
    {
      return clipRange;
    }

  protected:
    IndexRange<2> clipRange;
  };

  //: Write to text stream.
  template <class DataT>
  std::ostream &operator<<(std::ostream &strm, const SummedAreaTable2C<DataT> &data)
  {
    strm << static_cast<const Array<Vector<DataT, 2>,2> &>(data);
    return strm;
  }

  //: Read from text stream.
  template <class DataT>
  std::istream &operator>>(std::istream &strm, SummedAreaTable2C<DataT> &data)
  {
    strm >> static_cast<Array<Vector<DataT, 2>, 2> &>(data);
    IndexRange<2> clipRange = data.range();
    clipRange.min(1)++;
    clipRange.min(0)++;
    data.SetClipRange(clipRange);
    return strm;
  }

  extern template class SummedAreaTable2C<int64_t>;

}// namespace Ravl2

