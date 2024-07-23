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

    //: Rescale an image by sampling points with bilinear interpolation
    //!param: img - input image
    //!param: scale - output image size is input size <i>divided</i> by <code>scale</code>
    //!param: result - output image
    // If <code>result</code> is empty, the correct size is computed.

    template <class InT, class OutT>
    bool WarpScaleBilinear(const Array<InT,2> &img,
                           const Vector2f &scale, // Distance between samples in the input image.
                           Array<OutT,2> &result    // Output of scaling. The image must be of the appropriate size
    )
    {
      //call subsampling function
      //cout << "WarpScaleBilinear scale:" << scale << std::endl;
      if(scale[0] >= 1.0f && scale[1] >= 1.0f)
        return WarpSubsample(img, scale, result);

      //cout << "src frame:" << img.Frame() << std::endl;
      if(result.Frame().IsEmpty()) {
        const IndexRange<2> &imgFrame = img.Frame();
        IndexRange<2> rng(
                IndexRange<1>(int_ceil(imgFrame[0].min() / scale[0]),
                                int_floor((imgFrame[0].max() - 0) / scale[0])),
                IndexRange<1>(int_ceil(imgFrame[1].min() / scale[1]),
                                int_floor((imgFrame[1].max() - 0) / scale[1]))
                );
        result = Array<OutT,2>(rng);
      }
      //cout << "res frame:" << result.Frame() << std::endl;
      Point2f origin(result.Frame().TRow() * scale[0], result.Frame().LCol() * scale[1]);
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
      for(auto it = result.begin();it.valid();) {
        Point2f pnt = rowStart;

        int fx = int_floor(pnt[0]); // Row
        int fxp1 = fx + 1;
        if(fxp1 >= img.Rows()) fxp1 = fx;
        RealT u = pnt[0] - RealT(fx);
        if(u < RealT(1e-5)) {
          do {
            int fy = int_floor(pnt[1]); // Col
            RealT t = pnt[1] - RealT(fy);
            if(t < RealT(1e-5)) {
              const InT* pixel1 = &(img)[fx][fy];
              *it = OutT(pixel1[0]);
              pnt[1] += scale[1];
            } else {
              RealT onemt = (1.0f-t);

              //printf("x:%g  y:%g  fx:%i  fy:%i\n", pnt[0], pnt[1], fx, fy);
              const InT* pixel1 = &(img)[fx][fy];
              *it = OutT((pixel1[0] * onemt) +
                         (pixel1[1] * t));
              pnt[1] += scale[1];
            }
          } while(it.next()); // True while in same row.
        } else {
          RealT onemu = (RealT(1.0)-u);
          do {
            int fy = int_floor(pnt[1]); // Col
            RealT t = pnt[1] - RealT(fy);
            if(t < RealT(1e-5)) {
              const InT* pixel1 = &(img)[fx][fy];
              const InT* pixel2 = &(img)[fxp1][fy];
              *it = OutT((pixel1[0] * onemu) +
                         (pixel2[0] * u));
              pnt[1] += scale[1];
            } else {
              RealT onemt = (1.0f-t);

              //printf("x:%g  y:%g  fx:%i  fy:%i\n", pnt[0], pnt[1], fx, fy);
              const InT* pixel1 = &(img)[fx][fy];
              const InT* pixel2 = &(img)[fxp1][fy];
              *it = OutT((pixel1[0] * (onemt*onemu)) +
                         (pixel1[1] * (t*onemu)) +
                         (pixel2[0] * (onemt*u)) +
                         (pixel2[1] * (t*u)));
              pnt[1] += scale[1];
            }
          } while(it.next()); // True while in same row.
        }

        rowStart[0] += scale[0];
      }
      return true;
    }


    template <class InT, class OutT>
    bool WarpScaleBilinear(const Array<InT,2> &img,Array<OutT,2> &result)
    {
      if(result.Frame().IsEmpty()) return false;
      // Distance between samples in the input image.
      Vector2f scale({
		       float(img.Rows()) / float(result.Rows()),
		       float(img.Cols()) / float(result.Cols())
		     }
      );

      return WarpScaleBilinear(img, scale, result);
    }
    //: Rescale an image
    //!param: img - input image
    //!param: result - output image
    // This version computes the scaling factor from the input and output image sizes


    //:--
    //! userlevel=Develop
    template <class InT, class OutT>
    inline void WS_prepareRow(const Array<InT,2> &img, int srcRowI, double srcColR, double scaleColR,
                              OutT *resPtr, int resCols)
    {
      //cerr << "srcRowI:" << srcRowI << endl;
      //cerr << "srcColR:" << srcColR << endl;
      //cerr << "scaleColR:" << scaleColR << endl;
      //cerr << "resCols:" << resCols << endl;
      int srcColI = int_floor(srcColR);
      double t = srcColR - srcColI;

      const InT* srcPtr = &(img)[srcRowI][srcColI];

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
        const int srcLastColI = int_floor(srcLastColR);
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

    //the only difference with previous function is that this adds pixels to result
    template <class InT, class OutT>
    inline void WS_prepareRowAdd(const Array<InT,2> &img, int srcRowI, double srcColR, double scaleColR,
                                 OutT *resPtr, int resCols)
    {
      int srcColI = int_floor(srcColR);
      double t = srcColR - srcColI;

      const InT* srcPtr = &(img)[srcRowI][srcColI];

      InT pixVal = *srcPtr;
      srcPtr++;
      for(int i = 0; i < resCols; i++) {
        //first partial pixel in the row
        const double onemt = 1. - t;
        OutT resPixel = OutT(pixVal) * onemt;

        //all full pixels
        const double srcLastColR = srcColR + scaleColR;
        const int srcLastColI = int_floor(srcLastColR);
        for(srcColI++; srcColI < srcLastColI; srcColI++) {
          resPixel += OutT(*srcPtr);
          srcPtr++;
        }

        //last partial pixel
        t = srcLastColR - srcLastColI;
        pixVal = *srcPtr; //this could read outside the row, but the value will not be used
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

    template <class InT, class OutT, typename RealAccumT = double>
    bool WarpSubsample(const Array<InT,2> &img,
                       Vector2f scale, // Distance between samples in the input image.
                       Array<OutT,2> &result    // Output of scaling. The image must be of the appropriate size
    )
    {
      //cout << "WarpSubsample scale:" << scale << endl;
      //we can't do supersampling
      if(scale[0] < 1 || scale[1] < 1)
        return false;

      //cout << "src frame:" << img.Frame() << std::endl;

      const IndexRange<2> &imgFrame = img.Frame();
      IndexRange<2> rng(
              IndexRange<1>(int_ceil(imgFrame[0].min() / scale[0]),
                            int_floor((imgFrame[0].max() - 0) / scale[0])),
              IndexRange<1>(int_ceil(imgFrame[1].min() / scale[1]),
                            int_floor((imgFrame[1].max() - 0) / scale[1]))
      );

      if(result.Frame().IsEmpty()) {
        result = ImageC<OutT>(rng);
      } else {
        if(!rng.contains(result.Frame())) {
          std::cerr << "Resulting image is too large\n";
          return false;
        }
      }

      //cout << "res frame:" << result.Frame() << std::endl;
      const Point2f origin(result.Frame().TRow() * scale[0], result.Frame().LCol() * scale[1]);
      //cout << "origin:" << origin << std::endl;

      const int resRows = int(result.Rows());
      const int resCols = int(result.Cols());

      const RealAccumT norm = RealAccumT(1.0) / (scale[0] * scale[1]);

      std::vector<RealAccumT> bufferRow(resCols);
      std::vector<RealAccumT> bufferRes(resCols);

      //prepare row buffer
      RealAccumT srcRowR = RealAccumT(origin[0]);
      int srcRowI = int_floor(srcRowR);
      RealAccumT u = srcRowR - srcRowI;

      WS_prepareRow(img, srcRowI, origin[1], scale[1], bufferRow.data(), resCols);
      //if(!CheckRow(buffer, resCols, scale[1])) return false;

      for(int j = 0; j < resRows; j++) {
        //cerr << "j:" << j << endl;
        //first partial row
        double onemu = 1. - u;
        for(int i = 0; i < resCols; i++) {
          bufferRes[i] = bufferRow[i] * onemu;
        }

        //all full rows
        const double srcLastRowR = srcRowR + scale[0];
        const int srcLastRowI = int_floor(srcLastRowR);
        //cerr << "srcRowI:" << srcRowI << endl;
        //cerr << "srcLastRowI:" << srcLastRowI << endl;
        for(srcRowI++; srcRowI < srcLastRowI; srcRowI++) {
          //cerr << "srcRowI:" << srcRowI << endl;
          WS_prepareRowAdd(img, srcRowI, origin[1], scale[1], bufferRes.data(), resCols);
        }

        //last partial pixel
        u = srcLastRowR - srcLastRowI;
        //cerr << "u:" << u << endl;
        if(u > 1e-5) {
          WS_prepareRow(img, srcRowI, origin[1], scale[1], bufferRow.data(), resCols);
          //if(!CheckRow(buffer, resCols, scale[1])) return false;
          for(int i = 0; i < resCols; i++) {
            bufferRes[i] += bufferRow[i] * u;
          }
        } else {
          //check if we need buffer for next iteration
          if(j + 1 < resRows) {
            //cerr << "u srcRowI:" << srcRowI << endl;
            WS_prepareRow(img, srcRowI, origin[1], scale[1], bufferRow.data(), resCols);
            //if(!CheckRow(buffer, resCols, scale[1])) return false;
          }
        }
        //if(!CheckRow(resRowPtr, resCols, scale[1]*scale[0])) return false;

        //copy and scale result
        OutT *resRowPtr = &(result[j+result.TRow()][result.LCol()]);
        for(int i = 0; i < resCols; i++) {
          resRowPtr[i] = OutT(bufferRes[i] * norm);
        }

        srcRowR = srcLastRowR;
        srcRowI = srcLastRowI;
      }

      return true;
    }
    //: Fast image subsample


}


