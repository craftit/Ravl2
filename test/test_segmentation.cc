//
// Created by charles galambos on 27/07/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Image/Segmentation/FloodRegion.hh"
#include "Ravl2/Image/Segmentation/SegmentExtrema.hh"


TEST_CASE("FloodRegion", "[FloodRegion]")
{
  using namespace Ravl2;
  using PixelT = int;
  Array<PixelT,2> img({10,10}, 99);
  Index<2> at = toIndex(5,5);

  // Setup a square in the middle of the image.
  auto rng = img.range().shrink(2);
  fill(clip(img,rng),10);
  //SPDLOG_INFO("Image: {}", img);

  SECTION("Test boundary creation")
  {
    FloodRegionC<PixelT> flood(img);

    Boundary boundary;
    //SPDLOG_INFO("Seed: {}", at);
    CHECK(flood.GrowRegion(at, FloodRegionLessThanThresholdC(15), boundary));
    for(auto it : boundary.edges()) {
      //SPDLOG_INFO("Edge: {}", it);
      CHECK(img[it.LPixel()] == 10);
      CHECK(img[it.RPixel()] == 99);
    }
    CHECK(boundary.BoundingBox() == rng);
    SPDLOG_INFO("Boundary: {}  ({})", boundary.size(), size_t((rng.range(0).size() + rng.range(1).size()) * 2));
    CHECK(boundary.size() == size_t((rng.range(0).size() + rng.range(1).size()) * 2));
    CHECK(boundary.area() == rng.area());
  }

  SECTION("Test mask creation")
  {
    FloodRegionC<PixelT> flood(img);

    Array<unsigned,2> mask;
    size_t area = flood.GrowRegion(at, FloodRegionLessThanThresholdC(15), mask);
    CHECK(area == size_t(rng.area()));
    int count = 0;
    //SPDLOG_INFO("Mask: {}", mask);
    for(auto it = begin(mask,clip(img,mask.range())); it.valid(); ++it) {
      if(it.template data<0>() == 1) {
	CHECK(it.template data<1>() == 10);
	count++;
      } else {
	CHECK(it.template data<1>() == 99);
      }
    }
    CHECK(count == rng.area());
   // SPDLOG_INFO("Mask: {}", mask);
  }

}


TEST_CASE("SegmentExtrema")
{
  using namespace Ravl2;
  using PixelT = uint8_t;
  Array<PixelT,2> img({10,10}, 100);

  // Setup a square in the middle of the image.
  auto rng = img.range().shrink(2);
  fill(clip(img,rng),uint8_t(10));

  SegmentExtremaC<PixelT> segmentExtrema(8);

  std::vector<Boundary> boundaries = segmentExtrema.Apply(img);

  REQUIRE(!boundaries.empty());
  CHECK(boundaries.size() == 2);
  for(auto &boundary : boundaries) {
    SPDLOG_INFO("Boundary: {} -> {}", boundary.BoundingBox(), boundary);
  }

  {
    const Boundary &boundary = boundaries[0];

    CHECK(boundary.BoundingBox() == rng);
    CHECK(boundary.area() == rng.area());

    for(auto it : boundary.edges()) {
      //SPDLOG_INFO("Edge: {}", it);
      REQUIRE(img.range().contains(it.LPixel()));
      REQUIRE(img.range().contains(it.RPixel()));
      CHECK(img[it.LPixel()] <= 20);
      CHECK(img[it.RPixel()] >= 20);
    }
  }


}
