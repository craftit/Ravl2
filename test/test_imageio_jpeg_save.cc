//
// JPEG ImageIO save tests (JPEGTurbo + fallback)
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/ImageIO/JpegTurboImageIO.hh"

namespace Ravl2
{
  TEST_CASE("JPEG save and load RGB8 via ImageIO")
  {
    initJpegTurboImageIO();

    // Make a simple 32x24 gradient image
    Array<PixelRGB8, 2> img({24, 32});
    for (auto it = img.begin(); it != img.end(); ++it) {
      const auto idx = it.index();
      const uint8_t r = static_cast<uint8_t>((idx[1] * 255) / (img.range().size(1) - 1));
      const uint8_t g = static_cast<uint8_t>((idx[0] * 255) / (img.range().size(0) - 1));
      const uint8_t b = static_cast<uint8_t>((idx[0] + idx[1]) % 256);
      *it = PixelRGB8{r, g, b};
    }

    const std::string filename = std::string("tmp_jpeg_rgb8.jpg");

    nlohmann::json hints;
    hints["verbose"] = true;
    hints["jpegQuality"] = 85;

    REQUIRE(ioSave(filename, img, hints));

    Array<PixelRGB8, 2> loaded;
    REQUIRE(ioLoad(loaded, filename, defaultLoadFormatHint(true)));

    CHECK(loaded.range().size(0) == img.range().size(0));
    CHECK(loaded.range().size(1) == img.range().size(1));

    // Basic sanity on pixel values
    [[maybe_unused]] auto p = loaded[{loaded.range().min(0), loaded.range().min(1)}];
    // FIXME: This test will always pass for bytes, what else can we do?
    // CHECK(p[0] <= 255);
    // CHECK(p[1] <= 255);
    // CHECK(p[2] <= 255);

    // Cleanup
    std::remove(filename.c_str());
  }

  TEST_CASE("JPEG save and load Gray8 via ImageIO")
  {
    initJpegTurboImageIO();

    Array<uint8_t, 2> img({20, 30});
    for (auto it = img.begin(); it != img.end(); ++it) {
      const auto idx = it.index();
      *it = static_cast<uint8_t>((idx[0] * 7 + idx[1] * 13) & 0xFF);
    }

    const std::string filename = std::string("tmp_jpeg_gray8.jpg");

    nlohmann::json hints;
    hints["verbose"] = true;
    hints["jpegQuality"] = 80;

    REQUIRE(ioSave(filename, img, hints));

    Array<uint8_t, 2> loaded;
    REQUIRE(ioLoad(loaded, filename, defaultLoadFormatHint(true)));

    CHECK(loaded.range().size(0) == img.range().size(0));
    CHECK(loaded.range().size(1) == img.range().size(1));

    auto v = loaded[{loaded.range().min(0), loaded.range().min(1)}];
    CHECK(v <= 255);

    std::remove(filename.c_str());
  }
}
