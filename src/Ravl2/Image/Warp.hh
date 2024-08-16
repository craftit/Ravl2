//
// Created by charles galambos on 08/08/2024.
//

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Range.hh"
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

  //! @brief Get the range of pixels the transformed points will sample from
  //! @param targetRange The range to iterate over in a grid
  //! @param transform The point-to-point mapping to use
  //! @return The range of pixels the transformed points will sample from
  template <typename TransformT, unsigned N,typename CoordTypeT = float>
  Range<CoordTypeT,N> projectedBounds(const IndexRange<N> &targetRange, const TransformT &transform)
  {
    Range<CoordTypeT,N> transformedRange;
    for(unsigned i = 0; i < (1u<<N); i++) {
      Point<CoordTypeT,N> pnt;
      for(unsigned j = 0; j < N; j++) {
        pnt[j] = (i & (1u<<j)) ? targetRange.max(j) : targetRange.min(j);
      }
      transformedRange.involve(transform(pnt));
    }
    return transformedRange;
  }

  //! @brief Warp an image using a point transform
  //! This class is used to warp an image using a point-to-point mapping.
  //! @param target The target image to warp to iterate over on a grid.
  //! @param source The source image to warp points from
  //! @param transform The point-to-point mapping to use
  //! @param operation The operation to use to combine the pixels
  //! @param sampler The method to use to sample the source image given a point
  //! @return True all of the target image was filled, false otherwise
  template <
    typename TargetArrayT,
    typename SourceArrayT,
    typename TransformT,
    typename CoordTypeT = float,
    typename PointT = Point<float,SourceArrayT::dimensions>,
    typename SamplerT = InterpolateBilinear<SourceArrayT,PointT>,
    typename OperationT = AssignOp<typename TargetArrayT::value_type, typename SourceArrayT::value_type>
      >
    requires WindowedArray<TargetArrayT, typename TargetArrayT::value_type, TargetArrayT::dimensions> &&
    WindowedArray<SourceArrayT, typename SourceArrayT::value_type, SourceArrayT::dimensions>
  bool warp(TargetArrayT &target,
            const SourceArrayT &source,
            const TransformT &transform,
            const typename TargetArrayT::value_type &fillValue = {},
	    OperationT &&operation = OperationT(),
            SamplerT &&sampler = SamplerT())
  {
    // Get the range of pixels the transformed points will sample from
    // The reduced range is the one needed for bilinear interpolation, how to make this more general?
    auto realSourceRange = toRange<CoordTypeT>(source.range()).shrinkMax(CoordTypeT(1.0 + 1e-6));
    auto realSampleRange = projectedBounds(target.range(), transform);
    if(realSourceRange.contains(realSampleRange)) {
      // Iterate over the target image, no need for bounds check.
      for(auto it = target.begin(); it.valid();) {
        do {
          // Get the point in the target image
          auto targetIndex = it.index();

          // Get the point in the source image
          auto sourcePoint = transform(toPoint<CoordTypeT>(targetIndex));

          // Get the pixel value from the source image
          auto sourcePixel = sampler(source, sourcePoint);

          // Assign the pixel value to the target image
          operation(*it, sourcePixel);
        } while(it.next());
      }
      return true;
    } else {
      // Iterate over the target image
      // We could project the polygon and iterate over that.
      for(auto it = target.begin(); it.valid();) {
        do {
          // Get the point in the target image
          auto targetIndex = it.index();

          // Get the point in the source image
          auto sourcePoint = transform(toPoint<CoordTypeT>(targetIndex));

          // Check if the source point is within the source image
          if(realSourceRange.contains(sourcePoint)) {
            // Get the pixel value from the source image
            auto sourcePixel = sampler(source, sourcePoint);

            // Assign the pixel value to the target image
            operation(*it, sourcePixel);
          } else {
            // Assign the fill value to the target image
            operation(*it, fillValue);
          }
        } while(it.next());
      }
      return false;
    }
  }



} // Ravl2
