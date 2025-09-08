//
// Created on 08/09/2025.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"
#include "Ravl2/Pixel/PixelPlane.hh"

namespace Ravl2
{

  TEST_CASE("PixelPlane")
  {
    SECTION("PlaneScale Basic Functionality")
    {
      // Test the basic scaling functionality

      // Test planeToMaster and masterToPlane conversions
      {
        // Single coordinate test
        Index<2> planeCoord(2, 3);

        // No scaling (1:1)
        {
          auto masterCoord = PlaneScale<2, 1, 1>::planeToMaster(planeCoord);
          CHECK(masterCoord[0] == 2);
          CHECK(masterCoord[1] == 3);

          auto backToPlane = PlaneScale<2, 1, 1>::masterToPlane(masterCoord);
          CHECK(backToPlane[0] == 2);
          CHECK(backToPlane[1] == 3);
        }

        // 2:1 horizontal scaling
        {
          auto masterCoord = PlaneScale<2, 2, 1>::planeToMaster(planeCoord);
          CHECK(masterCoord[0] == 4);  // 2*2
          CHECK(masterCoord[1] == 3);  // 3*1

          // Reverse conversion
          CHECK(PlaneScale<2, 2, 1>::masterToPlane(masterCoord)[0] == 2);
          CHECK(PlaneScale<2, 2, 1>::masterToPlane(masterCoord)[1] == 3);

          // Check other master coordinates that map to the same plane coordinate
          CHECK(PlaneScale<2, 2, 1>::masterToPlane(Index<2>(4, 3))[0] == 2);
          CHECK(PlaneScale<2, 2, 1>::masterToPlane(Index<2>(5, 3))[0] == 2);  // Integer division: 5/2 = 2
        }
      }

      // Test range calculation
      {
        // Master range: 0,0 to 10,10
        // Note: IndexRange constructor takes min corner and max corner
        IndexRange<2> masterRange({{0,10}, {0,10}});

        // No scaling (1:1)
        {
          auto planeRange = PlaneScale<2, 1, 1>::calculateRange(masterRange);
          SPDLOG_INFO("No scaling - Plane range: [{}, {}] to [{}, {}]", 
                     planeRange.min(0), planeRange.min(1), 
                     planeRange.max(0), planeRange.max(1));

          // With no scaling, the plane range should match the master range
          CHECK(planeRange.min(0) == masterRange.min(0));
          CHECK(planeRange.min(1) == masterRange.min(1));
          CHECK(planeRange.max(0) == masterRange.max(0));
          CHECK(planeRange.max(1) == masterRange.max(1));
        }

        // 2:1 scaling in both dimensions
        {
          auto planeRange = PlaneScale<2, 2, 2>::calculateRange(masterRange);
          SPDLOG_INFO("2:1 scaling - Plane range: [{}, {}] to [{}, {}]", 
                     planeRange.min(0), planeRange.min(1), 
                     planeRange.max(0), planeRange.max(1));

          // With 2:1 scaling, the plane range should be half the master range
          // The formula in PlaneScale is: (master + scale - 1) / scale
          // For min: (0 + 2 - 1) / 2 = 0
          // For max: (10 + 2 - 1) / 2 = 5
          CHECK(planeRange.min(0) == 0);
          CHECK(planeRange.min(1) == 0);
          CHECK(planeRange.max(0) == 5);
          CHECK(planeRange.max(1) == 5);
        }
      }

      // Test what happens when converting back from plane to master
      {
        // Start with master range 0,0 to 10,10
        IndexRange<2> masterRange({{0, 10}, {0, 10}});

        // Convert to plane range with 2:1 scaling
        auto planeRange = PlaneScale<2, 2, 2>::calculateRange(masterRange);

        // Manually construct min and max indices
        Index<2> planeMin(planeRange.min(0), planeRange.min(1));
        Index<2> planeMax(planeRange.max(0), planeRange.max(1));

        // Convert back to master space
        auto masterMin = PlaneScale<2, 2, 2>::planeToMaster(planeMin);
        auto masterMax = PlaneScale<2, 2, 2>::planeToMaster(planeMax);

        // Check results
        CHECK(masterMin[0] == 0);
        CHECK(masterMin[1] == 0);
        CHECK(masterMax[0] == 10);  // 5*2 = 10
        CHECK(masterMax[1] == 10);

        // Important: masterMax[0] is 10, not 11, which means each plane pixel
        // covers coordinates from its mapped master coordinate up to
        // but not including the next plane pixel's mapped master coordinate

        // Now check what happens if we add the scale-1 to get the last
        // coordinate in the block
        CHECK(masterMax[0] + (2-1) == 11);  // Now we get 11
        CHECK(masterMax[1] + (2-1) == 11);  // Now we get 11
      }
    }

    SECTION("PlanarImage - Plane sizes")
    {
      // Test plane sizes for different image types

      // Regular RGB image (all planes same size)
      {
        IndexRange<2> range({{0, 10}, {0, 10}});
        RGBPlanarImage<uint8_t> rgbImage(range);

        // All planes should have the same size
        CHECK(rgbImage.plane<0>().range() == range);
        CHECK(rgbImage.plane<1>().range() == range);
        CHECK(rgbImage.plane<2>().range() == range);

        // Check master ranges too (should match for non-subsampled)
        CHECK(rgbImage.plane<0>().masterRange() == range);
        CHECK(rgbImage.plane<1>().masterRange() == range);
        CHECK(rgbImage.plane<2>().masterRange() == range);
      }

      // YUV 4:2:0 image (chroma planes at half resolution)
      {
        // Create a master range for testing
        Index<2> minCorner(0, 0);
        Index<2> maxCorner(10, 10);
        IndexRange<2> masterRange(minCorner, maxCorner);

        // Create a YUV image with the master range
        YUV420Image<uint8_t> yuvImage(masterRange);

        // Print the actual plane ranges for debugging
        SPDLOG_INFO("Y plane range: [{}, {}] to [{}, {}]", 
                   yuvImage.plane<0>().range().min(0), yuvImage.plane<0>().range().min(1),
                   yuvImage.plane<0>().range().max(0), yuvImage.plane<0>().range().max(1));

        SPDLOG_INFO("U plane range: [{}, {}] to [{}, {}]", 
                   yuvImage.plane<1>().range().min(0), yuvImage.plane<1>().range().min(1),
                   yuvImage.plane<1>().range().max(0), yuvImage.plane<1>().range().max(1));

        // Y plane should be full size (calculateRange with scale=1 is identity)
        CHECK(yuvImage.plane<0>().range().min(0) == masterRange.min(0));
        CHECK(yuvImage.plane<0>().range().min(1) == masterRange.min(1));
        CHECK(yuvImage.plane<0>().range().max(0) == masterRange.max(0));
        CHECK(yuvImage.plane<0>().range().max(1) == masterRange.max(1));

        // U and V planes should be half size in both dimensions
        // The formula in PlaneScale is: (master + scale - 1) / scale
        // For min: (0 + 2 - 1) / 2 = 0
        // For max: (10 + 2 - 1) / 2 = 5
        CHECK(yuvImage.plane<1>().range().min(0) == 0);
        CHECK(yuvImage.plane<1>().range().min(1) == 0);
        CHECK(yuvImage.plane<1>().range().max(0) == 5);
        CHECK(yuvImage.plane<1>().range().max(1) == 5);

        CHECK(yuvImage.plane<2>().range().min(0) == 0);
        CHECK(yuvImage.plane<2>().range().min(1) == 0);
        CHECK(yuvImage.plane<2>().range().max(0) == 5);
        CHECK(yuvImage.plane<2>().range().max(1) == 5);

        // Check master ranges
        CHECK(yuvImage.plane<0>().masterRange() == masterRange);

        // Due to the implementation of masterRange, the subsampled planes 
        // will cover a slightly larger range (by 1 pixel) in the subsampled
        // dimensions. This is because each plane pixel at coordinates (x,y)
        // covers master coordinates from (x*s, y*s) to ((x+1)*s-1, (y+1)*s-1),
        // where s is the scaling factor.
        IndexRange<2> expectedSubsampledRange({{0, 11}, {0, 11}});
        CHECK(yuvImage.plane<1>().masterRange() == expectedSubsampledRange);
        CHECK(yuvImage.plane<2>().masterRange() == expectedSubsampledRange);

        // Test coordinate access with subsampling
        Index<2> masterCoord(5, 5);

        // Master coordinates should map to expected plane coordinates
        CHECK(yuvImage.plane<0>().containsMaster(masterCoord));
        CHECK(yuvImage.plane<1>().containsMaster(masterCoord));
        CHECK(yuvImage.plane<2>().containsMaster(masterCoord));
      }
    }

    SECTION("PlanarImage::createPackedPixel - RGB")
    {
      // Create an RGB planar image with known values
      IndexRange<2> range({{0, 10}, {0, 10}}); // 10x10 image
      RGBPlanarImage<uint8_t> planarImage(range);

      // Fill each plane with different values
      planarImage.plane<0>().fill(100); // Red = 100
      planarImage.plane<1>().fill(150); // Green = 150
      planarImage.plane<2>().fill(200); // Blue = 200

      // Test createPackedPixel for PixelRGB8
      Index<2> testCoord(5, 5);

      // Verify the plane sizes are as expected
      CHECK(planarImage.plane<0>().range().min(0) == 0);
      CHECK(planarImage.plane<0>().range().min(1) == 0);
      CHECK(planarImage.plane<0>().range().max(0) == 10);
      CHECK(planarImage.plane<0>().range().max(1) == 10);

      CHECK(planarImage.plane<1>().range() == planarImage.plane<0>().range());
      CHECK(planarImage.plane<2>().range() == planarImage.plane<0>().range());

      // Check that the test coordinate is within the range
      CHECK(planarImage.plane<0>().range().contains(testCoord));
      CHECK(planarImage.plane<1>().range().contains(testCoord));
      CHECK(planarImage.plane<2>().range().contains(testCoord));

      // Verify plane values directly first
      CHECK(planarImage.plane<0>()[testCoord] == 100);
      CHECK(planarImage.plane<1>()[testCoord] == 150);
      CHECK(planarImage.plane<2>()[testCoord] == 200);

      auto rgbPixel = planarImage.createPackedPixel<Pixel, uint8_t, 
                          ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>(testCoord);

      CHECK(rgbPixel.get<ImageChannel::Red>() == 100);
      CHECK(rgbPixel.get<ImageChannel::Green>() == 150);
      CHECK(rgbPixel.get<ImageChannel::Blue>() == 200);
    }

    SECTION("PlanarImage::createPackedPixel - Component type conversion")
    {
      // Create an RGB planar image with uint8_t components
      IndexRange<2> range({{0, 10}, {0, 10}});
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
      CHECK(floatPixel.get<ImageChannel::Red>() == Catch::Approx(100.0f/255.0f).epsilon(0.01f));
      CHECK(floatPixel.get<ImageChannel::Green>() == Catch::Approx(150.0f/255.0f).epsilon(0.01f));
      CHECK(floatPixel.get<ImageChannel::Blue>() == Catch::Approx(200.0f/255.0f).epsilon(0.01f));
    }


    SECTION("PlanarImage::createPackedPixel - Missing channels")
    {
      // Create an RGB planar image (no alpha)
      IndexRange<2> range({{0, 10}, {0, 10}});
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
      CHECK(rgbaPixel.get<ImageChannel::Red>() == 100);
      CHECK(rgbaPixel.get<ImageChannel::Green>() == 150);
      CHECK(rgbaPixel.get<ImageChannel::Blue>() == 200);

      // Alpha should be default value (255 for uint8_t)
      CHECK(rgbaPixel.get<ImageChannel::Alpha>() == 255);
    }

    SECTION("convertToPlanar and convertToPacked - Roundtrip conversion")
    {
      // Create a packed pixel array with known values
      IndexRange<2> range({{0, 5}, {0, 5}}); // 5x5 image
      Array<PixelRGB8, 2> packedArray(range);

      // Fill the array with a pattern
      for (auto it = packedArray.begin(); it != packedArray.end(); ++it) {
        const auto& idx = it.index();
        (*it).set<ImageChannel::Red>(static_cast<uint8_t>(50 + idx[0] * 10));
        (*it).set<ImageChannel::Green>(static_cast<uint8_t>(100 + idx[1] * 10));
        (*it).set<ImageChannel::Blue>(static_cast<uint8_t>(150));
      }

      // Convert to planar
      auto planarImage = convertToPlanar(packedArray);

      // Verify planar structure
      REQUIRE(planarImage.planeCount() == 3);
      CHECK(planarImage.planeChannelType<0>() == ImageChannel::Red);
      CHECK(planarImage.planeChannelType<1>() == ImageChannel::Green);
      CHECK(planarImage.planeChannelType<2>() == ImageChannel::Blue);

      // Verify some values in the planar image
      CHECK(planarImage.plane<0>()[{2, 3}] == 70);  // Red at (2,3) = 50 + 2*10
      CHECK(planarImage.plane<1>()[{2, 3}] == 130); // Green at (2,3) = 100 + 3*10
      CHECK(planarImage.plane<2>()[{2, 3}] == 150); // Blue at (2,3) = 150

      // Convert back to packed
      auto roundtripArray = convertToPacked<uint8_t, Pixel,
                                           ImageChannel::Red,
                                           ImageChannel::Green,
                                           ImageChannel::Blue>(planarImage);

      // Verify the array has the same range
      CHECK(roundtripArray.range() == packedArray.range());

      // Verify some values in the roundtrip array
      CHECK(roundtripArray[{2, 3}].get<ImageChannel::Red>() == 70);
      CHECK(roundtripArray[{2, 3}].get<ImageChannel::Green>() == 130);
      CHECK(roundtripArray[{2, 3}].get<ImageChannel::Blue>() == 150);

      // Check all values match the original
      for (auto it = packedArray.begin(); it != packedArray.end(); ++it) {
        const auto& idx = it.index();
        const auto& original = *it;
        const auto& roundtrip = roundtripArray[idx];

        CHECK(roundtrip.get<ImageChannel::Red>() == original.get<ImageChannel::Red>());
        CHECK(roundtrip.get<ImageChannel::Green>() == original.get<ImageChannel::Green>());
        CHECK(roundtrip.get<ImageChannel::Blue>() == original.get<ImageChannel::Blue>());
      }
    }

    SECTION("convertToPacked - Component type conversion")
    {
      // Create a planar image with uint8_t planes
      IndexRange<2> range({{0, 3}, {0, 3}}); // Small 3x3 image
      RGBPlanarImage<uint8_t> planarImage(range);

      // Fill with some values
      planarImage.plane<0>().fill(100); // Red = 100
      planarImage.plane<1>().fill(150); // Green = 150
      planarImage.plane<2>().fill(200); // Blue = 200

      // Convert to packed array with float components
      auto floatArray = convertToPacked<float, Pixel,
                                       ImageChannel::Red,
                                       ImageChannel::Green,
                                       ImageChannel::Blue>(planarImage);

      // Check dimensions
      CHECK(floatArray.range() == planarImage.range());

      // Check values were converted to float range (0-1)
      CHECK(floatArray[{1, 1}].get<ImageChannel::Red>() == Catch::Approx(100.0f/255.0f).epsilon(0.01f));
      CHECK(floatArray[{1, 1}].get<ImageChannel::Green>() == Catch::Approx(150.0f/255.0f).epsilon(0.01f));
      CHECK(floatArray[{1, 1}].get<ImageChannel::Blue>() == Catch::Approx(200.0f/255.0f).epsilon(0.01f));
    }

    SECTION("convertToPacked - Custom channel selection and ordering")
    {
      // Create a planar image with R,G,B planes
      IndexRange<2> range({{0, 3}, {0, 3}});
      RGBPlanarImage<uint8_t> planarImage(range);

      // Fill with different values
      planarImage.plane<0>().fill(100); // Red = 100
      planarImage.plane<1>().fill(150); // Green = 150
      planarImage.plane<2>().fill(200); // Blue = 200

      // Convert to BGR order (reorder channels)
      auto bgrArray = convertToPacked<uint8_t, Pixel,
                                     ImageChannel::Blue,
                                     ImageChannel::Green,
                                     ImageChannel::Red>(planarImage);

      // Check that the channels were reordered correctly
      auto bgrPixel = bgrArray[{1, 1}];
      CHECK(bgrPixel[0] == 200); // First channel (Blue) should be 200
      CHECK(bgrPixel[1] == 150); // Second channel (Green) should be 150
      CHECK(bgrPixel[2] == 100); // Third channel (Red) should be 100

      // Convert to just Green channel
      auto greenArray = convertToPacked<uint8_t, Pixel, ImageChannel::Green>(planarImage);

      // Check that only the green channel was selected
      auto greenPixel = greenArray[{1, 1}];
      CHECK(greenPixel[0] == 150); // First and only channel (Green) should be 150
      CHECK(greenPixel.channel_count == 1);
    }
  }
}
