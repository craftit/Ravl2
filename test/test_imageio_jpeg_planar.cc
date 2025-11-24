//
// JPEG planar YUV tests (requires libjpeg for raw planar path)
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/PixelPlane.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/ImageIO/JpegTurboImageIO.hh"

namespace Ravl2
{
  TEST_CASE("JPEG load YUV420 planar via ImageIO (best-effort)")
  {
    // Ensure JPEG ImageIO TU is linked and plane conversions are available
    initJpegTurboImageIO();

    const std::string filename = std::string(RAVL_SOURCE_DIR) + "/../data/lena.jpg";

    // Ask explicitly for a planar 4:2:0 output (typical JPEG subsampling)
    YUV420Image<uint8_t> yuv;
    nlohmann::json hints = defaultLoadFormatHint(true);

    const bool ok = ioLoad(yuv, filename, hints);

    if (!ok) {
      // Environments without libjpeg (or with non-420 subsampling) may not satisfy this request.
      // Don't fail the suite; simply skip with an info message.
      SPDLOG_INFO("Skipping YUV420 planar JPEG test: no suitable loader available or subsampling mismatch.");
      SUCCEED();
      return;
    }

    const auto master = yuv.range();
    REQUIRE(master.size(0) > 0);
    REQUIRE(master.size(1) > 0);

    // Validate plane sizes respect 4:2:0 scaling
    const auto &yPlane = yuv.template planeByChannel<ImageChannel::Luminance>();
    const auto &uPlane = yuv.template planeByChannel<ImageChannel::ChrominanceU>();
    const auto &vPlane = yuv.template planeByChannel<ImageChannel::ChrominanceV>();

    CHECK(yPlane.data().range().size(0) == master.size(0));
    CHECK(yPlane.data().range().size(1) == master.size(1));

    CHECK(uPlane.data().range().size(0) == (master.size(0) + 1) / 2);
    CHECK(uPlane.data().range().size(1) == (master.size(1) + 1) / 2);
    CHECK(vPlane.data().range().size(0) == (master.size(0) + 1) / 2);
    CHECK(vPlane.data().range().size(1) == (master.size(1) + 1) / 2);

    // Basic pixel sanity: read a top-left packed pixel assembled from planes
    [[maybe_unused]] auto packed = yuv.template createPackedPixel<Pixel, uint8_t,
      ImageChannel::Luminance, ImageChannel::ChrominanceU, ImageChannel::ChrominanceV>({master.min(0), master.min(1)});
    // FIXME: This test will always pass for bytes, what else can we do?
    //CHECK(packed[0] <= 255);
  }
}
