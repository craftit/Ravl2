// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include <vector>

#include <algorithm>

#include <stdexcept>
#include "Ravl2/Array.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{



  namespace detail
  {
    //! Internal helper function for WarpSubsample

    template <class InT, class OutT>
    inline void
    WS_prepareRow(const Array<InT, 2> &img, int srcRowI, double srcColR, double scaleColR,
                  OutT *resPtr, int resCols)
    {
      //cerr << "srcRowI:" << srcRowI << endl;
      //cerr << "srcColR:" << srcColR << endl;
      //cerr << "scaleColR:" << scaleColR << endl;
      //cerr << "resCols:" << resCols << endl;
      int srcColI = intFloor(srcColR);
      double t = srcColR - srcColI;

      const InT *srcPtr = &(img)[srcRowI][srcColI];

      InT pixVal = *srcPtr;
      //cerr << "pixVal:" << pixVal << endl;
      srcPtr++;
      for(int i = 0; i < resCols; i++) {
        //cerr << "i:" << i << endl;
        //first partial pixel in the row
        const double onemt = 1. - t;
        OutT resPixel = OutT(pixVal) * onemt;

        //all full pixels
        const double srcLastColR = srcColR + scaleColR;
        const int srcLastColI = intFloor(srcLastColR);
        for(srcColI++; srcColI < srcLastColI; srcColI++) {
          resPixel += OutT(*srcPtr);
          srcPtr++;
        }

        //last partial pixel
        t = srcLastColR - srcLastColI;
        pixVal = *srcPtr;
        srcPtr++;
        //cerr << "t:" << t << endl;
        if(t > 1e-5) {
          resPixel += OutT(pixVal) * t;
        }

        *resPtr = resPixel;
        resPtr++;
        srcColR = srcLastColR;
        srcColI = srcLastColI;
      }
    }

    //! Internal helper function for WarpSubsample
    //! the only difference with previous function is that this adds pixels to result

    template <class InT, class OutT>
    void
    WS_prepareRowAdd(const Array<InT, 2> &img, int srcRowI, double srcColR, double scaleColR,
                     OutT *resPtr, int resCols)
    {
      int srcColI = intFloor(srcColR);
      double t = srcColR - srcColI;

      const InT *srcPtr = &(img)[srcRowI][srcColI];

      InT pixVal = *srcPtr;
      srcPtr++;
      for(int i = 0; i < resCols; i++) {
        //first partial pixel in the row
        const double onemt = 1. - t;
        OutT resPixel = OutT(pixVal) * onemt;

        //all full pixels
        const double srcLastColR = srcColR + scaleColR;
        const int srcLastColI = intFloor(srcLastColR);
        for(srcColI++; srcColI < srcLastColI; srcColI++) {
          resPixel += OutT(*srcPtr);
          srcPtr++;
        }

        //last partial pixel
        t = srcLastColR - srcLastColI;
        pixVal = *srcPtr;//this could read outside the row, but the value will not be used
        srcPtr++;
        if(t > 1e-5) {
          resPixel += OutT(pixVal) * t;
        }

        *resPtr += resPixel;
        resPtr++;
        srcColR = srcLastColR;
        srcColI = srcLastColI;
      }
    }
  }// namespace detail

  //! @brief Fast image subsample
  //! This function sub-samples an image by taking the average of the pixels in the input image
  //! over the area of the output pixel.
  //! This function won't do super-sampling with this function.
  //! @param img - input image
  //! @param scale - The scale is the distance between samples in the input image.
  //! @param result - output image, output image size is input size <i>divided</i> by <code>scale</code>

  template <typename Array1T, typename InT = typename Array1T::value_type,
            typename Array2T = Array1T, typename OutT = Array2T::value_type, unsigned N = Array1T::dimensions,
            typename RealAccumT = double>
    requires WindowedArray<Array1T, InT, N> && WindowedArray<Array2T, OutT, N> && (N >= 2)
    void
    warpSubsample(Array2T &result, const Array1T &img, Vector2f scale)
  {
    // We can't do super-sampling
    if(scale[0] < 1.0f || scale[1] < 1.0f) {
      SPDLOG_WARN("WarpSubsample: scale must be >= 1.0");
      throw std::runtime_error("WarpSubsample: scale must be >= 1.0");
    }
    using RealT = double;
    const IndexRange<2> &imgFrame = img.range();
    // Output pixel j averages source [j*scale, (j+1)*scale), which must lie within [min, max+1).
    IndexRange<2> rng(
      IndexRange<1>(intCeil(RealT(imgFrame[0].min()) / RealT(scale[0])),
                    intFloor(RealT(imgFrame[0].max() + 1) / RealT(scale[0])) - 1),
      IndexRange<1>(intCeil(RealT(imgFrame[1].min()) / RealT(scale[1])),
                    intFloor(RealT(imgFrame[1].max() + 1) / RealT(scale[1])) - 1));

    // A result window inside 'rng' is filled as given; anything else would read outside the image.
    if(result.range().empty() || !rng.contains(result.range())) {
      //! Can we resize the result?
      if constexpr(!std::is_same_v<Array2T, Array<OutT, N>>) {
        SPDLOG_WARN("Resulting image is too large");
        throw std::runtime_error("Resulting image is too large");
      } else {
	if(!result.range().empty()) {
	  SPDLOG_WARN("Resulting incompatible image size: {} vs {}", result.range(), rng);
	}
        result = Array<OutT, 2>(rng);
      }
    }

    //cout << "res frame:" << result.range() << std::endl;
    const auto origin = toPoint<RealT>(RealT(result.range().min(0)) * RealT(scale[0]), RealT(result.range().min(1)) * RealT(scale[1]));
    //cout << "origin:" << origin << std::endl;

    const auto resRows = size_t(result.range(0).size());
    const auto resCols = size_t(result.range(1).size());

    const RealAccumT norm = RealAccumT(1.0) / RealAccumT(scale[0] * scale[1]);

    std::vector<RealAccumT> bufferRow(resCols);
    std::vector<RealAccumT> bufferRes(resCols);

    //prepare row buffer
    RealAccumT srcRowR = RealAccumT(origin[0]);
    int srcRowI = intFloor(srcRowR);
    RealAccumT u = srcRowR - srcRowI;

    detail::WS_prepareRow(img, srcRowI, RealT(origin[1]), RealT(scale[1]), bufferRow.data(), int(resCols));
    //if(!CheckRow(buffer, resCols, scale[1])) return false;

    for(size_t j = 0; j < resRows; j++) {
      //cerr << "j:" << j << endl;
      //first partial row
      double onemu = 1. - u;
      for(size_t i = 0; i < resCols; i++) {
        bufferRes[i] = bufferRow[i] * onemu;
      }

      //all full rows
      const double srcLastRowR = srcRowR + double(scale[0]);
      const int srcLastRowI = intFloor(srcLastRowR);
      //cerr << "srcRowI:" << srcRowI << endl;
      //cerr << "srcLastRowI:" << srcLastRowI << endl;
      for(srcRowI++; srcRowI < srcLastRowI; srcRowI++) {
        //cerr << "srcRowI:" << srcRowI << endl;
        detail::WS_prepareRowAdd(img, srcRowI, double(origin[1]), double(scale[1]), bufferRes.data(), int(resCols));
      }

      //last partial pixel
      u = srcLastRowR - srcLastRowI;
      //cerr << "u:" << u << endl;
      if(u > 1e-5) {
        detail::WS_prepareRow(img, srcRowI, double(origin[1]), double(scale[1]), bufferRow.data(), int(resCols));
        //if(!CheckRow(buffer, resCols, scale[1])) return false;
        for(size_t i = 0; i < resCols; i++) {
          bufferRes[i] += bufferRow[i] * u;
        }
      } else {
        //check if we need buffer for next iteration
        if(j + 1 < resRows) {
          //cerr << "u srcRowI:" << srcRowI << endl;
          detail::WS_prepareRow(img, srcRowI, double(origin[1]), double(scale[1]), bufferRow.data(), int(resCols));
          //if(!CheckRow(buffer, resCols, scale[1])) return false;
        }
      }
      //if(!CheckRow(resRowPtr, resCols, scale[1]*scale[0])) return false;

      //copy and scale result
      OutT *resRowPtr = &(result[int(j) + result.range(0).min()][result.range(1).min()]);
      for(size_t i = 0; i < resCols; i++) {
        resRowPtr[i] = OutT(bufferRes[i] * norm);
      }

      srcRowR = srcLastRowR;
      srcRowI = srcLastRowI;
    }
  }


  //! Rescale an image by sampling points with bi-linear interpolation.
  //! This is good for up scaling images, it will use sub-sampling if the scale is less than 1.
  //! If the scale is greater than 1 in one dimension and less than 1 in the other, it
  //! falls back to bi-linear interpolation.
  //! @param: result - output image
  //! @param: img - input image
  //! @param: scale - output image size is input size <i>divided</i> by <code>scale</code>
  //! If <code>result</code> is empty, the correct size is computed.

  template <class InT, class OutT, typename RealT = float>
  bool
  warpScaleBilinear(Array<OutT, 2> &result, const Array<InT, 2> &img, Vector2f scale)
  {
    //call subsampling function
    //cout << "WarpScaleBilinear scale:" << scale << std::endl;
    if(scale[0] >= 1.0f && scale[1] >= 1.0f) {
      warpSubsample(result, img, scale);
      return true;
    }
    //warpSubsample(Array2T &result, const Array1T &img, Vector2f scale)

    //cout << "src frame:" << img.range() << std::endl;
    // Output pixel j samples the source at j*scale, which must be within [min, max]. The tolerance stops float
    // rounding in the step (e.g. warpScale's (in-1)/(out-1)) from excluding the pixel that lands on the edge.
    const IndexRange<2> &imgFrame = img.range();
    constexpr RealT tolerance = RealT(1e-3);
    IndexRange<2> rng(
    IndexRange<1>(intCeil(RealT(imgFrame[0].min()) / scale[0] - tolerance),
                    intFloor(RealT(imgFrame[0].max()) / scale[0] + tolerance)),
    IndexRange<1>(intCeil(RealT(imgFrame[1].min()) / scale[1] - tolerance),
                    intFloor(RealT(imgFrame[1].max()) / scale[1] + tolerance)));
    if(result.range().empty()) {
      result = Array<OutT, 2>(rng);
    } else if(!rng.contains(result.range())) {
      throw std::out_of_range("warpScaleBilinear: result range samples outside the image");
    }

    // Each position is computed from its index (not accumulated, which drifts) and clamped to the image, and the
    // second neighbour is clamped in both dimensions, so no read leaves the image.
    struct Tap {
      int i0;
      int i1;
      RealT w;
    };
    auto tap = [](RealT pos, int lo, int hi) {
      pos = std::clamp(pos, RealT(lo), RealT(hi));
      int i0 = std::min(intFloor(pos), hi);
      RealT w = pos - RealT(i0);
      if(w < RealT(1e-5)) w = 0;
      return Tap {i0, std::min(i0 + 1, hi), w};
    };
    std::vector<Tap> cols;
    cols.reserve(size_t(result.range(1).size()));
    for(int c = result.range(1).min(); c <= result.range(1).max(); ++c)
      cols.push_back(tap(RealT(c) * scale[1], imgFrame[1].min(), imgFrame[1].max()));
    for(int r = result.range(0).min(); r <= result.range(0).max(); ++r) {
      const Tap row = tap(RealT(r) * scale[0], imgFrame[0].min(), imgFrame[0].max());
      const RealT u = row.w;
      const RealT onemu = RealT(1) - u;
      auto out = result[r];
      auto row0 = img[row.i0];
      auto row1 = img[row.i1];
      size_t k = 0;
      for(int c = result.range(1).min(); c <= result.range(1).max(); ++c, ++k) {
        const Tap &col = cols[k];
        const RealT t = col.w;
        if(u == 0 && t == 0) {
          out[c] = OutT(row0[col.i0]);
        } else {
          const RealT onemt = RealT(1) - t;
          out[c] = OutT((row0[col.i0] * (onemt * onemu)) + (row0[col.i1] * (t * onemu))
                        + (row1[col.i0] * (onemt * u)) + (row1[col.i1] * (t * u)));
        }
      }
    }
    return true;
  }

  //! Rescale an image
  //! @param: img - input image
  //! @param: result - output image
  //! This version computes the scaling factor from the input and output image sizes

  template <class InT, class OutT>
  bool
  warpScale(Array<OutT, 2> &result, const Array<InT, 2> &img)
  {
    if(result.range().empty() || img.range().empty()) return false;
    // Distance between samples in the input image, per dimension. Reducing uses in/out, so the subsampler's
    // averaging windows tile the image exactly. Enlarging uses (in-1)/(out-1), which puts the first and last
    // output samples on the first and last input pixels: with in/out the last sample fell past the image edge.
    auto step = [](int in, int out) {
      if(out > in && out > 1)
        return float(in - 1) / float(out - 1);
      return float(in) / float(out);
    };
    Vector2f scale({step(img.range(0).size(), result.range(0).size()),
		    step(img.range(1).size(), result.range(1).size())});
    // This wil call warpSubsample if needed.
    return warpScaleBilinear(result, img,scale);
  }


}// namespace Ravl2
