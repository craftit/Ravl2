//
// Created by charles galambos on 27/07/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Image/Segmentation/FloodRegion.hh"
#include "Ravl2/Image/Segmentation/SegmentExtrema.hh"
#include "Ravl2/Image/Segmentation/ConnectedComponents.hh"


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
      CHECK(img[it.leftPixel()] == 10);
      CHECK(img[it.rightPixel()] == 99);
    }
    CHECK(boundary.boundingBox() == rng);
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

  SECTION("Basic test")
  {
    Array<PixelT, 2> img({10, 10}, 100);

    // Setup a square in the middle of the image.
    auto rng = img.range().shrink(2);
    fill(clip(img, rng), uint8_t(10));

    SegmentExtremaC<PixelT> segmentExtrema(8);

    std::vector<Boundary> boundaries = segmentExtrema.Apply(img);

    REQUIRE(!boundaries.empty());
    CHECK(boundaries.size() == 2);
    for (auto &boundary: boundaries) {
      SPDLOG_INFO("Boundary: {} -> {}", boundary.boundingBox(), boundary);
    }

    {
      const Boundary &boundary = boundaries[0];

      CHECK(boundary.boundingBox() == rng);
      CHECK(boundary.area() == rng.area());

      for (auto it: boundary.edges()) {
        //SPDLOG_INFO("Edge: {}", it);
        REQUIRE(img.range().contains(it.leftPixel()));
        REQUIRE(img.range().contains(it.rightPixel()));
        CHECK(img[it.leftPixel()] <= 20);
        CHECK(img[it.rightPixel()] >= 20);
      }
    }
  }
  SECTION("Multi level")
  {
    using ByteT = uint8_t;
    ImageC<ByteT> img({100,100}, 196);
    DrawFrame(img,(ByteT) 128,IndexRange2dC(10,90,10,90),true);
    DrawFrame(img,(ByteT) 196,IndexRange2dC(20,30,20,30),true);
    DrawFrame(img,(ByteT) 64,IndexRange2dC(20,30,40,50),true);
    DrawFrame(img,(ByteT) 196,IndexRange2dC(40,50,40,50),true);
    SegmentExtremaC<ByteT> segExt(5);
    DListC<BoundaryC> bnd = segExt.Apply(img);
    DListC<ImageC<IntT> > segs = segExt.ApplyMask(img);
    SPDLOG_INFO("Bounds: {}  Segs: {}", bnd.size(), segs.size());

    for(DLIterC<ImageC<IntT> > it(segs);it;it++) {
      IndexRange2dC frame = it->Frame();
      frame.ClipBy(img.Frame());
      for(Array2dIter2C<ByteT,IntT> iit(img,*it,frame);iit;iit++)
        if(iit.Data2() != 0) iit.Data1() = 255;
    }
  }
}



TEST_CASE("ConnectedComponents")
{
  using namespace Ravl2;
  Array<unsigned,2> test({8,8},0);


  test[1][1] = 1;
  test[1][2] = 1;
  test[2][1] = 1;
  test[6][6] = 1;
  test[5][6] = 1;
  //cerr << test;
  ConnectedComponents<unsigned> conComp(false);
  auto [segMap, regionCount] = conComp.apply(test);

  //cerr << "Regions=" << result.Data2() << "\n";
  //cerr << segMap;
  CHECK(regionCount == 4);
  CHECK(segMap[1][1] == segMap[1][2]);
  CHECK(segMap[1][2] == segMap[2][1]);
  CHECK(segMap[6][6] == segMap[5][6]);
  CHECK(segMap[6][6] != segMap[1][1]);
  CHECK(segMap[6][6] != segMap[0][0]);
  CHECK(segMap[1][2] != segMap[0][0]);
}

