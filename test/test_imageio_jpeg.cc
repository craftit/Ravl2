//
// JPEG ImageIO tests (JPEGTurbo + fallback)
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/ImageIO/JpegTurboImageIO.hh"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace Ravl2
{
  TEST_CASE("JPEG load RGB8 via ImageIO")
  {
    // Ensure JPEG ImageIO TU is linked and any registration happens
    initJpegTurboImageIO();

    // Build absolute path to sample image in repo data directory
    const std::string filename = std::string(RAVL_SOURCE_DIR) + "/../data/lena.jpg";
    SPDLOG_INFO("Opening '{}' ",filename);

    // Quiet hints; loader(s) may log INFO on verbose
    nlohmann::json hints = defaultLoadFormatHint(true);

    Array<PixelRGB8, 2> img;
    const bool ok = ioLoad(img, filename, hints);
    REQUIRE(ok);

    // Basic sanity checks
    const auto &rng = img.range();
    CHECK(rng.size(0) > 0);
    CHECK(rng.size(1) > 0);

    // Spot-check a pixel is within 0..255 range and structure looks sane
    [[maybe_unused]] auto p = img[{rng.min(0), rng.min(1)}];
    // FIXME: This test will always pass for bytes, what else can we do?
    // CHECK(p[0] <= 255);
    // CHECK(p[1] <= 255);
    // CHECK(p[2] <= 255);
  }

  TEST_CASE("Corrupt JPEG fails to load without ending the process")
  {
    initJpegTurboImageIO();
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto filename = (std::filesystem::temp_directory_path() / ("ravl2-corrupt-" + std::to_string(unique) + ".jpg")).string();
    {
      // JPEG start-of-image marker followed by garbage: libjpeg's default handler would call exit().
      std::ofstream f(filename, std::ios::binary);
      const char soi[] = {char(0xFF), char(0xD8)};
      f.write(soi, 2);
      const std::string junk(200, char(0x55));
      f.write(junk.data(), std::streamsize(junk.size()));
    }
    Array<PixelRGB8, 2> img;
    bool ok = true;
    CHECK_NOTHROW(ok = ioLoad(img, filename, defaultLoadFormatHint(false)));
    CHECK_FALSE(ok);
    std::filesystem::remove(filename);
  }
}
