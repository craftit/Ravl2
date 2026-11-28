//
// OpenCV interop: strides of sub-arrays and 4-channel images.
//

#include <array>
#include <stdexcept>
#include <catch2/catch_test_macros.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/OpenCV/Image.hh"

namespace Ravl2
{
  TEST_CASE("toCvMat keeps the row pitch of a sub-array")
  {
    Array<uint8_t, 2> img({10, 10});
    for(int r = 0; r < 10; ++r)
      for(int c = 0; c < 10; ++c)
        img[r][c] = uint8_t(10 * r + c);
    ArrayView<uint8_t, 2> view(img, IndexRange<2>({{2, 4}, {3, 5}}));
    cv::Mat mat = toCvMat(view);
    REQUIRE(mat.rows == 3);
    REQUIRE(mat.cols == 3);
    CHECK(mat.at<uint8_t>(0, 0) == 23);
    CHECK(mat.at<uint8_t>(1, 0) == 33);// was 26 when the step was ignored
    CHECK(mat.at<uint8_t>(2, 2) == 45);
  }

  TEST_CASE("toCvMat rejects negative strides")
  {
    Array<uint8_t, 2> img({10, 10}, 0);
    // Rows in reverse order.
    std::array<std::ptrdiff_t, 2> strides {-10, 1};
    ArrayView<uint8_t, 2> flipped(&img[9][0], IndexRange<2>({10, 10}), strides);
    CHECK_THROWS_AS(toCvMat(flipped), std::invalid_argument);
  }

  TEST_CASE("toArray reads 4-channel images with their own stride")
  {
    cv::Mat mat(3, 6, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    mat.at<cv::Vec4b>(2, 5) = cv::Vec4b(1, 2, 3, 4);
    auto arr = toArray<PixelBGRA8, 2>(mat);
    CHECK(arr.stride(0) == 6);
    CHECK(arr[2][5] == PixelBGRA8(1, 2, 3, 4));
    CHECK(arr[1][5] == PixelBGRA8(0, 0, 0, 0));
  }
}// namespace Ravl2
