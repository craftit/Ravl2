//
// JPEG ImageIO tests (JPEGTurbo + fallback)
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/ImageIO/JpegTurboImageIO.hh"

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
    auto p = img[{rng.min(0), rng.min(1)}];
    CHECK(p[0] <= 255);
    CHECK(p[1] <= 255);
    CHECK(p[2] <= 255);
  }
}
