//
// Created by charles galambos on 12/06/2026.
//

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Image/Warp.hh"

namespace Ravl2
{
  //! @brief Extract a normalised patch defined by a unit-circle-to-image affine.
  //! Warps the image region that 'norm2img' maps the unit circle onto, onto the
  //! circle inscribed in 'target' (so target's inscribed circle = the region).
  //! @param target Output patch; its range defines the patch size, must not be empty.
  //! @param source Image to sample.
  //! @param norm2img Affine mapping the unit circle (normalised frame) to the image region.
  //! @return false if sampling would leave the source image.
  template <typename PixelT>
  [[nodiscard]] bool warpNormalisedPatch(Array<PixelT, 2> &target, const Array<PixelT, 2> &source,
                                         const Affine<float, 2> &norm2img)
  {
    assert(target.range().size(0) > 0 && target.range().size(1) > 0);
    const float targetRadius = float(std::min(target.range().size(0), target.range().size(1))) / 2.0f;
    const Point<float, 2> targetCentre = toPoint<float>(target.range().center());
    const Matrix<float, 2, 2> srMatrix = norm2img.SRMatrix() / targetRadius;
    const Affine<float, 2> target2source(srMatrix, norm2img.translation() - srMatrix * targetCentre);
    return warp<WarpWrapMode::Stop>(target, source, target2source);
  }
}// namespace Ravl2
