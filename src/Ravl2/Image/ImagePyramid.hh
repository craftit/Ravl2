//
// Created by charles galambos on 06/08/2024.
//

#pragma once

#include "Ravl2/Geometry/ScaleTranslate.hh"
#include "Ravl2/Image/WarpScale.hh"
#include "Ravl2/Image/SummedAreaTable.hh"
#include "Ravl2/Image/ImageExtend.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! A single level in an image pyramid.

  template <typename ImageT, typename TransformT = ScaleTranslate<float, 2>>
  class PyramidLevel
  {
  public:
    PyramidLevel() = default;

    //! Create a pyramid level with a scale and an image.
    //! @param org2level Transform to map a point from the original image to the level image.
    //! @param img The scaled image.
    PyramidLevel(const ImageT &img, const TransformT &org2level)
        : mTransformTo(org2level),
          mTransformFrom(inverse(org2level)),
          mImage(img)
    {}

    //! Access the scale and translation of the image.
    //! @return Transform to map a point from the original image to the level image.
    [[nodiscard]] const auto &transformTo() const
    {
      return mTransformTo;
    }

    //! Access the scale and translation of the image.
    //! @return Transform to map a point from the original image to the level image.
    [[nodiscard]] const auto &transformFrom() const
    {
      return mTransformFrom;
    }

    //! Access the image.
    //! @return The image.
    [[nodiscard]] const auto &image() const
    {
      return mImage;
    }

  private:
    TransformT mTransformTo;
    TransformT mTransformFrom;
    ImageT mImage;
  };

  //! @brief Image pyramid
  //! This assumes the image at each level is a scaled version of the original image.
  //! So any point in the original image can be mapped to a point in the level image.

  template <typename ImageT, typename TransformT = ScaleTranslate<float, 2>>
  class ImagePyramid
  {
  public:
    //! Default constructor.
    constexpr ImagePyramid() = default;

    //! Create a pyramid with a single level.
    //! @param level The level to add to the pyramid.
    constexpr explicit ImagePyramid(const PyramidLevel<ImageT, TransformT> &level)
    {
      addLevel(level);
    }

    //! Construct from a list of levels.
    //! @param levels The levels to add to the pyramid.
    constexpr explicit ImagePyramid(const std::vector<PyramidLevel<ImageT, TransformT>> &&levels)
        : mLevels(std::move(levels))
    {}

    //! Access a level in the pyramid.
    //! @param index The index of the level to access.
    //! @return The level at the given index.
    [[nodiscard]] constexpr const PyramidLevel<ImageT, TransformT> &level(size_t index) const
    {
      return mLevels[index];
    }

    //! Access an image in the pyramid.
    //! @param index The index of the level to access.
    //! @return The image at the given index.
    [[nodiscard]] constexpr const auto &image(size_t index) const {
      return mLevels[index].image();
    }

    //! Access transform to a level in the pyramid.
    //! @param index The index of the level to access.
    //! @return The transform to the level at the given index.
    [[nodiscard]] constexpr const auto &transformTo(size_t index) const {
      return mLevels[index].transformTo();
    }

    //! Access transform from a level in the pyramid.
    //! @param index The index of the level to access.
    //! @return The transform from the level at the given index.
    [[nodiscard]] constexpr const auto &transformFrom(size_t index) const {
      return mLevels[index].transformFrom();
    }

    //! Find the pyramid level that best matches the scale and translation.
    //! @param target Target scale.
    //! @return The pyramid level that best matches the scale and translation.
    [[nodiscard]] constexpr const PyramidLevel<ImageT, TransformT> &findLevel(const TransformT &target) const
    {
      if(mLevels.empty()) {
        throw std::runtime_error("ImagePyramid::findLevel: No levels in the pyramid.");
      }
      float best = std::numeric_limits<float>::max();
      const PyramidLevel<ImageT> *bestLevel = nullptr;
      for(const auto &level : mLevels) {
        const auto &levelScale = level.transform().scaleVector();
        auto reScale = (levelScale.scale() / target.scale());
        float diff = std::abs(1 - reScale.norm());
        if(diff < best) {
          best = diff;
          bestLevel = &level;
        }
      }
      return *bestLevel;
    }

    //! Find level at which the area is scaled by 'scale'
    [[nodiscard]]
    constexpr size_t findAreaScale(float scale) const {
      if(mLevels.empty()) {
        SPDLOG_WARN("ImagePyramid::findAreaScale is empty");
        throw std::runtime_error("ImagePyramid::findAreaScale is empty");
      }
      // This assumes the first entry is the original image.
      auto baseArea = float(mLevels[0].image().range().area());
      float bestScaleError = std::abs(1.0f - scale);
      size_t bestLevel = 0;
      for(size_t i = 1; i < mLevels.size(); ++i) {
        auto area = float(mLevels[i].image().range().area());
        auto scaleError = std::abs((area / baseArea) - scale);
        if(scaleError < bestScaleError) {
          bestScaleError = scaleError;
          bestLevel = i;
        }
      }
      return bestLevel;
    }

    //! Add a level to the pyramid.
    //! @param level The level to add.
    constexpr void addLevel(const PyramidLevel<ImageT, TransformT> &level)
    {
      mLevels.push_back(level);
    }

    //! Access the number of levels in the pyramid.
    //! @return The number of levels in the pyramid.
    [[nodiscard]] constexpr size_t numLevels() const
    {
      return mLevels.size();
    }

  private:
    std::vector<PyramidLevel<ImageT, TransformT>> mLevels;
  };


  //! Create an image pyramid from an image.
  //! @param fullImg The image to create the pyramid from.
  //! @param sumImg The summed area table of the image.
  //! @param numLevels Maximum number of levels in the pyramid.
  //! @param scale The scale factor between levels.
  //! @param pad The amount of padding to add to the image.
  //! @param minArea The minimum area of a level, if the area is less than this the pyramid is truncated.
  //! @return The image pyramid.
  template <typename PixelT, typename DataT,typename TransformT = ScaleTranslate<float, 2>>
  ImagePyramid<Array<PixelT,2>, TransformT> buildImagePyramid(
    const Array<PixelT,2> &fullImg,
    const SummedAreaTable<DataT> &subImg,
    size_t numLevels,
    Vector<float, 2> scale,
    unsigned pad = 0,
    int minArea = 16
    )
  {
    ImagePyramid<Array<PixelT,2>, TransformT> pyramid;
    TransformT levelScale;
    // Put in the first level.
    pyramid.addLevel(PyramidLevel(fullImg, levelScale));
    for(size_t i = 1; i < numLevels; ++i) {
      levelScale.scale(1.0f / scale);
      IndexRange<2> sampleRange = toInnerIndexRange(levelScale(toRange<float>(fullImg.range()))).shrinkMax(1);
      SPDLOG_TRACE("Level {} Scale: {}  sample:{} ",i, levelScale,sampleRange);
      if(sampleRange.area() < minArea) {
        break;
      }
      IndexRange<2> fullRange = sampleRange.expand(int(pad));
      Array<PixelT, 2> newImage(fullRange);
      subImg.template sampleGrid<float>(clip(newImage,sampleRange), scale);
      if(pad > 0) {
        // If we've padded the image then we need to set the padding to zero.
        mirrorEdges(newImage, pad);
      }
      pyramid.addLevel(PyramidLevel(newImage, levelScale));
    }
    return pyramid;
  }

  //! Create an image pyramid from an image.
  //! @param fullImg The image to create the pyramid from.
  //! @param numLevels Maximum number of levels in the pyramid.
  //! @param scale The scale factor between levels.
  //! @param pad The amount of padding to add to the image.
  //! @param minArea The minimum area of a level, if the area is less than this the pyramid is truncated.
  //! @return The image pyramid.
  template <typename ImageT, typename DataT = typename ImageT::value_type,  typename TransformT = ScaleTranslate<float, 2>>
    requires WindowedArray<ImageT, DataT, 2>
  ImagePyramid<ImageT, TransformT> buildImagePyramid(
    const ImageT &img,
    size_t numLevels,
    Vector<float, 2> scale,
    unsigned pad = 0,
    int minArea = 16)
  {
    if constexpr(std::is_floating_point_v<DataT>) {
      auto sumImg = SummedAreaTable<double>::buildTable(img);
      return buildImagePyramid(img, sumImg, numLevels, scale, pad, minArea);
    } else {
      auto sumImg = SummedAreaTable<int64_t>::buildTable(img);
      return buildImagePyramid(img, sumImg, numLevels, scale, pad, minArea);
    }
  }

}// namespace Ravl2
