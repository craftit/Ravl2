//
// Created by charles galambos on 08/08/2024.
//

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Image/BilinearInterpolation.hh"

namespace Ravl2
{

  //! @brief Assignment pixel operation
  //! This operation is used to assign the pixel value from the source to the target
  //! @param Target The target pixel to assign to
  //! @param Source The source pixel to assign from
  template <typename TargetT, typename SourceT>
  struct AssignOp
  {
    void operator()(TargetT &Target, const SourceT &Source) const
    {
      Target = Source;
    }
  };

  //! @brief Warp an image using a point transform
  //! This class is used to warp an image using a point-to-point mapping.
  //! @param target The target image to warp to iterate over on a grid.
  //! @param source The source image to warp points from
  //! @param transform The point-to-point mapping to use
  //! @param operation The operation to use to combine the pixels
  template <
    typename TargetArrayT,
    typename SourceArrayT,
    typename TransformT,
    typename SamplerT,
    PixelCoordinateSystemT pixCoordSystem = PixelCoordinateSystemT::Center,
      typename OperationT = AssignOp<typename TargetArrayT::value_type, typename SourceArrayT::value_type>
      >
    requires WindowedArray<TargetArrayT, typename TargetArrayT::value_type, TargetArrayT::dimensions> &&
    WindowedArray<SourceArrayT, typename SourceArrayT::value_type, SourceArrayT::dimensions>
  void warp(TargetArrayT &target, const SourceArrayT &source, const TransformT &transform,
	    OperationT &&operation = OperationT(),SamplerT &&sampler = interpolateBilinear<SourceArrayT,typename TransformT::PointT>)
  {
    //

    // Iterate over the target image
    for (auto it = target.begin(); it.valid();)
    {
      do {
	// Get the point in the target image
	auto targetPoint = it.index();

	// Get the point in the source image
	auto sourcePoint = transform(targetPoint);

	// Check if the source point is within the source image
	if(source.range().contains(sourcePoint)) {
	  // Get the pixel value from the source image
	  auto sourcePixel = sampler(source,sourcePoint);

	  // Assign the pixel value to the target image
	  operation(*it, sourcePixel);
	}
      } while(it.next());
    }
  }



} // Ravl2
