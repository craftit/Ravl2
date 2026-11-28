//
// Qt interop: QImage <-> Array orientation, row pitch and lifetime.
//

#include <catch2/catch_test_macros.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/Qt/Image.hh"

namespace Ravl2
{
  TEST_CASE("toQImage uses rows as height and columns as width")
  {
    // 3 rows x 8 columns; 8 bytes per row so it can be shared without copying.
    Array<uint8_t, 2> img({3, 8});
    for(int r = 0; r < 3; ++r)
      for(int c = 0; c < 8; ++c)
        img[r][c] = uint8_t(10 * r + c);
    QImage q = toQImage(img);
    CHECK(q.width() == 8);
    CHECK(q.height() == 3);
    CHECK(q.constScanLine(2)[7] == 27);// row 2, column 7

    // Misaligned rows (5 bytes) are copied, including the last row.
    Array<uint8_t, 2> odd({3, 5});
    for(int r = 0; r < 3; ++r)
      for(int c = 0; c < 5; ++c)
        odd[r][c] = uint8_t(10 * r + c);
    QImage q2 = toQImage(odd);
    CHECK(q2.width() == 5);
    CHECK(q2.height() == 3);
    CHECK(q2.constScanLine(2)[4] == 24);
  }

  TEST_CASE("Shared QImage keeps the Array data alive")
  {
    QImage q;
    {
      Array<uint8_t, 2> img({2, 4}, 42);
      q = toQImage(img);
    }
    CHECK(q.constScanLine(1)[3] == 42);
  }

  TEST_CASE("toArray reads a QImage the right way round")
  {
    QImage q(8, 3, QImage::Format_Grayscale8);
    q.fill(0);
    q.scanLine(2)[7] = 99;
    auto shared = toArray<uint8_t>(q);
    REQUIRE(shared.range().size(0) == 3);
    REQUIRE(shared.range().size(1) == 8);
    CHECK(shared[2][7] == 99);

    auto copied = toArray<uint8_t, CopyModeT::Always>(q);
    CHECK(copied[2][7] == 99);

    // A different format is converted.
    QImage rgb(4, 2, QImage::Format_RGB888);
    rgb.fill(QColor(10, 20, 30));
    auto rgbArray = toArray<PixelRGB8>(rgb.convertToFormat(QImage::Format_RGB32));
    CHECK(rgbArray[1][3] == PixelRGB8(10, 20, 30));
  }
}// namespace Ravl2
