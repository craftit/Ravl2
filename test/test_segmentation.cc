//
// Created by charles galambos on 27/07/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Image/Segmentation/FloodRegion.hh"
#include "Ravl2/Image/Segmentation/SegmentExtrema.hh"
#include "Ravl2/Image/Segmentation/ConnectedComponents.hh"
#include "Ravl2/Image/Segmentation/Segmentation.hh"

#define DODEBUG 0

#if DODEBUG
#include <opencv2/highgui.hpp>
#include "Ravl2/OpenCV/Image.hh"
#endif

namespace Ravl2
{
  TEST_CASE("FloodRegion")
  {
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


  TEST_CASE("SegmentExtrema on images with many minima")
  {
    // A checkerboard has ceil(area/2) local minima; the region map was sized area/2 rounded down.
    Array<uint8_t, 2> board({3, 3});
    for(int r = 0; r < 3; ++r)
      for(int c = 0; c < 3; ++c)
        board[r][c] = ((r + c) % 2 == 0) ? 10 : 200;
    SegmentExtrema<uint8_t> segmentExtrema(1);
    CHECK_NOTHROW((void)segmentExtrema.apply(board));

    Array<uint8_t, 2> single({1, 1}, 50);
    SegmentExtrema<uint8_t> segmentSingle(1);
    CHECK_NOTHROW((void)segmentSingle.apply(single));
  }

  TEST_CASE("SegmentExtrema")
  {
#if 1
    SECTION("Basic test")
    {
      using PixelT = uint8_t;
      Array<PixelT, 2> img({10, 10}, 100);

      // Setup a square in the middle of the image.
      auto rng = img.range().shrink(2);
      fill(clip(img, rng), uint8_t(10));

      SegmentExtrema<PixelT> segmentExtrema(8);

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

      SegmentExtrema<ByteT> segExt(5);
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
    Array<unsigned,2> test({8,8},0);

    test[1][1] = 1;
    test[1][2] = 1;
    test[2][1] = 1;
    test[6][6] = 1;
    test[5][6] = 1;
    //cerr << test;
    // Default label table size. (This passed 'false' as the table size, which hid a wrong count.)
    ConnectedComponents<unsigned> conComp;
    auto [segMap, regionCount] = conComp.apply(test);

    // Background and two blobs, labelled 1..3, so 4 label values including the reserved 0.
    CHECK(regionCount == 4);
    CHECK(segMap[1][1] == segMap[1][2]);
    CHECK(segMap[1][2] == segMap[2][1]);
    CHECK(segMap[6][6] == segMap[5][6]);
    CHECK(segMap[6][6] != segMap[1][1]);
    CHECK(segMap[6][6] != segMap[0][0]);
    CHECK(segMap[1][2] != segMap[0][0]);
  }

  TEST_CASE("ConnectedComponents counts regions and handles thin images")
  {
    // U shape: one region plus the background.
    Array<unsigned, 2> u({5, 5}, 0);
    for(int r = 0; r < 5; ++r) {
      u[r][0] = 1;
      u[r][4] = 1;
    }
    for(int c = 0; c < 5; ++c)
      u[4][c] = 1;
    ConnectedComponents<unsigned> cc;
    auto [uMap, uCount] = cc.apply(u);
    CHECK(uCount == 3);// labels 1 and 2, plus 0
    CHECK(uMap[0][0] == uMap[0][4]);

    ConnectedComponents<unsigned> ccIgnore(10000, true);
    auto [iMap, iCount] = ccIgnore.apply(u);
    CHECK(iCount == 2);// the U only
    CHECK(iMap[2][2] == 0);

    // Single row and single column images used to assert.
    Array<unsigned, 2> row({1, 6});
    for(int c = 0; c < 6; ++c)
      row[0][c] = unsigned(c / 2);
    auto [rowMap, rowCount] = cc.apply(row);
    CHECK(rowCount == 4);
    CHECK(rowMap[0][0] == rowMap[0][1]);
    CHECK(rowMap[0][1] != rowMap[0][2]);

    Array<unsigned, 2> col({6, 1});
    for(int r = 0; r < 6; ++r)
      col[r][0] = unsigned(r / 3);
    auto [colMap, colCount] = cc.apply(col);
    CHECK(colCount == 3);
    CHECK(colMap[2][0] == colMap[0][0]);
    CHECK(colMap[3][0] != colMap[2][0]);
  }

  TEST_CASE("FloodRegion maxSize and small images")
  {
    Array<int, 2> img({10, 10}, 99);
    fill(clip(img, img.range().shrink(2)), 10);// 6x6 = 36 pixels below threshold
    FloodRegionC<int> flood(img);
    // maxSize stops the flood: it used to be ignored for boundaries and left masks partly unwritten.
    Boundary boundary;
    CHECK(flood.GrowRegion(toIndex(5, 5), FloodRegionLessThanThresholdC(15), boundary, 36));
    CHECK_FALSE(flood.GrowRegion(toIndex(5, 5), FloodRegionLessThanThresholdC(15), boundary, 35));
    Array<int, 2> mask;
    CHECK(flood.GrowRegion(toIndex(5, 5), FloodRegionLessThanThresholdC(15), mask, 0, 36) == 36);
    CHECK(flood.GrowRegion(toIndex(5, 5), FloodRegionLessThanThresholdC(15), mask, 0, 35) == 0);
    CHECK(mask.range().empty());

    // 1x1 images.
    Array<int, 2> one({1, 1}, 5);
    FloodRegionC<int> floodOne(one);
    CHECK(floodOne.GrowRegion(toIndex(0, 0), FloodRegionLessThanThresholdC(15), mask) == 1);
    Array<unsigned, 2> oneLabel({1, 1}, 3u);
    ConnectedComponents<unsigned> cc;
    auto [map, count] = cc.apply(oneLabel);
    CHECK(count == 2);
  }

  TEST_CASE("Convex hull of a boundary")
  {
    // An L shape: its hull cuts across the inner corner. (Boundary::ConvexHull() was declared but never defined.)
    Array<int, 2> img({8, 8}, 99);
    fill(clip(img, IndexRange<2>({{1, 5}, {1, 2}})), 10);
    fill(clip(img, IndexRange<2>({{4, 5}, {1, 5}})), 10);
    FloodRegionC<int> flood(img);
    Boundary boundary;
    REQUIRE(flood.GrowRegion(toIndex(2, 1), FloodRegionLessThanThresholdC(15), boundary));
    const auto outline = toPolygon<float>(boundary.orderEdges().front());
    const auto hull = convexHull<float>(boundary.orderEdges().front());
    CHECK(hull.size() < outline.size());
    CHECK(std::abs(hull.area()) > std::abs(outline.area()));
    CHECK((hull.isConvex(BoundaryOrientationT::INSIDE_LEFT) || hull.isConvex(BoundaryOrientationT::INSIDE_RIGHT)));
  }

  TEST_CASE("Segmentation adjacency and boundary lengths count every edge")
  {
    // Column of label 1 next to label 2: the first window of each row used to be skipped.
    Array<unsigned, 2> seg({3, 3}, 2);
    for(int r = 0; r < 3; ++r)
      seg[r][0] = 1;
    Segmentation s(seg, 3);
    auto adj = s.Adjacency(false);
    CHECK(adj[1].count(2) == 1);
    auto len = s.BoundaryLength(false);
    CHECK(len[1][2] == 3);
    auto lenBi = s.BoundaryLength(true);
    CHECK(lenBi[2][1] == 3);
    CHECK(s.Areas().size() == 3);

    // An isolated pixel has 4 boundary edges.
    Array<unsigned, 2> dot({3, 3}, 1);
    dot[1][1] = 2;
    Segmentation d(dot, 3);
    auto local = d.LocalBoundaryLength();
    CHECK(local[2] == 4);
    CHECK(local[1] == 4);
  }
}

