//
// Created by charles galambos on 08/08/2024.
//

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Image/BilinearInterpolation.hh"

namespace Ravl2
{

  //! @brief Assignment pixel operation
  //! This operation is used to assign the pixel value from the source to the target
  //! @param Target The target pixel to assign to
  //! @param Source The source pixel to assign from
  template <typename TargetT>
  struct AssignOp {
    template <typename SourceT>
    constexpr void operator()(TargetT &target, const SourceT &source) const
    {
      if constexpr (std::is_integral_v<TargetT> && std::is_floating_point_v<SourceT>) {
        target = intRound<SourceT,TargetT>(source);
      } else {
        target = TargetT(source);
      }
    }
  };

  //! @brief Get the range of pixels the transformed points will sample from
  //! @param targetRange The range to iterate over in a grid
  //! @param transform The point-to-point mapping to use
  //! @return The range of pixels the transformed points will sample from
  template <typename TransformT, unsigned N, typename CoordTypeT = float>
  Range<CoordTypeT, N> projectedBounds(const TransformT &transform, const IndexRange<N> &targetRange)
  {
    Range<CoordTypeT, N> transformedRange;
    for(unsigned i = 0; i < (1u << N); i++) {
      Point<CoordTypeT, N> pnt;
      for(unsigned j = 0; j < N; j++) {
        pnt[j] = CoordTypeT((i & (1u << j)) ? targetRange.max(j) : targetRange.min(j));
      }
      transformedRange.involve(transform(pnt));
    }
    return transformedRange;
  }

  //! @brief Wrap a point coordinate to the source image
  //! @param sourcePoint The point to wrap
  //! @param realSourceRange The range of the source image
  //! @return The wrapped point
  template <typename CoordTypeT, unsigned N>
  Point<CoordTypeT, N> wrapPoint(const Point<CoordTypeT, N> &sourcePoint, const Range<CoordTypeT, N> &realSourceRange)
  {
    Point<CoordTypeT, N> wrappedPoint = sourcePoint;
    for(unsigned i = 0; i < N; i++) {
      if(sourcePoint[i] < realSourceRange.min(i)) {
        auto diff = realSourceRange.min(i) - sourcePoint[i];
        wrappedPoint[i] += realSourceRange.size(i) * (1 + int(diff / realSourceRange.size(i)));
      } else if(sourcePoint[i] >= realSourceRange.max(i)) {
        auto diff = sourcePoint[i] - realSourceRange.max(i);
        wrappedPoint[i] -= realSourceRange.size(i) * (1 + int(diff / realSourceRange.size(i)));
      }
    }
    return wrappedPoint;
  }

  //! @brief Mirror a point coordinate in the source image
  //! @param sourcePoint The point to mirror
  //! @param realSourceRange The range of the source image
  //! @return The mirrored point

  template <typename CoordTypeT, unsigned N>
  Point<CoordTypeT, N> mirrorPoint(const Point<CoordTypeT, N> &sourcePoint, const Range<CoordTypeT, N> &realSourceRange)
  {
    Point<CoordTypeT, N> mirroredPoint = sourcePoint;
    for(unsigned i = 0; i < N; i++) {
      if(sourcePoint[i] < realSourceRange.min(i)) {
        auto diff = realSourceRange.min(i) - sourcePoint[i];
        auto numWraps = 1 + int(diff / realSourceRange.size(i));
        if(numWraps % 2 == 0) {
          mirroredPoint[i] = realSourceRange.min(i) + diff;
        } else {
          mirroredPoint[i] = realSourceRange.min(i) + realSourceRange.size(i) - diff;
        }
      } else if(sourcePoint[i] >= realSourceRange.max(i)) {
        auto diff = sourcePoint[i] - realSourceRange.max(i);
        auto numWraps = 1 + int(diff / realSourceRange.size(i));
        if(numWraps % 2 == 0) {
          mirroredPoint[i] = realSourceRange.max(i) - diff;
        } else {
          mirroredPoint[i] = realSourceRange.max(i) - realSourceRange.size(i) + diff;
        }
      }
    }
    return mirroredPoint;
  }

  //! @brief Enum for the strategy to use for points outside the source image
  enum class WarpWrapMode
  {
    Stop, //!< Stop the warp operation
    Leave,//!< Leave the target image unchanged, points are not filled or processed
    Fill, //!< Fill the target image with a fill value
    Clamp,//!< Clamp the point to the source image
    Wrap, //!< Wrap the point to the source image (experimental)
    Mirror//!< Mirror the point in the source image (experimental)
  };

  //! @brief Warp an image using a point transform
  //! This class is used to warp an image using a point-to-point mapping.
  //! This function assumes the transform is projective, so straight lines
  //! are preserved to avoid pixel by pixel checks.
  //! @param target The target image to warp to iterate over on a grid.
  //! @param source The source image to warp points from
  //! @param transform The point-to-point mapping to use
  //! @param operation The operation to use to combine the pixels
  //! @param sampler The method to use to sample the source image given a point
  //! @return True all of the target image was filled, false otherwise
  template <
    WarpWrapMode wrapMode = WarpWrapMode::Fill,
    typename CoordTypeT = float,
    typename TargetArrayT,
    typename SourceArrayT,
    typename TransformT,
    typename FillTypeT = typename TargetArrayT::value_type,
    typename PointT = Point<float, SourceArrayT::dimensions>,
    typename SamplerT = InterpolateBilinear<SourceArrayT, PointT>,
    typename OperationT = AssignOp<typename TargetArrayT::value_type>,
    unsigned N = SourceArrayT::dimensions>
    requires WindowedArray<TargetArrayT, typename TargetArrayT::value_type, TargetArrayT::dimensions> &&
             WindowedArray<SourceArrayT, typename SourceArrayT::value_type, SourceArrayT::dimensions>
  bool warp(TargetArrayT &target,
            const SourceArrayT &source,
            const TransformT &transform,
            const FillTypeT &fillValue = {},
            OperationT &&operation = OperationT(),
            SamplerT &&sampler = SamplerT())
  {
    // Get the range of pixels the transformed points will sample from
    auto realSourceRange = interpolationBounds<CoordTypeT>(source.range(), sampler);
    auto realSampleRange = projectedBounds(transform, target.range());
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
      if constexpr(wrapMode == WarpWrapMode::Stop) {
        return false;
      } else {
        // Iterate over the target image
        // We could project the polygon and iterate over that.
        for(auto it = target.begin(); it.valid();) {
          do {
            // Get the point in the target image
            auto targetIndex = it.index();

            // Get the point in the source image
            auto sourcePoint = transform(toPoint<CoordTypeT>(targetIndex));

            if constexpr(wrapMode == WarpWrapMode::Clamp) {
              // Clamp the point to the source image
              sourcePoint = clamp(sourcePoint, realSourceRange);
            } else if constexpr(wrapMode == WarpWrapMode::Wrap) {
              // Wrap the point to the source image
              sourcePoint = wrapPoint(sourcePoint, realSourceRange);
            } else if constexpr(wrapMode == WarpWrapMode::Mirror) {
              // Mirror the point in the source image
              sourcePoint = mirrorPoint(sourcePoint, realSourceRange);
            } else if constexpr(wrapMode == WarpWrapMode::Leave || wrapMode == WarpWrapMode::Fill) {
              // Check if the source point is within the source image
              if(realSourceRange.contains(sourcePoint)) {
                // Get the pixel value from the source image
                auto sourcePixel = sampler(source, sourcePoint);

                // Assign the pixel value to the target image
                operation(*it, sourcePixel);
              } else {
                if(wrapMode == WarpWrapMode::Fill) {
                  // Assign the fill value to the target image
                  operation(*it, fillValue);
                }
                // Otherwise WarpWrapMode::Leave, to leave the target image unchanged
              }
            }
          } while(it.next());
        }
      }
      return false;
    }
  }

}// namespace Ravl2
