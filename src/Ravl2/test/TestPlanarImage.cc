//
// Created on 08/09/2025.
//

#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Pixel/PixelPlane.hh"

using namespace Ravl2;

TEST_CASE("PlanarImage createPackedPixel test", "[planarimage][pixelplane]")
{
  SECTION("Test RGB planar to packed conversion")
  {
    // Create an RGB planar image with known values
    IndexRange<2> range(0, 10, 0, 10); // 10x10 image
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
    IndexRange<2> range(0, 10, 0, 10); // 10x10 image
    YUV444Image<uint8_t> planarImage(range);

    // Fill with values representing Y=128 (mid-gray), U=128 (no chroma), V=128 (no chroma)
    planarImage.plane<0>().fill(128); // Y = 128
    planarImage.plane<1>().fill(128); // U = 128 (neutral)
    planarImage.plane<2>().fill(128); // V = 128 (neutral)

    // Test createPackedPixel for PixelRGB8
    Index<2> testCoord(5, 5);
    auto rgbPixel = planarImage.createPackedPixel<Pixel, uint8_t, 
                        ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>(testCoord);

    // Since Y=128, U=128, V=128 represents a mid-gray color, RGB should be around 128
    REQUIRE(rgbPixel.get<ImageChannel::Red>() == 128);
    REQUIRE(rgbPixel.get<ImageChannel::Green>() == 128);
    REQUIRE(rgbPixel.get<ImageChannel::Blue>() == 128);
  }

  SECTION("Test with subsampled planar image")
  {
    // Create a YUV 4:2:0 image (with subsampling)
    IndexRange<2> range(0, 10, 0, 10); // 10x10 image
    YUV420Image<uint8_t> planarImage(range);

    // Fill each plane with different values
    planarImage.plane<0>().fill(128); // Y = 128
    planarImage.plane<1>().fill(64);  // U = 64 (blueish)
    planarImage.plane<2>().fill(192); // V = 192 (reddish)

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

    // With Y=128, U=64 (blueish), V=192 (reddish), we expect:
    // - Red component to be high (due to high V)
    // - Blue component to be high (due to low U)
    // - Green component to be lower
    REQUIRE(pixel1.get<ImageChannel::Red>() > 128);
    REQUIRE(pixel1.get<ImageChannel::Blue>() > 128);
    REQUIRE(pixel1.get<ImageChannel::Green>() < 128);
  }

  SECTION("Test with component type conversion")
  {
    // Create an RGB planar image with uint8_t components
    IndexRange<2> range(0, 10, 0, 10);
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
    REQUIRE(floatPixel.get<ImageChannel::Red>() == Approx(100.0f/255.0f).epsilon(0.01f));
    REQUIRE(floatPixel.get<ImageChannel::Green>() == Approx(150.0f/255.0f).epsilon(0.01f));
    REQUIRE(floatPixel.get<ImageChannel::Blue>() == Approx(200.0f/255.0f).epsilon(0.01f));
  }

  SECTION("Test with missing channels")
  {
    // Create an RGB planar image (no alpha)
    IndexRange<2> range(0, 10, 0, 10);
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
