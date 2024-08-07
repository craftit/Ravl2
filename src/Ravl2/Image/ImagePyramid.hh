//
// Created by charles galambos on 06/08/2024.
//

#pragma once

#include "Ravl2/Geometry/ScaleTranslate.hh"
#include "Ravl2/Image/WarpScale.hh"
//#include "Ravl2/Image/ImageExtend.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! A single level in an image pyramid.

  template<typename ImageT,typename TransformT = ScaleTranslate<float,2>>
  class PyramidLevel
  {
  public:
    PyramidLevel() = default;

    //! Create a pyramid level with a scale and an image.
    //! @param org2level Transform to map a point from the original image to the level image.
    //! @param img The scaled image.
    PyramidLevel(const TransformT &org2level, const ImageT &img)
      : mTransform(org2level),
        mImage(img)
    {}

    //! Access the scale and translation of the image.
    //! @return The scale and translation of the image.
    [[nodiscard]] const auto &transform() const
    {
      return mTransform;
    }

    //! Access the image.
    //! @return The image.
    [[nodiscard]] const auto &image() const
    {
      return mImage;
    }

  private:
    TransformT mTransform;
    ImageT mImage;
  };

  //! @brief Image pyramid
  //! This assumes the image at each level is a scaled version of the original image.
  //! So any point in the original image can be mapped to a point in the level image.

  template<typename ImageT,typename TransformT = ScaleTranslate<float,2> >
  class ImagePyramid
  {
  public:
    //! Default constructor.
    ImagePyramid() = default;

    //! Create a pyramid with a single level.
    //! @param level The level to add to the pyramid.
    explicit ImagePyramid(const PyramidLevel<ImageT,TransformT> &level)
    {
      addLevel(level);
    }

    //! Construct from a list of levels.
    //! @param levels The levels to add to the pyramid.
    explicit ImagePyramid(const std::vector<PyramidLevel<ImageT,TransformT>> &&levels)
      : mLevels(std::move(levels))
    {}

    //! Access a level in the pyramid.
    //! @param index The index of the level to access.
    //! @return The level at the given index.
    [[nodiscard]] const PyramidLevel<ImageT,TransformT> &level(size_t index) const
    {
      return mLevels[index];
    }

    //! Find the pyramid level that best matches the scale and translation.
    //! @param target Target scale.
    //! @return The pyramid level that best matches the scale and translation.
    [[nodiscard]] const PyramidLevel<ImageT,TransformT> &findLevel(const TransformT &target) const
    {
      if(mLevels.empty())
      {
	throw std::runtime_error("ImagePyramid::findLevel: No levels in the pyramid.");
      }
      float best = std::numeric_limits<float>::max();
      const PyramidLevel<ImageT> *bestLevel = nullptr;
      for (const auto &level : mLevels)
      {
	const auto &levelScale = level.transform().scaleVector();
	auto reScale = (levelScale.scale() / target.scale());
	float diff = std::abs(1 - reScale.norm());
	if (diff < best)
	{
	  best = diff;
	  bestLevel = &level;
	}
      }
      return *bestLevel;
    }

    //! Add a level to the pyramid.
    //! @param level The level to add.
    void addLevel(const PyramidLevel<ImageT,TransformT> &level)
    {
      mLevels.push_back(level);
    }

    //! Access the number of levels in the pyramid.
    //! @return The number of levels in the pyramid.
    [[nodiscard]] size_t numLevels() const
    {
      return mLevels.size();
    }

  private:
    std::vector<PyramidLevel<ImageT,TransformT>> mLevels;
  };

  //! Create an image pyramid from an image.
  //! @param img The image to create the pyramid from.
  //! @param numLevels The number of levels in the pyramid.
  //! @param scale The scale factor between levels.
  //! @return The image pyramid.
  template <typename ImageT, typename DataT = typename ImageT::value_type, unsigned N = ImageT::dimensions,typename TransformT = ScaleTranslate<float,2>>
  requires WindowedArray<ImageT, DataT, N>
  ImagePyramid<ImageT,TransformT> makeImagePyramid(
    const ImageT &img,
    size_t numLevels,
    Vector<float,2> scale,
    int pad = 0)
  {
    ImagePyramid<ImageT,TransformT> pyramid;
    TransformT levelScale;
    Array<DataT,N> levelImg;
    // Do we need to clone the initial image?
    if constexpr (std::is_same_v<ImageT, Array<DataT,N>>) {
      levelImg = img;
    } else {
      levelImg = clone(img);
    }
    // Put in the first level.
    pyramid.addLevel(PyramidLevel<ImageT,TransformT>(levelScale, levelImg));
    for(size_t i = 1; i < numLevels; ++i)
    {
      levelScale.scale(1.0f / scale);
      IndexRange<2> newRange = levelScale(img.range()).expand(pad);
      Array<DataT,N> newImage(img.range());
      warpScale(img, levelScale,newImage);
      if(pad > 0) {
	// If we've padded the image then we need to set the padding to zero.
      }
      pyramid.addLevel(PyramidLevel<ImageT,TransformT>(levelScale, newImage));
    }
    return pyramid;
  }


}
