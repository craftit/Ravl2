// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

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
    requires WindowedArray<Array1T, InT, N> && WindowedArray<Array1T, OutT, N> && (N >= 2)
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
    IndexRange<2> rng(
      IndexRange<1>(intCeil(RealT(imgFrame[0].min()) / RealT(scale[0])),
                    intFloor(RealT(imgFrame[0].max() - 0) / RealT(scale[0]))),
      IndexRange<1>(intCeil(RealT(imgFrame[1].min()) / RealT(scale[1])),
                    intFloor(RealT(imgFrame[1].max() - 0) / RealT(scale[1]))));

    if(!result.range().contains(rng)) {
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
    if(result.range().empty()) {
      const IndexRange<2> &imgFrame = img.range();
      IndexRange<2> rng(
      IndexRange<1>(intCeil(RealT(imgFrame[0].min()) / scale[0]),
                      intFloor(RealT(imgFrame[0].max() - 0) / scale[0])),
      IndexRange<1>(intCeil(RealT(imgFrame[1].min()) / scale[1]),
                      intFloor(RealT(imgFrame[1].max() - 0) / scale[1])));
      result = Array<OutT, 2>(rng);
    }
    //cout << "res frame:" << result.range() << std::endl;
    Point2f origin = toPoint<float>(RealT(result.range().min(0)) * scale[0], RealT(result.range().min(1)) * scale[1]);
    //cout << "origin:" << origin << std::endl;

    // Reference implementation
    //    Point2f rowStart = origin;
    //    for(Array2dIterC<OutT> it(result);it;) {
    //      Point2f pnt = rowStart;
    //      do {
    //        BilinearInterpolation(img,pnt,*it);
    //        pnt[1] += scale[1];
    //      } while(it.next_col()); // True while in same row.
    //      rowStart[0] += scale[0];
    //    }
    Point2f rowStart = origin;
    for(auto it = result.begin(); it.valid();) {
      Point2f pnt = rowStart;

      int fx = intFloor(pnt[0]);// Row
      int fxp1 = fx + 1;
      if(fxp1 >= img.range().size(0)) fxp1 = fx;
      RealT u = pnt[0] - RealT(fx);
      if(u < RealT(1e-5)) {
	do {
	  int fy = intFloor(pnt[1]);// Col
	  RealT t = pnt[1] - RealT(fy);
	  if(t < RealT(1e-5)) {
	    const InT *pixel1 = &(img)[fx][fy];
	    *it = OutT(pixel1[0]);
	    pnt[1] += scale[1];
	  } else {
	    RealT onemt = (1.0f - t);

	    //printf("x:%g  y:%g  fx:%i  fy:%i\n", pnt[0], pnt[1], fx, fy);
	    const InT *pixel1 = &(img)[fx][fy];
	    *it = OutT((pixel1[0] * onemt) + (pixel1[1] * t));
	    pnt[1] += scale[1];
	  }
	} while(it.next());// True while in same row.
      } else {
	RealT onemu = (RealT(1.0) - u);
	do {
	  int fy = intFloor(pnt[1]);// Col
	  RealT t = pnt[1] - RealT(fy);
	  if(t < RealT(1e-5)) {
	    const InT *pixel1 = &(img)[fx][fy];
	    const InT *pixel2 = &(img)[fxp1][fy];
	    *it = OutT((pixel1[0] * onemu) + (pixel2[0] * u));
	    pnt[1] += scale[1];
	  } else {
	    RealT onemt = (1.0f - t);

	    //printf("x:%g  y:%g  fx:%i  fy:%i\n", pnt[0], pnt[1], fx, fy);
	    const InT *pixel1 = &(img)[fx][fy];
	    const InT *pixel2 = &(img)[fxp1][fy];
	    *it = OutT((pixel1[0] * (onemt * onemu)) + (pixel1[1] * (t * onemu)) + (pixel2[0] * (onemt * u)) + (pixel2[1] * (t * u)));
	    pnt[1] += scale[1];
	  }
	} while(it.next());// True while in same row.
      }

      rowStart[0] += scale[0];
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
    if(result.range().empty()) return false;
    // Distance between samples in the input image.
    Vector2f scale({float(img.range(0).size()) / float(result.range(0).size()),
		    float(img.range(1).size()) / float(result.range(1).size())});
    // This wil call warpSubsample if needed.
    return warpScaleBilinear(result, img,scale);
  }


}// namespace Ravl2
