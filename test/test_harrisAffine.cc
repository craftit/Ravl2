//
// Created by charles galambos on 12/06/2026.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Image/Convolve.hh"
#include "Ravl2/Image/CornerDetectorHarris.hh"
#include "Ravl2/Image/HarrisAffineDetector.hh"
#include "Ravl2/Image/DrawPolygon.hh"
#include "Ravl2/Geometry/Polygon.hh"

namespace Ravl2
{
  namespace
  {
    //! Smallest distance from 'point' to any corner in 'corners'.
    float nearestCornerDistance(const std::vector<Corner> &corners, const Point<float, 2> &point)
    {
      float best = std::numeric_limits<float>::max();
      for(const auto &corner : corners) {
        best = std::min(best, (corner.location() - point).norm());
      }
      return best;
    }
  }// namespace

  TEST_CASE("GaussianKernel", "[convolve]")
  {
    SECTION("Normalised and symmetric")
    {
      const float sigma = 1.5f;
      auto kernel = gaussianKernel(sigma);
      CHECK(kernel.size() % 2 == 1);
      float sum = 0;
      for(auto value : kernel)
        sum += value;
      EXPECT_FLOAT_EQ(sum, 1.0f);
      const size_t centre = kernel.size() / 2;
      for(size_t i = 0; i < centre; ++i) {
        EXPECT_FLOAT_EQ(kernel[i], kernel[kernel.size() - 1 - i]);
      }
      // Ratios match the analytic Gaussian.
      const float expected = std::exp(-1.0f / (2.0f * sigma * sigma));
      EXPECT_FLOAT_EQ(kernel[centre + 1] / kernel[centre], expected);
    }
  }

  TEST_CASE("ConvolveSeparable", "[convolve]")
  {
    SECTION("Impulse reproduces kernel outer product")
    {
      Array<float, 2> img(IndexRange<2>({{0, 14}, {0, 14}}), 0.0f);
      img[7][7] = 1.0f;
      auto kernel = gaussianKernel(1.0f);
      const int radius = int(kernel.size()) / 2;
      Array<float, 2> out, tmp;
      convolveSeparable(out, img, std::span<const float>(kernel), std::span<const float>(kernel), tmp);
      CHECK(out.range() == img.range().shrink(radius));
      for(int r : out.range(0)) {
        for(int c : out.range(1)) {
          const int dr = r - 7;
          const int dc = c - 7;
          float expected = 0.0f;
          if(std::abs(dr) <= radius && std::abs(dc) <= radius) {
            expected = kernel[size_t(dr + radius)] * kernel[size_t(dc + radius)];
          }
          CHECK(std::abs(out[r][c] - expected) < 1e-6f);
        }
      }
    }
    SECTION("Output preserves absolute coordinates with non-zero origin")
    {
      Array<float, 2> img(IndexRange<2>({{10, 30}, {-5, 20}}), 1.0f);
      img[20][8] = 2.0f;
      auto kernel = gaussianKernel(1.0f);
      Array<float, 2> out, tmp;
      convolveSeparable(out, img, std::span<const float>(kernel), std::span<const float>(kernel), tmp);
      const int radius = int(kernel.size()) / 2;
      CHECK(out.range() == img.range().shrink(radius));
      // The blurred impulse peak stays at the same absolute position.
      float best = 0;
      Index<2> bestAt {0, 0};
      for(int r : out.range(0)) {
        for(int c : out.range(1)) {
          if(out[r][c] > best) {
            best = out[r][c];
            bestAt = Index<2>(r, c);
          }
        }
      }
      CHECK(bestAt == Index<2>(20, 8));
    }
  }

  TEST_CASE("GaussianBlur", "[convolve]")
  {
    SECTION("Preserves range and constant image")
    {
      Array<float, 2> img(IndexRange<2>({{-4, 20}, {3, 40}}), 0.5f);
      Array<float, 2> out;
      gaussianBlur(out, img, 1.2f);
      CHECK(out.range() == img.range());
      for(int r : out.range(0)) {
        for(int c : out.range(1)) {
          CHECK(std::abs(out[r][c] - 0.5f) < 1e-5f);
        }
      }
    }
    SECTION("Cascade property g(sa) then g(sb) ~ g(sqrt(sa^2+sb^2))")
    {
      Array<float, 2> img(IndexRange<2>({{0, 40}, {0, 40}}), 0.0f);
      img[20][20] = 1.0f;
      const float sigmaA = 1.0f;
      const float sigmaB = 1.5f;
      Array<float, 2> stepA, stepAB, direct;
      gaussianBlur(stepA, img, sigmaA);
      gaussianBlur(stepAB, stepA, sigmaB);
      gaussianBlur(direct, img, std::sqrt(sigmaA * sigmaA + sigmaB * sigmaB));
      CHECK(stepAB.range() == direct.range());
      for(int r : direct.range(0)) {
        for(int c : direct.range(1)) {
          CHECK(std::abs(stepAB[r][c] - direct[r][c]) < 2e-4f);
        }
      }
    }
  }

  TEST_CASE("CornerDetectorHarris", "[harris]")
  {
    SECTION("Axis-aligned rectangle: fires on corners, not edges")
    {
      Array<uint8_t, 2> img(IndexRange<2>({{0, 99}, {0, 99}}), 30);
      const std::vector<Point<float, 2>> trueCorners = {{20, 20}, {20, 80}, {60, 80}, {60, 20}};
      Polygon<float> rect({{20, 20}, {20, 80}, {60, 80}, {60, 20}});
      DrawFilledPolygon(img, uint8_t(220), rect);

      CornerDetectorHarris detector;
      auto corners = detector.apply(img);
      REQUIRE(!corners.empty());

      // Recall: every true corner has a detection nearby. Harris localisation is
      // biased inward by roughly sigmaI * sqrt(2) for step corners, hence 3.5 px.
      for(const auto &trueCorner : trueCorners) {
        CHECK(nearestCornerDistance(corners, trueCorner) < 3.5f);
      }
      // Precision: every detection is near a true corner, none along the edges.
      for(const auto &corner : corners) {
        float best = std::numeric_limits<float>::max();
        for(const auto &trueCorner : trueCorners) {
          best = std::min(best, (corner.location() - trueCorner).norm());
        }
        CHECK(best < 4.0f);
      }
      // Edge midpoints are quiet.
      CHECK(nearestCornerDistance(corners, Point<float, 2>({20, 50})) > 3.0f);
      CHECK(nearestCornerDistance(corners, Point<float, 2>({40, 20})) > 3.0f);
      CHECK(nearestCornerDistance(corners, Point<float, 2>({40, 80})) > 3.0f);
      CHECK(nearestCornerDistance(corners, Point<float, 2>({60, 50})) > 3.0f);
    }
    SECTION("Rotated rectangle: corners still found")
    {
      Array<uint8_t, 2> img(IndexRange<2>({{0, 99}, {0, 99}}), 30);
      const float angle = std::numbers::pi_v<float> / 6.0f;// 30 degrees
      const Point<float, 2> centre({50, 50});
      std::vector<Point<float, 2>> trueCorners;
      Polygon<float> poly;
      for(const Point<float, 2> &base : {Point<float, 2>({-20, -30}), Point<float, 2>({-20, 30}),
                                         Point<float, 2>({20, 30}), Point<float, 2>({20, -30})}) {
        const Point<float, 2> rotated({base[0] * std::cos(angle) - base[1] * std::sin(angle) + centre[0],
                                       base[0] * std::sin(angle) + base[1] * std::cos(angle) + centre[1]});
        trueCorners.push_back(rotated);
        poly.push_back(rotated);
      }
      DrawFilledPolygon(img, uint8_t(220), poly);

      CornerDetectorHarris detector;
      auto corners = detector.apply(img);
      REQUIRE(!corners.empty());
      for(const auto &trueCorner : trueCorners) {
        CHECK(nearestCornerDistance(corners, trueCorner) < 3.5f);
      }
    }
    SECTION("Flat image produces no corners")
    {
      Array<uint8_t, 2> img(IndexRange<2>({{0, 49}, {0, 49}}), 128);
      CornerDetectorHarris detector;
      CHECK(detector.apply(img).empty());
    }
  }

  TEST_CASE("HarrisLaplaceScaleSelection", "[harrisLaplace]")
  {
    // Two filled squares whose sizes differ by 2x: the characteristic scales of the
    // features they generate should also differ by roughly 2x.
    Array<uint8_t, 2> img(IndexRange<2>({{0, 159}, {0, 159}}), 30);
    const Point<float, 2> centreSmall({46, 46});
    const Point<float, 2> centreLarge({102, 102});
    const float sideSmall = 12;
    const float sideLarge = 24;
    DrawFilledPolygon(img, uint8_t(220),
                      Polygon<float>({{40, 40}, {40, 52}, {52, 52}, {52, 40}}));
    DrawFilledPolygon(img, uint8_t(220),
                      Polygon<float>({{90, 90}, {90, 114}, {114, 114}, {114, 90}}));

    HarrisAffineDetector detector;
    auto features = detector.apply(img);
    REQUIRE(!features.empty());

    // Largest characteristic scale among the features each square generates.
    auto maxScaleNear = [&](const Point<float, 2> &centre, float radius) {
      float maxScale = 0;
      for(const auto &feature : features) {
        if((feature.position - centre).norm() <= radius) {
          maxScale = std::max(maxScale, feature.scale);
        }
      }
      return maxScale;
    };
    const float scaleSmall = maxScaleNear(centreSmall, sideSmall);
    const float scaleLarge = maxScaleNear(centreLarge, sideLarge);
    REQUIRE(scaleSmall > 0);
    REQUIRE(scaleLarge > 0);
    const float ratio = scaleLarge / scaleSmall;
    const float step = detector.config().scaleStep;
    CHECK(ratio > 2.0f / step);
    CHECK(ratio < 2.0f * step);
  }

  TEST_CASE("HarrisAffineAdaptation", "[harrisAffine]")
  {
    SECTION("Anisotropic Gaussian blob: recovered shape matches ground truth")
    {
      // Blob with principal std devs (10, 5) rotated 30 degrees.
      const float majorStd = 10.0f;
      const float minorStd = 5.0f;
      const float angle = std::numbers::pi_v<float> / 6.0f;
      const Point<float, 2> centre({64, 64});
      Matrix<float, 2, 2> rotation;
      rotation << std::cos(angle), -std::sin(angle), std::sin(angle), std::cos(angle);
      const Matrix<float, 2, 2> invCovariance = rotation
        * Eigen::DiagonalMatrix<float, 2>(1.0f / (majorStd * majorStd), 1.0f / (minorStd * minorStd))
        * rotation.transpose();

      Array<uint8_t, 2> img(IndexRange<2>({{0, 127}, {0, 127}}), 30);
      for(int r : img.range(0)) {
        for(int c : img.range(1)) {
          const Vector<float, 2> offset = {float(r) - centre[0], float(c) - centre[1]};
          const float quadratic = offset.dot(invCovariance * offset);
          img[r][c] = uint8_t(30.0f + 190.0f * std::exp(-0.5f * quadratic));
        }
      }

      HarrisAffineDetector detector;
      auto features = detector.apply(img);
      REQUIRE(!features.empty());
      const AffineFeature *blob = nullptr;
      float bestDistance = 6.0f;
      for(const auto &feature : features) {
        const float distance = (feature.position - centre).norm();
        if(distance < bestDistance) {
          bestDistance = distance;
          blob = &feature;
        }
      }
      REQUIRE(blob != nullptr);

      Eigen::SelfAdjointEigenSolver<Matrix<float, 2, 2>> solver(blob->shape);
      const float axisRatio = solver.eigenvalues()[1] / solver.eigenvalues()[0];
      CHECK(axisRatio > 1.55f);
      CHECK(axisRatio < 2.6f);
      // Major axis direction (eigenvector of the larger eigenvalue), modulo pi.
      const Vector<float, 2> majorAxis = solver.eigenvectors().col(1);
      const float majorAngle = std::atan2(majorAxis[1], majorAxis[0]);
      float angleError = std::abs(majorAngle - angle);
      angleError = std::min(angleError, std::abs(angleError - std::numbers::pi_v<float>));
      CHECK(angleError < 0.18f);// ~10 degrees
    }

    SECTION("Affine covariance: regions transform with a known affine warp")
    {
      // The same Gaussian blob drawn directly and through a known affine map W
      // (blob covariance Sigma vs W * Sigma * W^T); the detected regions should
      // satisfy A_warped ~ W * A_orig * R with R a rotation.
      const Point<float, 2> centreA({60, 60});
      const Point<float, 2> centreB({64, 64});
      const float angle = std::numbers::pi_v<float> / 6.0f;
      Matrix<float, 2, 2> rotation;
      rotation << std::cos(angle), -std::sin(angle), std::sin(angle), std::cos(angle);
      const Matrix<float, 2, 2> warpMatrix = rotation * Eigen::DiagonalMatrix<float, 2>(1.8f, 0.9f);

      const Matrix<float, 2, 2> covarianceA = Eigen::DiagonalMatrix<float, 2>(8.0f * 8.0f, 6.0f * 6.0f);
      const Matrix<float, 2, 2> covarianceB = warpMatrix * covarianceA * warpMatrix.transpose();

      auto drawBlob = [](Array<uint8_t, 2> &img, const Point<float, 2> &centre, const Matrix<float, 2, 2> &covariance) {
        const Matrix<float, 2, 2> invCovariance = covariance.inverse();
        for(int r : img.range(0)) {
          for(int c : img.range(1)) {
            const Vector<float, 2> offset = {float(r) - centre[0], float(c) - centre[1]};
            const float quadratic = offset.dot(invCovariance * offset);
            img[r][c] = uint8_t(30.0f + 190.0f * std::exp(-0.5f * quadratic));
          }
        }
      };
      Array<uint8_t, 2> imgA(IndexRange<2>({{0, 127}, {0, 127}}), 30);
      Array<uint8_t, 2> imgB(IndexRange<2>({{0, 127}, {0, 127}}), 30);
      drawBlob(imgA, centreA, covarianceA);
      drawBlob(imgB, centreB, covarianceB);

      HarrisAffineDetector detector;
      auto featuresA = detector.apply(imgA);
      auto featuresB = detector.apply(imgB);

      auto findNear = [](const std::vector<AffineFeature> &features, const Point<float, 2> &at) -> const AffineFeature * {
        const AffineFeature *best = nullptr;
        float bestDistance = 6.0f;
        for(const auto &feature : features) {
          const float distance = (feature.position - at).norm();
          if(distance < bestDistance) {
            bestDistance = distance;
            best = &feature;
          }
        }
        return best;
      };
      const AffineFeature *featureA = findNear(featuresA, centreA);
      const AffineFeature *featureB = findNear(featuresB, centreB);
      REQUIRE(featureA != nullptr);
      REQUIRE(featureB != nullptr);

      const Matrix<float, 2, 2> regionA = featureA->norm2img().SRMatrix();
      const Matrix<float, 2, 2> regionB = featureB->norm2img().SRMatrix();
      const Matrix<float, 2, 2> residual = regionB.inverse() * warpMatrix * regionA;
      // residual should be a scaled rotation: residual^T * residual ~ c^2 * I.
      const Matrix<float, 2, 2> gram = residual.transpose() * residual;
      const float meanDiagonal = (gram(0, 0) + gram(1, 1)) / 2.0f;
      CHECK(std::abs(gram(0, 0) - gram(1, 1)) < 0.3f * meanDiagonal);
      CHECK(std::abs(gram(0, 1)) < 0.2f * meanDiagonal);
      // And close to unit scale: the two detections agree on the region size.
      const float scaleFactor = std::sqrt(std::abs(residual.determinant()));
      CHECK(scaleFactor > 0.7f);
      CHECK(scaleFactor < 1.45f);
    }

    SECTION("Flat image: no features, no numerical issues")
    {
      Array<uint8_t, 2> img(IndexRange<2>({{0, 63}, {0, 63}}), 100);
      HarrisAffineDetector detector;
      auto features = detector.apply(img);
      CHECK(features.empty());
    }

    SECTION("extractNormalisedPatch undoes the local affine deformation")
    {
      // Same anisotropic blob as the first section; its normalised patch should be
      // approximately isotropic.
      const float majorStd = 10.0f;
      const float minorStd = 5.0f;
      const float angle = std::numbers::pi_v<float> / 6.0f;
      const Point<float, 2> centre({64, 64});
      Matrix<float, 2, 2> rotation;
      rotation << std::cos(angle), -std::sin(angle), std::sin(angle), std::cos(angle);
      const Matrix<float, 2, 2> invCovariance = rotation
        * Eigen::DiagonalMatrix<float, 2>(1.0f / (majorStd * majorStd), 1.0f / (minorStd * minorStd))
        * rotation.transpose();
      Array<float, 2> img(IndexRange<2>({{0, 127}, {0, 127}}), 0.1f);
      for(int r : img.range(0)) {
        for(int c : img.range(1)) {
          const Vector<float, 2> offset = {float(r) - centre[0], float(c) - centre[1]};
          img[r][c] = 0.1f + 0.75f * std::exp(-0.5f * offset.dot(invCovariance * offset));
        }
      }
      HarrisAffineDetector detector;
      auto features = detector.apply(img);
      const AffineFeature *blob = nullptr;
      float bestDistance = 6.0f;
      for(const auto &feature : features) {
        const float distance = (feature.position - centre).norm();
        if(distance < bestDistance) {
          bestDistance = distance;
          blob = &feature;
        }
      }
      REQUIRE(blob != nullptr);

      Array<float, 2> patch(IndexRange<2>({{-16, 16}, {-16, 16}}));
      REQUIRE(HarrisAffineDetector::extractNormalisedPatch(patch, img, *blob, 2.0f));
      // The structure tensor of the normalised patch is close to isotropic.
      Array<float, 2> rr, rc, cc;
      structureTensor(rr, rc, cc, patch, 2.8f, 4.0f);
      Matrix<float, 2, 2> mu;
      mu << rr[0][0], rc[0][0], rc[0][0], cc[0][0];
      Eigen::SelfAdjointEigenSolver<Matrix<float, 2, 2>> solver(mu);
      REQUIRE(solver.eigenvalues()[0] > 0);
      CHECK(std::sqrt(solver.eigenvalues()[0] / solver.eigenvalues()[1]) > 0.8f);

      // A feature whose region pokes outside the image is rejected.
      AffineFeature nearEdge = *blob;
      nearEdge.position = Point<float, 2>({2, 2});
      CHECK_FALSE(HarrisAffineDetector::extractNormalisedPatch(patch, img, nearEdge, 2.0f));
    }
  }

}// namespace Ravl2
