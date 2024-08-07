// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2005, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Image/ImageExtend.hh"
#include "Ravl2/Image/GaussConvolve2d.hh"
#include "Ravl2/Image/WarpScale.hh"

namespace Ravl2
{

  //: Image pyramid
  // This class creates a set of images each filtered such that the
  // effective resolution of each one is reduced by a constant factor from
  // the previous one.

  template <typename PixelT, typename SumTypeT = PixelT>
  class ImagePyramidC
  {
  public:
    ImagePyramidC() = default;
    //: Default constructor.

    ImagePyramidC(const Array<PixelT, 2> &img, int nScales = 2, bool subSample = false, bool recursive = false)
    {
      ComputeImages(img, nScales, subSample, recursive);
    }
    //: Constructor that creates a diadic pyramid
    // The scale factor between each neighbouring image pair = 2.
    //!param: img - Image to filter.
    //!param: nScales - Number of scales to generate. -1 = Generate a complete pyramid.
    //!param: subSample - If true sub-sample the pixels as well as filtering.
    //!param: recursive - If true when subsampling, then use the results of previous filtering operations as input for the next.

    ImagePyramidC(const Array<PixelT, 2> &img, RealT scaleFactor, int nScales = 2, bool subSample = false)
    {
      ComputeImages(img, scaleFactor, nScales, subSample);
    }
    //: Constructor for non-diadic pyramid
    //!param: img - Image to filter
    //!param: scaleFactor - the scale factor to apply between levels (2.0 == double size)
    //!param: nScales - Number of scales to generate. -1 = Generate a complete pyramid.
    //!param: subSample - If true sub-sample the pixels as well as filtering.

    void ComputeImages(const Array<PixelT, 2> &img, int nScales, bool subSample, bool recursive = false);
    //: Compute images over given scales.
    //!param: img - Image to filter.
    //!param: nScales - Number of scales to generate. -1 = Generate a complete pyramid.
    //!param: subSample - If true sub-sample the pixels as well as filtering.
    //!param: recursive - If true when subsampling, then use the results of previous filtering operations as input for the next.

    void ComputeImages(const Array<PixelT, 2> &img, RealT scaleFactor, int nScales, bool subSample);
    //: Compute images over given scales for non-diadic pyramids.
    //!param: img - Image to filter
    //!param: scaleFactor - the scale factor to apply between levels (2.0 == double size)
    //!param: nScales - Number of scales to generate. -1 = Generate a complete pyramid.
    //!param: subSample - If true sub-sample the pixels as well as filtering.

    Array<PixelT, 2> ScaleImage(const Array<PixelT, 2> &img, int scale, bool subSample, int imgScale = 1);
    //: Compute a scaled image and add it to the pyramid.
    //!param: img - Image to filter.
    //!param: scale - Scaling to apply.
    //!param: subSample - Sub sample the pixels in the image ?
    //!param: imgScale - scale of image passed to routine, use 1 if the image at the original scale.
    //!return: resulting image.

    Array<PixelT, 2> ScaleImage(const Array<PixelT, 2> &img, RealT scale, bool subSample);
    //: Compute a scaled image and add it to the pyramid.
    //!param: img - Image to filter.
    //!param: scale - Scaling to apply.
    //!param: subSample - Sub sample the pixels in the image ?
    //!return: resulting image.

    bool Find(RealT reqScale, Array<PixelT, 2> &img, RealT &filterScale, RealT &pixelScale, bool notSmaller = false) const;
    //: Find image with closest scale.
    //!param: reqScale - Requested scale.
    //!param: img - Image found
    //!param: actualScale - scale of image
    //!param: notSmaller - If true use the image with scale equal or larger size to that requested if one is available.
    //!return: Set to true if image found, only fails if class is not initialised.

    std::vector<std::tuple<RealT, RealT, Array<PixelT, 2>>> &Images()
    {
      return images;
    }
    //: Access available images.
    // The objects in the returned collection are tuples consisting of:<ul>
    // <li> the filter scale - i.e. the amount of filtering applied, <i>relative to the original image</i>,</li>
    // <li> the pixel scale relative to the original image,</li>
    // <li> the image itself

    const std::vector<std::tuple<RealT, RealT, Array<PixelT, 2>>> &Images() const
    {
      return images;
    }
    //: Access available images.
    // The objects in the returned collection are tuples consisting of:<ul>
    // <li> the filter scale - i.e. the amount of filtering applied, <i>relative to the original image</i>,</li>
    // <li> the pixel scale relative to the original image,</li>
    // <li> the image itself

  protected:
    std::vector<std::tuple<RealT, RealT, Array<PixelT, 2>>> images;
  };

  template <typename PixelT, typename SumTypeT>
  void ImagePyramidC<PixelT, SumTypeT>::ComputeImages(const Array<PixelT, 2> &img, int nscales, bool subSample, bool recursive)
  {
    images = std::vector<std::tuple<RealT, RealT, Array<PixelT, 2>>>(nscales + 1);
    images.Insert(std::tuple<RealT, RealT, Array<PixelT, 2>>(1, 1, img));
    //cerr << " No Scales=" << nscales << "\n";
    int minSize = Min(img.range().size(0), img.range().size(1));
    if(subSample && recursive) {
      Array<PixelT, 2> srcImg = img;
      int scale = 1;
      for(int i = 1; (i < nscales) || nscales < 0; i++) {
        if((scale * 4 - 1) >= minSize)
          break;
        srcImg = ScaleImage(srcImg, 2, true, scale);
        scale *= 2;
      }
    } else {
      for(int i = 1; (i < nscales) || nscales < 0; i++) {
        int scale = 1 << i;
        int kernelSize = scale * 2 - 1;
        if(kernelSize >= minSize)// No point in scaling beyond the image size.
          break;
        ScaleImage(img, scale, subSample, 1);
      }
    }
  }

  template <typename PixelT, typename SumTypeT>
  void ImagePyramidC<PixelT, SumTypeT>::ComputeImages(const Array<PixelT, 2> &img, RealT scaleFactor, int nScales, bool subSample)
  {
    images = std::vector<std::tuple<RealT, RealT, Array<PixelT, 2>>>(nScales + 1);
    images.Insert(std::tuple<RealT, RealT, Array<PixelT, 2>>(1, 1, img));
    int minSize = Min(img.range().size(0), img.range().size(1));
    RealT scale = 1.0;
    for(int i = 1; (i < nScales) || nScales < 0; i++) {
      scale *= scaleFactor;
      int kernelSize = (int)(scale * 2.0) - 1;
      if(!(kernelSize & 1))
        kernelSize++;
      if(kernelSize >= minSize)// No point in scaling beyond the image size.
        break;
      ScaleImage(img, scale, subSample);
    }
  }

  //: Generate a single filtered image.
  //!param: img - Image to filter.
  //!param: scale - Scaling to apply.
  //!param: subSample - Sub sample the pixels in the image ?
  //!param: imgScale - scale of image passed to routine, use 1 if the image at the original scale.

  template <typename PixelT, typename SumTypeT>
  Array<PixelT, 2> ImagePyramidC<PixelT, SumTypeT>::ScaleImage(const Array<PixelT, 2> &img, int scale, bool subSample, int imgScale)
  {
    int kernelSize = scale * 2 - 1;
    Array<PixelT, 2> prepImage;
    ExtendImageCopy(img, scale - 1, prepImage);
    GaussConvolve2dC<PixelT, PixelT, RealT, SumTypeT> filter(kernelSize);
    Array<PixelT, 2> filteredImage = filter.apply(prepImage);
    if(!subSample) {
      images.Insert(std::tuple<RealT, RealT, Array<PixelT, 2>>(scale * imgScale, imgScale, filteredImage));
      return filteredImage;
    }
    // FIXME:- This isn't the most efficient way of getting a subsampled image, we could
    // compute filtered values for the points we want in the final image.
    //cerr << " Scale=" << scale << "\n";
    IndexRange<2> alignedFrame = filteredImage.range().AlignWithin(scale);
    IndexRange<2> subFrame = alignedFrame / scale;
    Array<PixelT, 2> subImage(subFrame);
    //cerr << " Frame=" << alignedFrame << " SF=" << subFrame << "\n";
    Array2dIterC<PixelT> dit(subImage);
    for(Array2dIterC<PixelT> sit(Array<PixelT, 2>(filteredImage, alignedFrame)); sit; sit += scale, dit++)
      *dit = *sit;
    images.Insert(std::tuple<RealT, RealT, Array<PixelT, 2>>(scale * imgScale, scale * imgScale, subImage));
    return subImage;
  }

  //: Generate a single filtered image.
  //!param: img - Image to filter.
  //!param: scale - Scaling to apply.
  //!param: subSample - Sub sample the pixels in the image ?

  template <typename PixelT, typename SumTypeT>
  Array<PixelT, 2> ImagePyramidC<PixelT, SumTypeT>::ScaleImage(const Array<PixelT, 2> &img, RealT scale, bool subSample)
  {
    int kernelSize = (int)(scale * 2.0) - 1;
    if(!(kernelSize & 1))
      kernelSize++;
    Array<PixelT, 2> prepImage;
    if(((kernelSize - 1) >> 1) > 0)
      ExtendImageCopy(img, (kernelSize - 1) >> 1, prepImage);
    else
      prepImage = img;
    if(!subSample) {
      GaussConvolve2dC<PixelT, PixelT, RealT, SumTypeT> filter(kernelSize);
      Array<PixelT, 2> filteredImage = filter.apply(prepImage);
      images.Insert(std::tuple<RealT, RealT, Array<PixelT, 2>>(scale, 1.0, filteredImage));
      return filteredImage;
    }
    Array<SumTypeT, 2> filteredImage;
    if(kernelSize > 1) {
      GaussConvolve2dC<PixelT, SumTypeT, RealT, SumTypeT> filter(kernelSize);
      filteredImage = filter.apply(prepImage);
    } else {
      filteredImage = Array<SumTypeT, 2>(prepImage.range());
      for(Array2dIter2C<SumTypeT, PixelT> it(filteredImage, prepImage); it; it++)
        it.data<0>() = static_cast<SumTypeT>(it.data<1>());
    }
    // FIXME:- This isn't the most efficient way of getting a subsampled image, we could
    // compute filtered values for the points we want in the final image.
    //cerr << " Scale=" << scale << "\n";
    IndexRange<2> alignedFrame;
    if(((kernelSize - 1) >> 1) > 0)
      alignedFrame = filteredImage.range().AlignWithin((kernelSize - 1) >> 1);
    else
      alignedFrame = filteredImage.range();
    IndexRange<2> subFrame(alignedFrame.Range1().Min() / scale, alignedFrame.Range1().Max() / scale,
                           alignedFrame.Range2().Min() / scale, alignedFrame.Range2().Max() / scale);
    WarpScaleC<SumTypeT, PixelT> warpScale(subFrame);
    Array<PixelT, 2> subImage = warpScale.apply(Array<SumTypeT, 2>(filteredImage, alignedFrame));
    images.Insert(std::tuple<RealT, RealT, Array<PixelT, 2>>(scale, scale, subImage));
    return subImage;
  }

  //: Find image with closest scale.
  //!param: reqScale - Requested scale.
  //!param: img - Image found
  //!param: actualScale - scale of image

  template <typename PixelT, typename SumTypeT>
  bool ImagePyramidC<PixelT, SumTypeT>::Find(RealT reqScale, Array<PixelT, 2> &img, RealT &filterScale, RealT &pixelScale, bool notSmaller) const
  {
    CollectionIterC<std::tuple<RealT, RealT, Array<PixelT, 2>>> it(const_cast<std::vector<std::tuple<RealT, RealT, Array<PixelT, 2>>> &>(images));
    if(!it) return false;
    // The first image should be the unscaled. Which is the default if reqScale is less than 1 and notSmaller is set.
    RealT diff = std::abs(it -.data<0>() - reqScale);
    img = it -.data<2>();
    RealT bestScale = diff;
    filterScale = it -.data<0>();
    pixelScale = it -.data<1>();

    for(it++; it; it++) {
      diff = std::abs(it -.data<0>() - reqScale);
      if(diff < bestScale && (!notSmaller || it -.data<0>() < reqScale)) {
        img = it -.data<2>();
        bestScale = diff;
        filterScale = it -.data<0>();
        pixelScale = it -.data<1>();
      }
    }
    return true;
  }

}// namespace Ravl2
