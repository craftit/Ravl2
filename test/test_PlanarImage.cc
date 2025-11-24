//
// Created on 08/09/2025.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>
#include "Ravl2/Pixel/PixelPlane.hh"
#include "Ravl2/Pixel/Colour.hh"

namespace Ravl2
{
  using Catch::Approx;

  TEST_CASE("PlanarImage createPackedPixel test", "[planarimage][pixelplane]")
  {
    SECTION("Test RGB planar to packed conversion")
    {
      // Create an RGB planar image with known values
      IndexRange<2> range({10,10}); // 10x10 image
      RGBPlanarImage<uint8_t> planarImage(range);

      // Fill each plane with different values
      planarImage.plane<0>().fill(100); // Red = 100
      planarImage.plane<1>().fill(150); // Green = 150
      planarImage.plane<2>().fill(200); // Blue = 200

      // Test createPackedPixel for PixelRGB8
      Index<2> testCoord(5, 5);
      auto rgbPixel = planarImage.createPackedPixel<Pixel, uint8_t,
                          ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>(testCoord);

      REQUIRE(rgbPixel.get<ImageChannel::Red>() == 100);
      REQUIRE(rgbPixel.get<ImageChannel::Green>() == 150);
      REQUIRE(rgbPixel.get<ImageChannel::Blue>() == 200);
    }

    SECTION("Test YUV planar to RGB packed conversion")
    {
      // Create a YUV planar image with known values
      IndexRange<2> range({10, 10}); // 10x10 image
      YUV444Image<uint8_t> planarImage(range);

      // Fill with values representing Y=128 (mid-gray), U=128 (no chroma), V=128 (no chroma)
      planarImage.plane<0>().fill(128); // Y = 128
      planarImage.plane<1>().fill(128); // U = 128 (neutral)
      planarImage.plane<2>().fill(128); // V = 128 (neutral)

      PixelYUV8 packedYUV(128,128,128);
      PixelRGB8 packedRGB(0,0,0);
      assign(packedRGB, packedYUV);

      // Test createPackedPixel for PixelRGB8
      Index<2> testCoord(5, 5);
      auto rgbPixel = planarImage.createPackedPixel<Pixel, uint8_t,
                          ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>(testCoord);


      auto atPackedYUV = planarImage.at(testCoord);

      REQUIRE(atPackedYUV.get<ImageChannel::Luminance>() == packedYUV.get<ImageChannel::Luminance>());
      REQUIRE(atPackedYUV.get<ImageChannel::ChrominanceU>() == packedYUV.get<ImageChannel::ChrominanceU>());
      REQUIRE(atPackedYUV.get<ImageChannel::ChrominanceV>() == packedYUV.get<ImageChannel::ChrominanceV>());


      SPDLOG_INFO("Converted RGB Pixel from planar YUV: {}, expected {} ", rgbPixel,packedRGB);

      // The conversion should match the plain packed one.
      REQUIRE(rgbPixel.get<ImageChannel::Red>() == packedRGB.get<ImageChannel::Red>());
      REQUIRE(rgbPixel.get<ImageChannel::Green>() == packedRGB.get<ImageChannel::Green>());
      REQUIRE(rgbPixel.get<ImageChannel::Blue>() == packedRGB.get<ImageChannel::Blue>());
    }

    SECTION("Test with subsampled planar image")
    {
      // Create a YUV 4:2:0 image (with subsampling)
      IndexRange<2> range({10, 10}); // 10x10 image
      YUV420Image<uint8_t> planarImage(range);

      // Fill each plane with different values
      planarImage.plane<0>().fill(128); // Y = 128
      planarImage.plane<1>().fill(64);  // U = 64
      planarImage.plane<2>().fill(192); // V = 192

      // Test at both even and odd coordinates to check subsampling handling
      // In 4:2:0, U and V planes are half resolution in both dimensions

      // Test at (4,4) - should access U and V planes at (2,2)
      Index<2> testCoord1(4, 4);
      auto pixel1 = planarImage.createPackedPixel<Pixel, uint8_t,
                         ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>(testCoord1);

      // Test at (5,5) - should still access U and V planes at (2,2)
      // since they're subsampled
      Index<2> testCoord2(5, 5);
      auto pixel2 = planarImage.createPackedPixel<Pixel, uint8_t,
                         ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>(testCoord2);

      // Both pixels should have the same color since they map to the same chroma samples
      REQUIRE(pixel1.get<ImageChannel::Red>() == pixel2.get<ImageChannel::Red>());
      REQUIRE(pixel1.get<ImageChannel::Green>() == pixel2.get<ImageChannel::Green>());
      REQUIRE(pixel1.get<ImageChannel::Blue>() == pixel2.get<ImageChannel::Blue>());

      SPDLOG_INFO("Got pixel {}, expected {} ", pixel1, pixel2);
      // Do a sanity check
      REQUIRE(pixel1.get<ImageChannel::Red>() > 190);
      REQUIRE(pixel1.get<ImageChannel::Green>() < 128);
      REQUIRE(pixel1.get<ImageChannel::Blue>() < 30);
    }

    SECTION("Test with component type conversion")
    {
      // Create an RGB planar image with uint8_t components
      IndexRange<2> range({ 10, 10} );
      RGBPlanarImage<uint8_t> planarImage(range);

      // Fill each plane with different values
      planarImage.plane<0>().fill(100); // Red = 100
      planarImage.plane<1>().fill(150); // Green = 150
      planarImage.plane<2>().fill(200); // Blue = 200

      // Create a float pixel from the uint8_t planar image
      Index<2> testCoord(5, 5);
      auto floatPixel = planarImage.createPackedPixel<Pixel, float,
                            ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>(testCoord);

      // Check that values were correctly converted to float (0-1 range)
      SPDLOG_INFO("Pixel {} ", floatPixel);

      REQUIRE(floatPixel.get<ImageChannel::Red>() == Approx(100.0f/255.0f).epsilon(0.01f));
      REQUIRE(floatPixel.get<ImageChannel::Green>() == Approx(150.0f/255.0f).epsilon(0.01f));
      REQUIRE(floatPixel.get<ImageChannel::Blue>() == Approx(200.0f/255.0f).epsilon(0.01f));
    }

    SECTION("Test with missing channels")
    {
      // Create an RGB planar image (no alpha)
      IndexRange<2> range({10, 10});
      RGBPlanarImage<uint8_t> planarImage(range);

      // Fill each plane with different values
      planarImage.plane<0>().fill(100); // Red = 100
      planarImage.plane<1>().fill(150); // Green = 150
      planarImage.plane<2>().fill(200); // Blue = 200

      // Try to create an RGBA pixel (with alpha)
      Index<2> testCoord(5, 5);
      auto rgbaPixel = planarImage.createPackedPixel<Pixel, uint8_t,
                           ImageChannel::Red, ImageChannel::Green,
                           ImageChannel::Blue, ImageChannel::Alpha>(testCoord);

      // RGB channels should match the planes
      REQUIRE(rgbaPixel.get<ImageChannel::Red>() == 100);
      REQUIRE(rgbaPixel.get<ImageChannel::Green>() == 150);
      REQUIRE(rgbaPixel.get<ImageChannel::Blue>() == 200);

      // Alpha should be default value (255 for uint8_t)
      REQUIRE(rgbaPixel.get<ImageChannel::Alpha>() == 255);
    }
  }
}
