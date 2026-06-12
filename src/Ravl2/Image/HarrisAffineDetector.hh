//
// Created by charles galambos on 12/06/2026.
//

#pragma once

#include <vector>
#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Ellipse.hh"
#include "Ravl2/Image/NormalisedPatch.hh"
#include "Ravl2/Image/StructureTensor.hh"
#include "Ravl2/Image/Warp.hh"

namespace Ravl2
{

  //! @brief An affine-covariant feature: an ellipse-shaped image region.
  //! Produced by HarrisAffineDetector. The region is the ellipse obtained by mapping
  //! the unit circle through norm2img(): p_img = regionScale * scale * shape * p_norm + position.
  struct AffineFeature {
    Point<float, 2> position = {0, 0};//!< Centre in original image coordinates (row, col).
    float scale = 1.0f;               //!< Characteristic scale sigma in original-image pixels.
    Matrix<float, 2, 2> shape = Matrix<float, 2, 2>::Identity();
    //!< Anisotropic part of the region, symmetric positive definite with det == 1.
    float response = 0.0f;   //!< Harris response at the detection.
    float logResponse = 0.0f;//!< Scale-normalised |LoG| at the characteristic scale.

    //! Affine mapping the normalised frame (unit circle) to the image-space ellipse.
    //! @param regionScale Region radius in units of the characteristic scale.
    [[nodiscard]] Affine<float, 2> norm2img(float regionScale = 1.0f) const
    {
      return Affine<float, 2>(shape * (regionScale * scale), position);
    }

    //! The region as an ellipse in image coordinates.
    [[nodiscard]] Ellipse<float> ellipse(float regionScale = 1.0f) const
    {
      return Ellipse<float>(norm2img(regionScale));
    }
  };

  //! Tunables for HarrisAffineDetector.
  struct HarrisAffineConfig {
    float k = 0.04f;               //!< Harris response trace weighting.
    float threshold = 0.005f;      //!< Harris threshold as a fraction of the per-level maximum response.
    float logThreshold = 0.001f;   //!< Minimum scale-normalised |LoG| to accept a characteristic scale.
    float sigma0 = 1.4f;           //!< First integration scale (pixels).
    float scaleStep = 1.4f;        //!< Multiplicative sigma step between scale levels.
    unsigned numScales = 8;        //!< Number of scale levels.
    float sigmaDRatio = 0.7f;      //!< sigmaD = sigmaDRatio * sigmaI (Mikolajczyk).
    unsigned maxIterations = 10;   //!< Affine adaptation iteration cap.
    float convergenceRatio = 0.95f;//!< Accept when sqrt(lambdaMin/lambdaMax) of mu reaches this.
    float maxAxisRatio = 6.0f;     //!< Reject when the accumulated shape's axis ratio exceeds this.
    int patchRadius = 16;          //!< Half-size in pixels of the normalised working patch.
    size_t maxFeatures = 400;      //!< Adapt only the strongest N Harris-Laplace points.
  };

  //! @brief Mikolajczyk-Schmid Harris-Affine detector.
  //! Detects Harris-Laplace interest points (multi-scale Harris corners at their
  //! characteristic LoG scale) and refines each into an affine-covariant elliptical
  //! region by iterative second-moment-matrix shape adaptation.
  //! Thread-safety: apply() reuses internal scratch buffers; use one instance per
  //! thread (like SegmentExtrema).
  class HarrisAffineDetector
  {
  public:
    HarrisAffineDetector() = default;

    //! Construct with configuration.
    explicit HarrisAffineDetector(const HarrisAffineConfig &config);

    //! Detect affine features. The image is converted to float in [0,1] first.
    [[nodiscard]] std::vector<AffineFeature> apply(const Array<uint8_t, 2> &img);

    //! Detect affine features. Pixel values are expected to be roughly in [0,1].
    [[nodiscard]] std::vector<AffineFeature> apply(const Array<float, 2> &img);

    //! Access the configuration.
    [[nodiscard]] const HarrisAffineConfig &config() const
    {
      return mConfig;
    }

    //! @brief Extract the affine-normalised patch around a feature.
    //! Warps the feature's image-space ellipse (at 'regionScale') onto the circle
    //! inscribed in 'target', undoing the local affine deformation. This is the
    //! intended input for descriptor computation.
    //! @param target Output patch; its range defines the patch size and must not be empty.
    //! @param source Image to sample, in the same coordinates the feature was detected in.
    //! @param feature The feature defining the region.
    //! @param regionScale Region radius in units of the characteristic scale.
    //! @return false if sampling would leave the source image.
    template <typename PixelT>
    [[nodiscard]] static bool extractNormalisedPatch(Array<PixelT, 2> &target,
                                                     const Array<PixelT, 2> &source,
                                                     const AffineFeature &feature,
                                                     float regionScale = 3.0f)
    {
      return warpNormalisedPatch(target, source, feature.norm2img(regionScale));
    }

  private:
    //! One sigma level of the scale space, all planes at full image resolution.
    struct ScaleLevel {
      float sigmaI = 0;
      float sigmaD = 0;
      Array<float, 2> smoothed;//!< g(sigmaI) * I, same range as the input image.
      Array<float, 2> harris;  //!< Harris response, range shrunk by 1.
      Array<float, 2> logImg;  //!< sigmaI^2 * |Lrr + Lcc|, range shrunk by 1.
    };

    //! Build the Gaussian scale space and per-level Harris / LoG planes.
    void buildScaleSpace(const Array<float, 2> &img);

    //! Harris-Laplace point selection over the scale space.
    void detectHarrisLaplace(std::vector<AffineFeature> &out) const;

    //! Iterative affine shape adaptation of a single feature.
    //! @return false if the feature should be rejected.
    bool adaptShape(AffineFeature &feature, const Array<float, 2> &img);

    //! Greedy suppression of near-duplicate features, strongest first.
    //! 'features' must be sorted by descending response.
    void deduplicate(std::vector<AffineFeature> &features) const;

    HarrisAffineConfig mConfig;
    std::vector<ScaleLevel> mLevels;            //!< Scratch, reused across apply() calls.
    StructureTensorWorkspace<float> mTensorWork;//!< Scratch for structure tensors.
    Array<float, 2> mTensorRR;
    Array<float, 2> mTensorRC;
    Array<float, 2> mTensorCC;
    Array<float, 2> mPatch;  //!< Normalised-frame working patch for adaptation.
    Array<float, 2> mPatchRR;//!< Patch-frame tensor planes, kept separate from the
    Array<float, 2> mPatchRC;//!< full-image ones so neither set thrashes the other's
    Array<float, 2> mPatchCC;//!< allocation between scale-space and adaptation phases.
    Array<float, 2> mPatchHarris;
  };

}// namespace Ravl2
