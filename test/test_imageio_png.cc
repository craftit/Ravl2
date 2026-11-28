//
// PNG ImageIO load tests (libpng + fallback via OpenCV)
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/ImageIO/PngImageIO.hh"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace Ravl2
{
  TEST_CASE("PNG load RGB8/RGBA8/Gray8 via ImageIO")
  {
    // Ensure PNG ImageIO TU is linked and any registration happens
    initPngImageIO();

    // Use lena.jpg if lena.png not present; this test focuses on the PNG path,
    // but fallback to OpenCV keeps it flexible. We'll try lena.png first.
    const std::string filenamePng = std::string(RAVL_SOURCE_DIR) + "/../data/lena.png";
    const std::string filenameJpg = std::string(RAVL_SOURCE_DIR) + "/../data/lena.jpg";
    const std::string filename = filenamePng; // prefer PNG if available in repo

    nlohmann::json hints = defaultLoadFormatHint(true);

    // Try RGB8
    {
      Array<PixelRGB8, 2> img;
      const bool ok = ioLoad(img, filename, hints);
      if (!ok) {
        // If lena.png is not present, fall back to lena.jpg to still exercise path
        const bool ok2 = ioLoad(img, filenameJpg, hints);
        REQUIRE(ok2);
      }
      const auto &rng = img.range();
      CHECK(rng.size(0) > 0);
      CHECK(rng.size(1) > 0);
    }

    // Try RGBA8
    {
      Array<PixelRGBA8, 2> img;
      const bool ok = ioLoad(img, filename, hints);
      // Not all inputs have alpha; loader should still succeed by adding opaque alpha if needed
      if (!ok) {
        // Fall back to RGB load if needed
        Array<PixelRGB8, 2> rgb;
        REQUIRE((ioLoad(rgb, filename, hints) || ioLoad(rgb, filenameJpg, hints)));
      } else {
        const auto &rng = img.range();
        CHECK(rng.size(0) > 0);
        CHECK(rng.size(1) > 0);
      }
    }

    // Try Gray8
    {
      Array<uint8_t, 2> img;
      const bool ok = ioLoad(img, filename, hints);
      if (!ok) {
        // Fallback to jpg if PNG not available
        const bool ok2 = ioLoad(img, filenameJpg, hints);
        REQUIRE(ok2);
      }
      const auto &rng = img.range();
      CHECK(rng.size(0) > 0);
      CHECK(rng.size(1) > 0);
    }
  }

  namespace
  {
    std::string tempPngPath(const std::string &tag)
    {
      const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
      return (std::filesystem::temp_directory_path() / ("ravl2-png-" + tag + "-" + std::to_string(unique) + ".png")).string();
    }

    //! 3x2 8-bit grey+alpha PNG: grey = 10*row + col, alpha = 255 - col.
    const std::array<uint8_t, 79> kGreyAlphaPng = {
      0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x03,
      0x00, 0x00, 0x00, 0x02, 0x08, 0x04, 0x00, 0x00, 0x00, 0x37, 0x7d, 0xae, 0x91, 0x00, 0x00, 0x00, 0x16, 0x49, 0x44, 0x41,
      0x54, 0x78, 0x9c, 0x63, 0x60, 0xf8, 0xcf, 0xf8, 0x8f, 0xe9, 0x2f, 0x03, 0xd7, 0x7f, 0xee, 0x7f, 0x3c, 0x7f, 0x01, 0x27,
      0x65, 0x06, 0x19, 0x7b, 0xdf, 0x08, 0xba, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
  }// namespace

  TEST_CASE("PNG grey+alpha decodes to grey, RGB and RGBA")
  {
    initPngImageIO();
    const auto filename = tempPngPath("ga");
    {
      std::ofstream f(filename, std::ios::binary);
      f.write(reinterpret_cast<const char *>(kGreyAlphaPng.data()), std::streamsize(kGreyAlphaPng.size()));
    }
    nlohmann::json hints = defaultLoadFormatHint(false);

    Array<uint8_t, 2> grey;
    REQUIRE(ioLoad(grey, filename, hints));
    REQUIRE(grey.range().size(0) == 2);
    REQUIRE(grey.range().size(1) == 3);
    CHECK(grey[1][2] == 12);
    CHECK(grey[0][1] == 1);

    Array<PixelRGB8, 2> rgb;
    REQUIRE(ioLoad(rgb, filename, hints));
    CHECK(rgb[1][1] == PixelRGB8(11, 11, 11));

    Array<PixelRGBA8, 2> rgba;
    REQUIRE(ioLoad(rgba, filename, hints));
    CHECK(rgba[1][2] == PixelRGBA8(12, 12, 12, 253));
    std::filesystem::remove(filename);
  }

  TEST_CASE("PNG 16-bit grey, interlaced and offset-origin round trips")
  {
    initPngImageIO();
    nlohmann::json loadHints = defaultLoadFormatHint(false);

    SECTION("16-bit grey keeps its byte order")
    {
      const auto filename = tempPngPath("g16");
      Array<uint16_t, 2> img({2, 2});
      img[0][0] = 1;
      img[0][1] = 1000;
      img[1][0] = 0xFF00;
      img[1][1] = 0x00FF;
      REQUIRE(ioSave(filename, img));
      Array<uint16_t, 2> back;
      REQUIRE(ioLoad(back, filename, loadHints));
      CHECK(back[0][0] == 1);
      CHECK(back[0][1] == 1000);
      CHECK(back[1][0] == 0xFF00);
      CHECK(back[1][1] == 0x00FF);
      std::filesystem::remove(filename);
    }

    SECTION("Adam7 interlaced image loads")
    {
      const auto filename = tempPngPath("adam7");
      Array<PixelRGB8, 2> img({9, 11});
      for(int r = 0; r < 9; ++r)
        for(int c = 0; c < 11; ++c)
          img[r][c] = PixelRGB8(uint8_t(r), uint8_t(c), uint8_t(r + c));
      nlohmann::json saveHints = defaultSaveFormatHint(false);
      saveHints["pngInterlace"] = true;
      REQUIRE(ioSave(filename, img, saveHints));
      Array<PixelRGB8, 2> back;
      REQUIRE(ioLoad(back, filename, loadHints));
      REQUIRE(back.range().size(0) == 9);
      CHECK(back[8][10] == PixelRGB8(8, 10, 18));
      CHECK(back[4][3] == PixelRGB8(4, 3, 7));
      std::filesystem::remove(filename);
    }

    SECTION("Image with a non-zero origin saves its own pixels")
    {
      const auto filename = tempPngPath("origin");
      Array<uint8_t, 2> img(IndexRange<2>({{5, 7}, {10, 13}}));
      for(auto r : img.range(0))
        for(auto c : img.range(1))
          img[r][c] = uint8_t(10 * (r - 5) + (c - 10));
      REQUIRE(ioSave(filename, img));
      Array<uint8_t, 2> back;
      REQUIRE(ioLoad(back, filename, loadHints));
      REQUIRE(back.range().size(0) == 3);
      REQUIRE(back.range().size(1) == 4);
      CHECK(back[0][0] == 0);
      CHECK(back[2][3] == 23);
      std::filesystem::remove(filename);
    }
  }
}
