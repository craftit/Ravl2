//
// Created by charles galambos on 12/06/2026.
//

#include "Ravl2/Image/HarrisAffineDetector.hh"

#include <algorithm>
#include "Ravl2/Image/Convolve.hh"
#include "Ravl2/Image/PeakDetector.hh"

namespace Ravl2
{

  HarrisAffineDetector::HarrisAffineDetector(const HarrisAffineConfig &config)
      : mConfig(config)
  {
    assert(config.numScales >= 3);// Interior levels are needed for the scale extremum test.
    assert(config.scaleStep > 1.0f);
    assert(config.sigma0 > 0.0f);
  }

  std::vector<AffineFeature> HarrisAffineDetector::apply(const Array<uint8_t, 2> &img)
  {
    Array<float, 2> floatImg(img.range());
    for(int r : img.range(0)) {
      for(int c : img.range(1)) {
        floatImg[r][c] = float(img[r][c]) / 255.0f;
      }
    }
    return apply(floatImg);
  }

  std::vector<AffineFeature> HarrisAffineDetector::apply(const Array<float, 2> &img)
  {
    buildScaleSpace(img);
    std::vector<AffineFeature> candidates;
    detectHarrisLaplace(candidates);
    std::vector<AffineFeature> features;
    features.reserve(candidates.size());
    for(auto &candidate : candidates) {
      if(adaptShape(candidate, img)) {
        features.push_back(candidate);
      }
    }
    deduplicate(features);
    return features;
  }

  void HarrisAffineDetector::buildScaleSpace(const Array<float, 2> &img)
  {
    mLevels.resize(mConfig.numScales);
    const IndexRange<2> innerRange = img.range().shrink(1);
    for(unsigned i = 0; i < mConfig.numScales; ++i) {
      ScaleLevel &level = mLevels[i];
      level.sigmaI = mConfig.sigma0 * std::pow(mConfig.scaleStep, float(i));
      level.sigmaD = mConfig.sigmaDRatio * level.sigmaI;

      // Incremental smoothing: each level is computed from the previous one with
      // sigmaInc = sqrt(sigmaI^2 - sigmaPrev^2), keeping the kernels small.
      if(i == 0) {
        gaussianBlur(level.smoothed, img, level.sigmaI, mTensorWork.extended, mTensorWork.tmp);
      } else {
        const float sigmaPrev = mLevels[i - 1].sigmaI;
        const float sigmaInc = std::sqrt(level.sigmaI * level.sigmaI - sigmaPrev * sigmaPrev);
        gaussianBlur(level.smoothed, mLevels[i - 1].smoothed, sigmaInc, mTensorWork.extended, mTensorWork.tmp);
      }

      // Harris response at this scale pair, computed from the original image.
      structureTensor(mTensorRR, mTensorRC, mTensorCC, img, level.sigmaD, level.sigmaI, mTensorWork);
      harrisResponse(level.harris, mTensorRR, mTensorRC, mTensorCC, mConfig.k);

      // Scale-normalised Laplacian-of-Gaussian magnitude from the smoothed plane.
      if(level.logImg.range() != innerRange) {
        level.logImg = Array<float, 2>(innerRange);
      }
      const float sigmaSqr = level.sigmaI * level.sigmaI;
      for(int r : innerRange[0]) {
        for(int c : innerRange[1]) {
          const float laplacian = level.smoothed[r + 1][c] + level.smoothed[r - 1][c]
            + level.smoothed[r][c + 1] + level.smoothed[r][c - 1]
            - 4.0f * level.smoothed[r][c];
          level.logImg[r][c] = sigmaSqr * std::abs(laplacian);
        }
      }
    }
  }

  void HarrisAffineDetector::detectHarrisLaplace(std::vector<AffineFeature> &out) const
  {
    // Endpoint levels have no scale neighbour on one side and are conservatively skipped.
    for(unsigned i = 1; i + 1 < mConfig.numScales; ++i) {
      const ScaleLevel &level = mLevels[i];
      float maxResponse = 0;
      for(int r : level.harris.range(0)) {
        for(int c : level.harris.range(1)) {
          maxResponse = std::max(maxResponse, level.harris[r][c]);
        }
      }
      if(maxResponse <= 0) {
        continue;
      }
      const float minResponse = mConfig.threshold * maxResponse;
      const IndexRange<2> peakRange = level.harris.range().shrink(1);
      for(int r : peakRange[0]) {
        for(int c : peakRange[1]) {
          if(level.harris[r][c] < minResponse) {
            continue;
          }
          const Index<2> at(r, c);
          if(!PeakDetect3Plateau(level.harris, at)) {
            continue;
          }
          // Characteristic scale: 3-point extremum of the scale-normalised LoG
          // across neighbouring levels at this position.
          const float logBelow = mLevels[i - 1].logImg[r][c];
          const float logHere = level.logImg[r][c];
          const float logAbove = mLevels[i + 1].logImg[r][c];
          if(logHere < mConfig.logThreshold || logHere <= logBelow || logHere <= logAbove) {
            continue;
          }
          AffineFeature feature;
          feature.position = LocatePeakSubPixel(level.harris, at);
          // Parabolic refinement of the scale in level index, clamped to +-0.5.
          const float denom = 2.0f * (logBelow + logAbove - 2.0f * logHere);
          float delta = 0;
          if(denom != 0) {
            delta = std::clamp((logBelow - logAbove) / denom, -0.5f, 0.5f);
          }
          feature.scale = mConfig.sigma0 * std::pow(mConfig.scaleStep, float(i) + delta);
          feature.response = level.harris[r][c];
          feature.logResponse = logHere;
          out.push_back(feature);
        }
      }
    }
    std::sort(out.begin(), out.end(), [](const AffineFeature &a, const AffineFeature &b) {
      return a.response > b.response;
    });
    if(out.size() > mConfig.maxFeatures) {
      out.resize(mConfig.maxFeatures);
    }
  }

  bool HarrisAffineDetector::adaptShape(AffineFeature &feature, const Array<float, 2> &img)
  {
    // Iterative affine shape adaptation (Mikolajczyk-Schmid): repeatedly warp the
    // region to a normalised frame with the accumulated shape U, measure the
    // second-moment matrix mu there, and fold mu^(-1/2) into U until mu is isotropic.
    const int radius = mConfig.patchRadius;
    const IndexRange<2> patchRange(IndexRange<1>(-radius, radius), IndexRange<1>(-radius, radius));
    if(mPatch.range() != patchRange) {
      mPatch = Array<float, 2>(patchRange);
    }
    // Fixed scales in the normalised frame: the characteristic scale maps to
    // sigmaINorm so the integration window always fits the patch.
    const float sigmaINorm = float(radius) / 4.0f;
    const float sigmaDNorm = mConfig.sigmaDRatio * sigmaINorm;
    const float pixelSpan = feature.scale / sigmaINorm;//!< Image pixels per normalised pixel at U = I.

    Matrix<float, 2, 2> accumulated = Matrix<float, 2, 2>::Identity();
    Point<float, 2> position = feature.position;
    bool converged = false;

    for(unsigned iteration = 0; iteration < mConfig.maxIterations; ++iteration) {
      // Warp the normalised patch: p_img = pixelSpan * U * p_norm + position.
      const Matrix<float, 2, 2> srMatrix = accumulated * pixelSpan;
      const Affine<float, 2> norm2image(srMatrix, position);
      if(!warp<WarpWrapMode::Stop>(mPatch, img, norm2image)) {
        return false;// Region leaves the image.
      }

      structureTensor(mPatchRR, mPatchRC, mPatchCC, mPatch, sigmaDNorm, sigmaINorm, mTensorWork);

      // Re-localise on the nearest local Harris maximum in the normalised frame,
      // then measure the second-moment matrix there (Mikolajczyk's ordering; using
      // the nearest rather than the strongest maximum keeps the point from hopping
      // between attractors on elongated structures).
      harrisResponse(mPatchHarris, mPatchRR, mPatchRC, mPatchCC, mConfig.k);
      const int window = std::min(int(std::ceil(sigmaDNorm)), radius - 2);
      Index<2> nearest(0, 0);
      int bestDistanceSqr = std::numeric_limits<int>::max();
      for(int r = -window; r <= window; ++r) {
        for(int c = -window; c <= window; ++c) {
          const Index<2> at(r, c);
          const int distanceSqr = r * r + c * c;
          if(distanceSqr < bestDistanceSqr && PeakDetect3Plateau(mPatchHarris, at)) {
            bestDistanceSqr = distanceSqr;
            nearest = at;
          }
        }
      }
      if(bestDistanceSqr < std::numeric_limits<int>::max()) {
        const Point<float, 2> normShift = LocatePeakSubPixel(mPatchHarris, nearest);
        position = position + srMatrix * normShift;
      }

      // Second-moment matrix at the (re-localised) point.
      Matrix<float, 2, 2> mu;
      mu << mPatchRR[nearest], mPatchRC[nearest],
        mPatchRC[nearest], mPatchCC[nearest];
      if(!mu.allFinite()) {
        return false;
      }
      Eigen::SelfAdjointEigenSolver<Matrix<float, 2, 2>> solver(mu);
      const float lambdaMin = solver.eigenvalues()[0];
      const float lambdaMax = solver.eigenvalues()[1];
      if(!(lambdaMin > 0) || !std::isfinite(lambdaMax)) {
        return false;// Degenerate local structure (flat or pure edge).
      }
      if(std::sqrt(lambdaMin / lambdaMax) >= mConfig.convergenceRatio) {
        converged = true;
        break;
      }

      // Fold mu^(-1/2) into the accumulated shape, renormalised to det == 1
      // (overall size stays in feature.scale).
      const Matrix<float, 2, 2> eigenvectors = solver.eigenvectors();
      const Matrix<float, 2, 2> muInvSqrt = eigenvectors
        * Eigen::DiagonalMatrix<float, 2>(1.0f / std::sqrt(lambdaMin), 1.0f / std::sqrt(lambdaMax))
        * eigenvectors.transpose();
      accumulated = accumulated * muInvSqrt;
      const float det = accumulated.determinant();
      if(!(det > 0) || !accumulated.allFinite()) {
        return false;
      }
      accumulated /= std::sqrt(det);

      // Divergence test on the accumulated anisotropy.
      Eigen::SelfAdjointEigenSolver<Matrix<float, 2, 2>> shapeSolver(accumulated * accumulated.transpose());
      const float axisRatio = std::sqrt(shapeSolver.eigenvalues()[1] / shapeSolver.eigenvalues()[0]);
      if(axisRatio > mConfig.maxAxisRatio) {
        return false;
      }
    }
    if(!converged) {
      return false;
    }

    // The region ellipse depends only on U * U^T; take the symmetric positive
    // definite factor (polar decomposition) as the final shape.
    Eigen::SelfAdjointEigenSolver<Matrix<float, 2, 2>> finalSolver(accumulated * accumulated.transpose());
    const Matrix<float, 2, 2> eigenvectors = finalSolver.eigenvectors();
    // Eigenvalues of U*U^T are the squared singular values of U; their square roots
    // rebuild |U| as a symmetric matrix with the same ellipse.
    Matrix<float, 2, 2> shape = eigenvectors
      * Eigen::DiagonalMatrix<float, 2>(std::sqrt(finalSolver.eigenvalues()[0]),
                                        std::sqrt(finalSolver.eigenvalues()[1]))
      * eigenvectors.transpose();
    // det(U) == 1 already, but renormalise against accumulated float error.
    const float det = shape.determinant();
    if(!(det > 0) || !shape.allFinite()) {
      return false;
    }
    shape /= std::sqrt(det);
    feature.shape = shape;
    feature.position = position;
    return true;
  }

  void HarrisAffineDetector::deduplicate(std::vector<AffineFeature> &features) const
  {
    std::vector<AffineFeature> accepted;
    accepted.reserve(features.size());
    for(const auto &candidate : features) {
      bool isDuplicate = false;
      for(const auto &keeper : accepted) {
        const float distance = (keeper.position - candidate.position).norm();
        const float scaleRatio = std::max(keeper.scale, candidate.scale) / std::min(keeper.scale, candidate.scale);
        if(distance < std::max(1.5f, 0.5f * std::min(keeper.scale, candidate.scale)) && scaleRatio < mConfig.scaleStep) {
          isDuplicate = true;
          break;
        }
      }
      if(!isDuplicate) {
        accepted.push_back(candidate);
      }
    }
    features = std::move(accepted);
  }

}// namespace Ravl2
