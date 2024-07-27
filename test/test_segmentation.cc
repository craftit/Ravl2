//
// Created by charles galambos on 27/07/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Image/Segmentation/FloodRegion.hh"


TEST_CASE("FloodRegion", "[FloodRegion]")
{
  using namespace Ravl2;
  using PixelT = int;
  Array<PixelT,2> img({10,10});
  img.fill(99);
  Index<2> at = toIndex(5,5);

  // Setup a square in the middle of the image.
  auto rng = img.range().shrink(2);
  clip(img,rng).fill(10);
  SPDLOG_INFO("Image: {}", img);

  SECTION("Test boundary creation")
  {
    FloodRegionC<PixelT> flood(img);

    BoundaryC boundary;
    SPDLOG_INFO("Seed: {}", at);
    CHECK(flood.GrowRegion(at, FloorRegionThresholdC(15), boundary));
    for(auto it : boundary.edges()) {
      SPDLOG_INFO("Edge: {}", it);
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
    int area = flood.GrowRegion(at, FloorRegionThresholdC(15), mask);
    CHECK(area == rng.area());
    int count = 0;
    SPDLOG_INFO("Mask: {}", mask);
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