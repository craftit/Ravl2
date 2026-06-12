//
// Created by charles galambos on 12/06/2026.
//

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Image/Corner.hh"

namespace Ravl2
{

  //! @brief Harris corner detector (single scale).
  //! Computes the structure tensor (gradients at the differentiation scale sigmaD,
  //! integrated at sigmaI), the Harris response det(M) - k * trace(M)^2, then keeps
  //! 3x3 spatial maxima above a threshold expressed as a fraction of the strongest
  //! response in the image. Corner positions are refined to sub-pixel accuracy.
  //! Thread-safe: apply() is const and keeps no state between calls.
  class CornerDetectorHarris
  {
  public:
    using RealT = Corner::RealT;

    //! Constructor.
    //! @param threshold Minimum response as a fraction of the maximum response in the image (0..1).
    //! @param sigmaD Differentiation scale in pixels.
    //! @param sigmaI Integration scale in pixels.
    //! @param k Harris response trace weighting.
    explicit CornerDetectorHarris(RealT threshold = RealT(0.005), RealT sigmaD = RealT(1.0),
                                  RealT sigmaI = RealT(2.0), RealT k = RealT(0.04));

    //! Get a list of corners from 'img'.
    //! The image is converted to float in [0,1] before processing.
    [[nodiscard]] std::vector<Corner> apply(const Array<uint8_t, 2> &img) const;

    //! Get a list of corners from 'img'. Values are expected to be in [0,1];
    //! Corner::level() is the pixel value scaled by 255 and clamped.
    [[nodiscard]] std::vector<Corner> apply(const Array<RealT, 2> &img) const;

  private:
    RealT mThreshold = RealT(0.005);
    RealT mSigmaD = RealT(1.0);
    RealT mSigmaI = RealT(2.0);
    RealT mK = RealT(0.04);
  };

}// namespace Ravl2
