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
#include "Ravl2/Image/Array2Sqr2Iter2.hh"
#include "Ravl2/Image/DrawFrame.hh"

namespace Ravl2
{

  //! Summed area and sum of squares table.
  // This class allows the summing of any area in an image in constant time.
  // The class builds the table with a single pass over the input image. Once
  // this is done the sum of any area can be computed by looking up the corners
  // of the rectangle. <br>
  // Note this class is more prone to overflow problems than normal SummedAreaTable's so
  // care must be taken when selecting types to use for the template.

  template <class DataT>
  class SummedAreaTable2C : public Array<Vector<DataT, 2>, 2>
  {
  public:
    SummedAreaTable2C()
    {}
    //: Default constructor

    template <class InT>
    SummedAreaTable2C(const Array<InT, 2> &in)
    {
      BuildTable(in);
    }

  protected:
    template <class InT>
    inline static Vector<DataT, 2> SumAndSqr(const InT &data)
    {
      Vector<DataT, 2> ret;
      DataT val = static_cast<DataT>(data);
      ret[0] = val;
      ret[1] = sqr(val);
      return ret;
    }
    //: Create vector containing value and its square.

  public:
    template <class InT>
    void BuildTable(const Array<InT, 2> &in)
    {
      clipRange = in.range();
      IndexRange<2> rng(in.range());
      rng.min(1)--;
      rng.min(0)--;
      if(this->Frame() != rng) {
        Vector<DataT, 2> zero;
        SetZero(zero);
        (*this).Array<Vector<DataT, 2, 2>>::operator=(Array<Vector<DataT, 2, 2>>(rng));
        DrawFrame((*this), zero, rng);// We only really need the top row and left column cleared.
      }
      Array<Vector<DataT, 2, 2>> work((*this), in.range());
      Array2dSqr2Iter2C<Vector<DataT, 2>, InT> it(work, in);
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
      for(; it;) {
        // Do beginning of row thing.
        rowSum = SumAndSqr(it.DataBL2());
        it.DataBL1() = it.DataTL1() + rowSum;
        // Do rest of row.
        do {
          rowSum += SumAndSqr(it.DataBR2());
          it.DataBR1() = it.DataTR1() + rowSum;
        } while(it.next());
      }
    }
    //: Build table form an array of values.

    Vector<DataT, 2> Sum(IndexRange<2> range) const
    {
      range.clipBy(clipRange);
      if(range.area() == 0)
        return (*this)[this->Frame().min()];// Return 0.
      range.min(1)--;
      range.min(0)--;
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.End()] - (*this)[range.TopRight()] - (*this)[range.BottomLeft()] + (*this)[range.TopLeft()];
    }
    //: Calculate the sum and sum of squares of the pixel values in the rectangle 'range'.

    RealT Sum1(IndexRange<2> range) const
    {
      range.clipBy(clipRange);
      if(range.area() == 0)
        return (*this)[this->Frame().min()][0];// Return 0.
      range.min(1)--;
      range.min(0)--;
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.End()][0] - (*this)[range.TopRight()][0] - (*this)[range.BottomLeft()][0] + (*this)[range.TopLeft()][0];
    }
    //: Calculate the sum of the pixel's in the rectangle 'range'.

    RealT Variance(IndexRange<2> range) const
    {
      Vector<DataT, 2> sum = Sum(range);
      RealT area = (RealT)range.area();
      return ((RealT)sum[1] - sqr((RealT)sum[0]) / area) / (area - 1);
    }
    //: Calculate variance of the image in 'range'.

    int MeanVariance(IndexRange<2> range, RealT &mean, RealT &var) const
    {
      Vector<DataT, 2> sum = Sum(range);
      int area = range.area();
      mean = (RealT)sum[0] / area;
      var = ((RealT)sum[1] - sqr((RealT)sum[0]) / area) / (area - 1);
      return area;
    }
    //: Calculate mean, variance and area of the image in 'range'.

    Vector<DataT, 2> VerticalDifference2(IndexRange<2> range, int mid) const
    {
      // Could speed this up by separating out row accesses ?
      return (*this)[range.TopLeft()]
        + ((*this)[mid][range.max(1)] - (*this)[mid][range.min(1)]) * 2
        - (*this)[range.BottomLeft()]
        - (*this)[range.TopRight()]
        + (*this)[range.End()];
    }
    //: Calculate the difference between two halfs of the rectangle split vertically.
    // This mid point is an absolute row location and should be within the rectangle.

    Vector<DataT, 2> HorizontalDifference2(IndexRange<2> range, int mid) const
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

    Vector<DataT, 2> VerticalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(1).contains(rng));
      IndexRange<2> rng2(range.range(0), rng);
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two halfs of the rectangle split vertially.
    // This mid point is an absolute row location and should be within the rectangle.

    Vector<DataT, 2> HorizontalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(0).contains(rng));
      IndexRange<2> rng2(rng, range.range(1));
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two rectangles one lying inside the other in the horizontal dimention.
    // This mid point is an absolute column location and should be within the rectangle.

    void SetClipRange(IndexRange<2> &nClipRange)
    {
      clipRange = nClipRange;
    }
    //: Set the clip range.

    const IndexRange<2> &ClipRange() const
    {
      return clipRange;
    }
    //: Return range of value positions.

  protected:
    IndexRange<2> clipRange;
  };

  template <class DataT>
  std::ostream &operator<<(std::ostream &strm, const SummedAreaTable2C<DataT> &data)
  {
    strm << static_cast<const Array<Vector<DataT, 2, 2>> &>(data);
    return strm;
  }
  //: Write to text stream.

  template <class DataT>
  std::istream &operator>>(std::istream &strm, SummedAreaTable2C<DataT> &data)
  {
    strm >> static_cast<Array<Vector<DataT, 2, 2>> &>(data);
    IndexRange<2> clipRange = data.range();
    clipRange.min(1)++;
    clipRange.min(0)++;
    data.SetClipRange(clipRange);
    return strm;
  }
  //: Read from text stream.

}// namespace Ravl2

