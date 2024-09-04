//
// Created by charles galambos on 27/07/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Image/Segmentation/FloodRegion.hh"
#include "Ravl2/Image/Segmentation/SegmentExtrema.hh"
#include "Ravl2/Image/Segmentation/ConnectedComponents.hh"

#define DODEBUG 0

#if DODEBUG
#include <opencv2/highgui.hpp>
#include "Ravl2/OpenCV/Image.hh"
#endif

TEST_CASE("FloodRegion")
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
    SPDLOG_TRACE("Boundary: {}  ({})", boundary.size(), size_t((rng.range(0).size() + rng.range(1).size()) * 2));
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
    for(auto it = zip(mask,clip(img,mask.range())); it.valid(); ++it) {
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
#if 1
  SECTION("Basic test")
  {
    using PixelT = uint8_t;
    Array<PixelT, 2> img({10, 10}, 100);

    // Setup a square in the middle of the image.
    auto rng = img.range().shrink(2);
    fill(clip(img, rng), uint8_t(10));

    SegmentExtremaC<PixelT> segmentExtrema(8);

    std::vector<Boundary> boundaries = segmentExtrema.apply(img);

    REQUIRE(!boundaries.empty());
    CHECK(boundaries.size() == 1);
    for ([[maybe_unused]] auto &boundary: boundaries) {
      SPDLOG_TRACE("Boundary: {} -> {}", boundary.boundingBox(), boundary);
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
#endif
  SECTION("Multi level")
  {
    using ByteT = uint8_t;
    Array<ByteT,2> img({100,100}, 196);
    DrawFilledFrame(img,ByteT(128),IndexRange<2>({{10,90},{10,90}}));
    DrawFilledFrame(img,ByteT(196),IndexRange<2>({{20,30},{20,30}}));
    DrawFilledFrame(img,ByteT(64),IndexRange<2>({{20,30},{40,50}}));
    DrawFilledFrame(img,ByteT(196),IndexRange<2>({{40,50},{40,50}}));

    SegmentExtremaC<ByteT> segExt(5);
    std::vector<Boundary> bnd = segExt.apply(img);
    SPDLOG_TRACE("Bounds: {}", bnd.size());
    CHECK(bnd.size() == 2);

#if DODEBUG
    {
      cv::Mat cvImg = toCvMat(img);
      cv::imshow("Image", cvImg);

      Ravl2::Array<ByteT,2> imgAcc(img.range(),128);
      for(auto it : bnd) {
        for(auto eit : it.edges()) {
          imgAcc[eit.leftPixel()] = 255;
          imgAcc[eit.rightPixel()] = 0;
        }
        cv::Mat cvAcc = toCvMat(imgAcc);
        cv::imshow("Acc", cvAcc);
        cv::waitKey(0);
        fill(imgAcc,128);
      }
    };
#endif
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

  //cerr << "Regions=" << result.data<1>() << "\n";
  //cerr << segMap;
  CHECK(regionCount == 4);
  CHECK(segMap[1][1] == segMap[1][2]);
  CHECK(segMap[1][2] == segMap[2][1]);
  CHECK(segMap[6][6] == segMap[5][6]);
  CHECK(segMap[6][6] != segMap[1][1]);
  CHECK(segMap[6][6] != segMap[0][0]);
  CHECK(segMap[1][2] != segMap[0][0]);
}

