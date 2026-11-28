//
// Templates that didn't compile when instantiated (review GAP-2). Each section uses one of them and checks the
// result, so they stay compiling and correct.
//

#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <tuple>
#include <vector>

#include "Ravl2/Array.hh"
#include "Ravl2/ArrayIterZip.hh"
#include "Ravl2/Types.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Math/FastFourierTransform.hh"
#include "Ravl2/Math/FastFourierTransformMath.hh"
#include "Ravl2/Math/Sums1d2.hh"
#include "Ravl2/3D/PinholeCamera0.hh"
#include "Ravl2/3D/PinholeCameraArray.hh"
#include "Ravl2/Image/ImagePyramid.hh"
#include "Ravl2/Image/Segmentation/SegmentExtrema.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Conic2.hh"
#include "Ravl2/Geometry/Ellipse.hh"
#include "Ravl2/Geometry/Isometry3.hh"
#include "Ravl2/Geometry/Line2ABC.hh"
#include "Ravl2/Geometry/LinePP.hh"
#include "Ravl2/Geometry/LinePV.hh"
#include "Ravl2/Geometry/Moments2.hh"
#include "Ravl2/Geometry/NSphere.hh"
#include "Ravl2/Geometry/Plane3PVV.hh"
#include "Ravl2/Geometry/PointSet.hh"
#include "Ravl2/Geometry/Projection.hh"
#include "Ravl2/Geometry/Quad.hh"
#include "Ravl2/Geometry/Quaternion.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/ScaleTranslate.hh"
#include "Ravl2/Geometry/Translate.hh"
#include "Ravl2/Geometry/VectorOffset.hh"

namespace Ravl2
{
  namespace
  {
    //! Write, read back, write again: both texts must match.
    template <typename T>
    void checkStreamRoundTrip(const T &value)
    {
      std::ostringstream first;
      first << value;
      std::istringstream in(first.str());
      T read {};
      in >> read;
      CHECK_FALSE(in.fail());
      std::ostringstream second;
      second << read;
      CHECK(first.str() == second.str());
    }

    //! Write only (types with no operator>>): must produce some text.
    template <typename T>
    void checkStreamOut(const T &value)
    {
      std::ostringstream out;
      out << value;
      CHECK_FALSE(out.str().empty());
    }
  }// namespace

  TEST_CASE("forEachIndexFirst passes the index and the elements")
  {
    Array<int, 2> a({2, 3}, 1);
    Array<int, 2> b({2, 3}, 0);
    forEachIndexFirst<2>([](const Index<2> &at, int &x, int &y) { y = x + at[0] * 10 + at[1]; }, a, b);
    CHECK(b[1][2] == 13);
    CHECK(b[0][0] == 1);
  }

  TEST_CASE("span of a const vector")
  {
    const VectorT<double> v = VectorT<double>::Ones(4);
    std::span<const double> s = span(v);
    CHECK(s.size() == 4);
    CHECK(s[3] == 1.0);
  }

  TEST_CASE("FFT of double data")
  {
    for(size_t n : {4u, 5u, 6u, 8u, 12u}) {
      std::vector<double> data;
      std::vector<float> dataF;
      for(size_t i = 1; i <= n; ++i) {
        data.push_back(double(i));
        dataF.push_back(float(i));
      }
      std::vector<std::complex<double>> result(n);
      std::vector<std::complex<float>> resultF(n);
      fastFourierTransform(std::span(result), std::span<const double>(data));
      fastFourierTransform(std::span(resultF), std::span<const float>(dataF));
      // The forward transform is scaled by 1/n, so element 0 is the mean.
      CHECK(std::abs(result[0].real() - double(n + 1) / 2) < 1e-9);
      for(size_t k = 0; k < n; ++k)
        CHECK(std::abs(result[k] - std::complex<double>(resultF[k])) < 1e-5);
    }
  }

  TEST_CASE("Radix-2 fft2 of double data")
  {
    // A unit impulse transforms to all ones, either way round.
    std::vector<std::complex<double>> data(8, 0.0);
    data[0] = 1.0;
    fft2(data.data(), 3, 'd');
    for(const auto &v : data)
      CHECK(std::abs(std::abs(v) - std::abs(data[0])) < 1e-12);
    CHECK(std::abs(data[0]) > 0);
  }

  TEST_CASE("Sums1d2C streams")
  {
    Sums1d2C<double> sums;
    sums += 1.0;
    sums += 3.0;
    checkStreamRoundTrip(sums);
  }

  TEST_CASE("ArrayView of a const vector, and PixelY8 from uint8_t")
  {
    const std::vector<int> vec {4, 5, 6};
    ArrayView<const int, 1> view(vec);
    CHECK(view.range().size() == 3);
    CHECK(view[2] == 6);
    PixelY8 pixel(uint8_t(7));
    CHECK(pixel.get<ImageChannel::Luminance>() == 7);
  }

  TEST_CASE("PinholeCameraArray triangulates")
  {
    using CameraT = PinholeCamera0<double>;
    const IndexRange<2> frame({480, 640});
    // Three cameras on the x axis, all looking along z.
    std::vector<CameraT> cameras;
    for(double x : {-1.0, 0.0, 1.0}) {
      Matrix<double, 3, 3> R = Matrix<double, 3, 3>::Identity();
      Vector<double, 3> t(-x, 0, 0);
      cameras.emplace_back(240.0, 320.0, 500.0, 500.0, R, t, frame);
    }
    PinholeCameraArray<CameraT, double> array;
    array.Array() = cameras;
    const Vector<double, 3> truth(0.2, -0.3, 5.0);
    std::vector<std::tuple<unsigned, Vector<double, 2>>> points;
    for(unsigned i = 0; i < cameras.size(); ++i) {
      Vector<double, 2> z;
      cameras[i].project(z, truth);
      points.emplace_back(i, z);
    }
    Vector<double, 3> found;
    // Two views (midpoint) and three (refined).
    REQUIRE(array.Triangulate(found, {points[0], points[1]}));
    CHECK((found - truth).norm() < 1e-6);
    REQUIRE(array.Triangulate(found, points));
    CHECK((found - truth).norm() < 1e-6);
    CHECK_FALSE(array.Triangulate(found, {points[0]}));
    CHECK_FALSE(array.Triangulate(found, {points[0], {7u, Vector<double, 2>(0, 0)}}));
  }

  TEST_CASE("ImagePyramid finds the closest level")
  {
    Array<float, 2> img({64, 64}, 1.0f);
    auto pyramid = buildImagePyramid(img, 4, Vector<float, 2>(2.0f, 2.0f));
    REQUIRE(pyramid.numLevels() >= 3);
    const ScaleTranslate<float, 2> quarter(Vector<float, 2>(0.26f, 0.24f), Vector<float, 2>(0, 0));
    const auto &level = pyramid.findLevel(quarter);
    CHECK(std::abs(level.transformTo().scaleVector()[0] - 0.25f) < 0.02f);
  }

  TEST_CASE("SegmentExtrema on a region of interest")
  {
    // A dark square on a bright background, with the ROI around the square.
    Array<uint8_t, 2> img({40, 40}, 200);
    for(int r = 10; r < 20; ++r)
      for(int c = 10; c < 20; ++c)
        img[r][c] = 20;
    SegmentExtrema<uint8_t> segment(5);
    IndexRangeSet<2> roi;
    roi.push_back(IndexRange<2>({{5, 25}, {5, 25}}));
    auto boundaries = segment.apply(img, roi);
    CHECK_FALSE(boundaries.empty());
    auto masks = segment.applyMask(img, roi);
    CHECK_FALSE(masks.empty());
  }

  TEST_CASE("Geometry types stream")
  {
    checkStreamRoundTrip(Line2ABC<float>(1, 2, 3));
    checkStreamRoundTrip(LinePP<float, 2>(toPoint<float>(1, 2), toPoint<float>(3, 4)));
    checkStreamRoundTrip(LinePV<float, 2>(toPoint<float>(1, 2), toVector<float>(3, 4)));
    checkStreamRoundTrip(Plane3PVV<float>(toPoint<float>(1, 2, 3), toVector<float>(1, 0, 0), toVector<float>(0, 1, 0)));
    checkStreamRoundTrip(VectorOffset<float, 3>(toVector<float>(0, 0, 1), 2.0f));
    checkStreamRoundTrip(Conic2<float>(1, 0, 1, 0, 0, -1));
    checkStreamRoundTrip(Ellipse<float>(toPoint<float>(5, 6), 3.0f, 2.0f, 0.5f));
    checkStreamRoundTrip(Projection<float, 2>());
    Moments2<double> moments;
    moments.addPixel(Index<2>(1, 2));
    moments.addPixel(Index<2>(3, 5));
    checkStreamRoundTrip(moments);

    checkStreamOut(Affine<float, 2>());
    checkStreamOut(Isometry3<float>());
    checkStreamOut(NSphere<float, 2>(toPoint<float>(1, 2), 3.0f));
    checkStreamOut(PointSet<float, 2>({toPoint<float>(1, 2)}));
    checkStreamOut(Quad<float, 2>());
    checkStreamOut(Quaternion<float>());
    checkStreamOut(Range<float, 1>(0, 1));
    checkStreamOut(Range<float, 2>(toPoint<float>(0, 0), toPoint<float>(1, 1)));
    checkStreamOut(ScaleTranslate<float, 2>());
    checkStreamOut(Translate<float, 2>());
  }
}// namespace Ravl2
