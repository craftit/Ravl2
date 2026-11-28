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
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Image/DrawFrame.hh"
#include "Ravl2/Image/BilinearInterpolation.hh"

namespace Ravl2
{

  //! Summed Area Table.
  //! This class allows the summing of any area in an image in constant time.
  //! The class builds the table with a single pass over the input image. Once
  //! this is done the sum of any area can be computed by looking up the corners
  //! of the rectangle.

  template <class DataT>
  class SummedAreaTable : public Array<DataT, 2>
  {
  public:
    //! Default constructor.
    SummedAreaTable() = default;

  private:
    //! Constructor
    SummedAreaTable(Array<DataT, 2> &&arr, IndexRange<2> newClipRange)
      : Array<DataT, 2>(std::move(arr)) ,
          mClipRange(newClipRange)
    {}
  public:

    //! Build table form an array of values.
    template <class InT>
    static SummedAreaTable<DataT> buildTable(const Array<InT, 2> &in)
    {
      IndexRange<2> rng(in.range().expand(1));
      SummedAreaTable<DataT> ret(Array<DataT, 2>(rng),in.range());
      DataT zero {};
      DrawFrame(ret, zero, rng);// We only really need the top row and left column cleared.
      if(in.range().empty())
        return ret;
      // table(r, c) = sum of in over [min0, r] x [min1, c]; the row and column before the image are 0, and the
      // row and column after it repeat the last ones (no more area), for interpolation at the edge.
      // (Images under 2x2 used to give an empty table, and the extra column and row held running sums of the
      // last image column and row, not totals.)
      const IndexRange<2> &img = in.range();
      for(int r = img.min(0); r <= img.max(0); ++r) {
        auto out = ret[r];
        auto above = ret[r - 1];
        auto src = in[r];
        DataT rowSum {};
        for(int c = img.min(1); c <= img.max(1); ++c) {
          rowSum += DataT(src[c]);
          out[c] = above[c] + rowSum;
        }
        out[img.max(1) + 1] = out[img.max(1)];
      }
      copy(ret[img.max(0) + 1], ret[img.max(0)]);
      return ret;
    }

    //! Calculate the sum of the pixel's in the rectangle 'range'.
    DataT sum(IndexRange<2> range) const
    {
      range.clipBy(mClipRange);
      if(range.area() == 0)
        return (*this)[this->range().min()];// Return 0.
      // Could speed this up by separating out row accesses ?
      auto top = range.range(0).min()-1;
      auto left = range.range(1).min()-1;
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      return (*this)(bottom,right) - (*this)(top,right) - (*this)(bottom,left) + (*this)(top,left);
    }

    //! Sample a grid of rectangles, pu the result in out
    //! @param out The array to put the result in.
    //! @param scale This is the scale factor to apply to the output.
    //! @param offset This is the point adds an offset in the input space to the sampled pixels.
    template <typename CoordT = double, typename RealT = float, typename ArrayT, typename PixelT = typename ArrayT::value_type>
      requires WindowedArray<ArrayT, PixelT, 2>
    ArrayT sampleGrid(ArrayT out, const Vector<RealT,2> pixelScale,Point<RealT,2> pixelOffset = toPoint<RealT>(0,0)) const
    {
      // Check it fits within the table.
      const Vector<CoordT,2> scale = pixelScale.template cast<CoordT>();
      const Vector<CoordT,2> offset =pixelOffset.template cast<CoordT>();
      const Range<CoordT,2> outRng = toRange<CoordT>(out.range()) ;
      //SPDLOG_INFO("SampleGrid: Out:{} IndexRng:{} Scale:{} Offset:{} ", outRng,out.range(), scale, offset);
      const Point<CoordT,2> binOffset = toPoint<CoordT>(-1,-1);
      const Range<CoordT,2> rng = Range<CoordT,2>(outRng.min().cwiseProduct(scale) + offset,outRng.max().cwiseProduct(scale) + offset);
      const auto areaNorm = CoordT(1)/(scale[0] * scale[1]);
      const auto indexBounds = rng.toIndexRange().shrinkMax(1);
      if(!mClipRange.contains(indexBounds)) {
        SPDLOG_WARN("SampleGrid: Out of bounds: {} Rng:{} Bounds:{} Array:{} Scale:{} ", indexBounds, rng, mClipRange,this->range(),scale);
        assert(mClipRange.contains(indexBounds));
        throw std::out_of_range("SampleGrid: Out of bounds");
      }
      // Is it worth caching the last row of interpolated values ?
      for(auto it = out.begin();it.valid();) {
        Point<CoordT,2> pnt = offset + toPoint<CoordT>(it.index()).cwiseProduct(scale) + binOffset;
        auto last0 = interpolateBilinear<CoordT>(*this,pnt + toVector<CoordT>(0,0));
        auto last1 = interpolateBilinear<CoordT>(*this,pnt + toVector<CoordT>(scale[0],0));
        do {
          auto val0 = interpolateBilinear<CoordT>(*this,pnt + toVector<CoordT>(0,scale[1]));
          auto val1 = interpolateBilinear<CoordT>(*this,pnt + toVector<CoordT>(scale[0],scale[1]) );
          if constexpr(std::is_integral_v<DataT>)  {
            *it = PixelT(intRound((val1 - val0 - last1 + last0) * areaNorm));
          } else {
            *it = PixelT((val1 - val0 - last1 + last0) * areaNorm);
          }
          last0 = val0;
          last1 = val1;
          pnt[1] += scale[1];
        } while(it.next());
      }
      return out;
    }

    //! Calculate the difference between two halfs of the rectangle split vertically.
    //! This mid-point is an absolute row location and should be within the rectangle.
    [[nodiscard]] DataT verticalDifference2(IndexRange<2> range, int mid) const
    {
      auto top = range.range(0).min();
      auto left = range.range(1).min();
      auto bottom = range.range(0).max();
      auto right = range.range(1).max();
      // (top half) - (bottom half), using the same corner convention as horizontalDifference2().
      return (*this)(top,left)
        + ((*this)(mid,right) - (*this)(mid,left)) * 2
        + (*this)(bottom,left)
        - (*this)(top,right)
        - (*this)(bottom,right);
    }

    //! Calculate the difference between two halves of the rectangle split horizontally.
    //! This mid-point is an absolute column location and should be within the rectangle.
    [[nodiscard]] DataT horizontalDifference2(IndexRange<2> range, int mid) const
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
    //! This mid-point is an absolute row location and should be within the rectangle.
    [[nodiscard]] DataT verticalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(1).contains(rng));
      IndexRange<2> rng2(range.range(0), rng);
      return sum(range) - sum(rng2);
    }

    //! Calculate the difference between two rectangles one lying inside the other in the horizontal dimension.
    //! This mid-point is an absolute column location and should be within the rectangle.
    [[nodiscard]] DataT horizontalDifference3(const IndexRange<2> &range, const IndexRange<1> &rng) const
    {
      RavlAssert(range.range(0).contains(rng));
      IndexRange<2> rng2(rng, range.range(1));
      return sum(range) - sum(rng2);
    }

    //! Return range of value positions.
    [[nodiscard]] const IndexRange<2> &ClipRange() const
    {
      return mClipRange;
    }

  protected:
    IndexRange<2> mClipRange;
  };


  extern template class SummedAreaTable<int>;
  extern template class SummedAreaTable<int64_t>;


}// namespace Ravl2

namespace fmt
{
  template <typename DataT>
  struct formatter<Ravl2::SummedAreaTable<DataT>> : ostream_formatter {
  };
}// namespace fmt
