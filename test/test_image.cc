
#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Geometry/Projection.hh"

#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Image/WarpScale.hh"
#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Image/Array2Sqr2Iter.hh"
#include "Ravl2/Image/Array2Sqr2Iter2.hh"
#include "Ravl2/Image/Matching.hh"
#include "Ravl2/Image/ImageExtend.hh"
#include "Ravl2/Image/DrawFrame.hh"
#include "Ravl2/Image/Segmentation/Boundary.hh"
#include "Ravl2/Image/SummedAreaTable.hh"
#include "Ravl2/Image/SummedAreaTable2.hh"
#include "Ravl2/Image/MatchNormalisedCorrelation.hh"
#include "Ravl2/Image/ImagePyramid.hh"
#include "Ravl2/Image/ZigZagIter.hh"
#include "Ravl2/Image/DCT2d.hh"
#include "Ravl2/Image/Warp.hh"
#include "Ravl2/Image/WarpIter.hh"

namespace Ravl2
{
  TEST_CASE("BilinearInterpolation")
  {
    Ravl2::Array<float,2> img({4,4},0);
    img[1][1] = 1.0;

    for(int i = 0;i < 3;i++) {
      for(int j = 0;j < 3;j++) {
        auto value = interpolateBilinear(img, std::array<float, 2>({float(i), float(j)}));
        ASSERT_FLOAT_EQ(img[i][j],value);
      }
    }

    auto value1 = interpolateBilinear(img, std::array<float, 2>({0.5f, 0.5f}));
    ASSERT_FLOAT_EQ(0.25f,value1);

    auto value2 = interpolateBilinear(img, std::array<float, 2>({1.5f, 1.5f}));
    ASSERT_FLOAT_EQ(0.25f,value2);
  }

  TEST_CASE("WarpIter")
  {
    SECTION("Shift")
    {
      Ravl2::IndexRange<2> const rng { 12, 10 };

      auto transform = [](const Ravl2::Point2f &p) -> Ravl2::Point2f {
        return p + Ravl2::toPoint<float>(1, 1);
      };

      auto iter = Ravl2::beginWarp(rng, transform);
      CHECK(iter.valid());
      int count = 0;
      int rows = 0;
      while(iter.valid()) {
        do {
          auto groundTruth = transform(Ravl2::toPoint<float>(iter.index()));
          SPDLOG_TRACE("Index:{} Pnt:{} GT:{}", iter.index(), iter.point(), groundTruth);
          CHECK(Ravl2::isNearZero(Ravl2::euclidDistance(groundTruth,iter.warpedIndex())));
          count++;
        } while(iter.next());
        rows++;
      }
      CHECK(count == rng.area());
      CHECK(rows == rng.size(0));
    }

    SECTION("Affine")
    {
      Ravl2::IndexRange<2> const rng { 12, 10 };
      Ravl2::Affine<float,2> affine = Ravl2::Affine<float,2>::identity();
      affine.scale(Ravl2::toVector<float>(1,2));

      auto iter = Ravl2::beginWarp(rng, affine);
      CHECK(iter.valid());
      int count = 0;
      int rows = 0;
      while(iter.valid()) {
        do {
          auto groundTruth = affine(Ravl2::toPoint<float>(iter.index()));
          SPDLOG_TRACE("Index:{} Pnt:{} GT:{}", iter.index(), iter.warpedIndex(), groundTruth);
          CHECK(Ravl2::isNearZero(Ravl2::euclidDistance(groundTruth,iter.warpedIndex())));
          count++;
        } while(iter.next());
        rows++;
      }
      CHECK(count == rng.area());
      CHECK(rows == rng.size(0));
    }

    SECTION("Projective")
    {
      Ravl2::IndexRange<2> const rng { 12, 10 };
      Matrix<float, 3, 3> const projMat({{-592.402f, 128.055f,0.0f}, {-691.364f, 149.244f, 170.722f}, {-2.92932f, 0.632021f, 1.0f}});
      Ravl2::Projection<float,2> const projection(projMat);

      auto iter = Ravl2::beginWarp(rng, projection);
      CHECK(iter.valid());
      int count = 0;
      int rows = 0;
      while(iter.valid()) {
        do {
          auto groundTruth = projection(Ravl2::toPoint<float>(iter.index()));
          float const norm = groundTruth.norm();
          SPDLOG_TRACE("Index:{} Pnt:{} GT:{}  Dist:{} ", iter.index(), iter.point(), groundTruth, Ravl2::euclidDistance(groundTruth,iter.point())/norm);
          CHECK(Ravl2::isNearZero(Ravl2::euclidDistance(groundTruth,iter.warpedIndex())/norm, 1e-5f));
          count++;
        } while(iter.next());
        rows++;
      }
      CHECK(count == rng.area());
      CHECK(rows == rng.size(0));
    }
  }



  TEST_CASE("Warp")
  {
    SECTION("Clamp, Wrap and Mirror fill pixels that map outside the source")
    {
      // Source value = column. Shifting by 5 columns maps the right half of the target outside the source;
      // these modes used to leave those pixels unwritten.
      Ravl2::Array<float, 2> img({10, 10});
      for(int r = 0; r < 10; ++r)
        for(int c = 0; c < 10; ++c)
          img[r][c] = float(c);
      auto transform = [](const Ravl2::Point2f &p) -> Ravl2::Point2f { return p + Ravl2::toPoint<float>(0, 5); };

      Ravl2::Array<float, 2> clamped(img.range(), -1.0f);
      Ravl2::warp<Ravl2::WarpWrapMode::Clamp>(clamped, img, transform);
      CHECK(clamped[4][2] == 7.0f);
      CHECK(clamped[4][9] >= 8.0f);// clamped to the right edge

      Ravl2::Array<float, 2> wrapped(img.range(), -1.0f);
      Ravl2::warp<Ravl2::WarpWrapMode::Wrap>(wrapped, img, transform);
      CHECK(wrapped[4][9] >= 0.0f);

      Ravl2::Array<float, 2> mirrored(img.range(), -1.0f);
      Ravl2::warp<Ravl2::WarpWrapMode::Mirror>(mirrored, img, transform);
      CHECK(mirrored[4][9] >= 0.0f);
    }

    SECTION("Shift")
    {
      Ravl2::Array<float, 2> img({10, 10}, 0);
      clip(img, img.range().shrink(3)) = 1.0f;
      SPDLOG_TRACE("Source:{}", img);

      auto transform = [](const Ravl2::Point2f &p) -> Ravl2::Point2f {
        return p + Ravl2::toPoint<float>(1, 1);
      };

      Ravl2::Array<float, 2> target(img.range(), 0);
      Ravl2::warp(target, img, transform);

      SPDLOG_TRACE("Target:{}", target);
      //IndexRange<2> rng = img.range().shift(1, 1);
    }

    SECTION("Shift")
    {
      Ravl2::Array<uint8_t, 2> img({10, 10}, 0);
      clip(img, img.range().shrink(3)) = 1;
      SPDLOG_TRACE("Source:{}", img);

      auto transform = [](const Ravl2::Point2f &p) -> Ravl2::Point2f {
        return p + Ravl2::toPoint<float>(1, 1);
      };

      Ravl2::Array<uint8_t, 2> target(img.range(), 0);
      Ravl2::warp(target, img, transform);

      SPDLOG_TRACE("Target:{}", target);
      //IndexRange<2> rng = img.range().shift(1, 1);
    }

  }

  TEST_CASE("WarpBounds")
  {
    SECTION("Projection")
    {
      Ravl2::IndexRange<2> rng({{-8, 7}, {-12, 9}});
      // ([-592.402  128.055        0] [-691.364  149.244  170.722] [-2.92932 0.632021        1]) iz=1 oz=1
      Matrix<float, 3, 3> projMat({{-592.402f, 128.055f,0.0f}, {-691.364f, 149.244f, 170.722f}, {-2.92932f, 0.632021f, 1.0f}});
      Ravl2::Projection<float,2> projection(projMat);
      auto bounds = Ravl2::projectedBounds(projection, rng);
      SPDLOG_INFO("Bounds:{}", bounds);

      auto checkPnt = [&](const Ravl2::Point2f &pnt) {
        auto projPnt = projection(pnt);
        SPDLOG_INFO("Pnt:{} Proj:{}", pnt, projPnt);
        CHECK(bounds.contains(projPnt));
      };
      checkPnt(Ravl2::toPoint<float>(-8,-12));
      checkPnt(Ravl2::toPoint<float>(7,9));
      checkPnt(Ravl2::toPoint<float>(7,-12));
      checkPnt(Ravl2::toPoint<float>(-8,9));

#if 0
      checkPnt(Ravl2::toPoint<float>(0,0));
      checkPnt(Ravl2::toPoint<float>(0,0));

      CHECK(bounds.contains(projection(Ravl2::toPoint<float>(7,9))));

      for(auto ind : rng) {
        auto projPnt = projection(Ravl2::toPoint<float>(ind));
        if(!bounds.contains(projPnt)) {
          SPDLOG_INFO("Ind:{} Proj:{}", ind, projPnt);
        }
        CHECK(bounds.contains(projPnt));
      }
#endif
    }
  }

  TEST_CASE("PeakDetection")
  {
    Array<int,2> img({10,10}, 0);
    Index<2> at({5,5});

    // Check 3x3
    ASSERT_FALSE(PeakDetect3(img,at));
    img[at] = 1;
    ASSERT_TRUE(PeakDetect3(img,at));
    img[at + Index<2>(1,0)] = 1;
    ASSERT_FALSE(PeakDetect3(img,at));
    img[at + Index<2>(1,0)] = 0;
    img[at + Index<2>(0,1)] = 1;
    ASSERT_FALSE(PeakDetect3(img,at));
    img[at + Index<2>(0,1)] = 0;
    img[at + Index<2>(-1,0)] = 1;
    ASSERT_FALSE(PeakDetect3(img,at));
    img[at + Index<2>(-1,0)] = 0;
    img[at + Index<2>(0,-1)] = 1;
    ASSERT_FALSE(PeakDetect3(img,at));
    img[at + Index<2>(0,-1)] = 0;
    img[at] = 0;

    // Check 5x5
    ASSERT_FALSE(PeakDetect5(img,at));
    img[at] = 1;
    ASSERT_TRUE(PeakDetect5(img,at));
    img[at + Index<2>(1,0)] = 1;
    ASSERT_FALSE(PeakDetect5(img,at));
    img[at + Index<2>(1,0)] = 0;
    img[at + Index<2>(0,1)] = 1;
    ASSERT_FALSE(PeakDetect5(img,at));
    img[at + Index<2>(0,1)] = 0;
    img[at + Index<2>(-1,0)] = 1;
    ASSERT_FALSE(PeakDetect5(img,at));
    img[at + Index<2>(-1,0)] = 0;
    img[at + Index<2>(0,-1)] = 1;
    ASSERT_FALSE(PeakDetect5(img,at));
    img[at + Index<2>(0,-1)] = 0;
    img[at] = 0;

    // Check 7x7
    ASSERT_FALSE(PeakDetect7(img,at));
    img[at] = 1;
    ASSERT_TRUE(PeakDetect7(img,at));
    img[at + Index<2>(1,0)] = 1;
    ASSERT_FALSE(PeakDetect7(img,at));
    img[at + Index<2>(1,0)] = 0;
    img[at + Index<2>(0,1)] = 1;
    ASSERT_FALSE(PeakDetect7(img,at));
    img[at + Index<2>(0,1)] = 0;
    img[at + Index<2>(-1,0)] = 1;
    ASSERT_FALSE(PeakDetect7(img,at));
    img[at + Index<2>(-1,0)] = 0;
    img[at + Index<2>(0,-1)] = 1;
    ASSERT_FALSE(PeakDetect7(img,at));
  }


  TEST_CASE("Wide frames on non-square images")
  {
    // 4 rows x 10 columns: the wide DrawFrame swapped rows and columns, so this went out of bounds.
    Array<int, 2> img({4, 10}, 0);
    DrawFrame(img, 1, 1, img.range());
    int count = 0;
    for(auto v : img)
      count += v;
    CHECK(count == 2 * 10 + 2 * 2);// top and bottom rows, plus both ends of the two middle rows
    CHECK(img[0][9] == 1);
    CHECK(img[2][0] == 1);
    CHECK(img[2][9] == 1);
    CHECK(img[1][5] == 0);

    // Clipped by the image edge.
    Array<int, 2> clipped({4, 10}, 0);
    DrawFrame(clipped, 1, 2, IndexRange<2>({{-1, 5}, {3, 20}}));
    CHECK(clipped[0][3] == 1);
    CHECK(clipped[3][4] == 1);
    CHECK(clipped[3][6] == 0);

    Array<uint8_t, 2> src({4, 10}, 5);
    Array<uint8_t, 2> extended;
    extendImageFill(extended, src, 2, uint8_t(0));
    REQUIRE(extended.range() == IndexRange<2>({{-2, 5}, {-2, 11}}));
    CHECK(extended[-2][-2] == 0);
    CHECK(extended[5][11] == 0);
    CHECK(extended[0][0] == 5);
  }

  TEST_CASE("Summed area tables and normalised correlation")
  {
    // Sum of a 3x3 block of ones was 4: the corners above and left of the range weren't used.
    Array<uint8_t, 2> ones({6, 6}, 1);
    auto sat2 = SummedAreaTable2C<int64_t>::BuildTable(ones);
    CHECK(sat2.Sum(IndexRange<2>({{1, 3}, {2, 4}}))[0] == 9);
    CHECK(sat2.Sum1(IndexRange<2>({{0, 5}, {0, 5}})) == 36);

    // Top half 1s, bottom half 3s: top minus bottom is 8 - 24 = -16, the same as the horizontal case
    // on the transposed image. Ranges here are in table-corner coordinates.
    Array<int, 2> halves({4, 4}, 1);
    Array<int, 2> halvesT({4, 4}, 1);
    for(int r = 0; r < 4; ++r)
      for(int c = 0; c < 4; ++c) {
        halves[r][c] = r < 2 ? 1 : 3;
        halvesT[r][c] = c < 2 ? 1 : 3;
      }
    auto sat = SummedAreaTable<int>::buildTable(halves);
    auto satT = SummedAreaTable<int>::buildTable(halvesT);
    const IndexRange<2> corners({{-1, 3}, {-1, 3}});
    CHECK(sat.verticalDifference2(corners, 1) == -16);
    CHECK(satT.horizontalDifference2(corners, 1) == -16);

    // An exact copy of part of the image is found with a score of 1.
    Array<uint8_t, 2> img({20, 20});
    for(int r = 0; r < 20; ++r)
      for(int c = 0; c < 20; ++c)
        img[r][c] = uint8_t((r * 37 + c * 11 + r * c * 7) % 251);
    Array<uint8_t, 2> templ({5, 4});
    for(int r = 0; r < 5; ++r)
      for(int c = 0; c < 4; ++c)
        templ[r][c] = img[6 + r][9 + c];
    MatchNormalisedCorrelation matcher(img);
    float score = 0;
    Index<2> at;
    REQUIRE(matcher.Search(templ, IndexRange<2>({{0, 19}, {0, 19}}), score, at));
    CHECK(at == Index<2>(6, 9));
    CHECK(std::abs(score - 1.0f) < 1e-3f);

    // Positions outside the search area are not considered.
    (void)matcher.Search(templ, IndexRange<2>({{0, 3}, {0, 3}}), score, at);
    CHECK(IndexRange<2>({{0, 3}, {0, 3}}).contains(at));
    CHECK(score < 0.999f);
  }

  TEST_CASE("Reference DCT works for any range")
  {
    // Non-square, non-zero origin: the cosine terms used absolute indices and swapped the sizes.
    Array<float, 2> img(IndexRange<2>({{10, 13}, {20, 25}}));
    Array<float, 2> img0({4, 6});
    for(int r = 0; r < 4; ++r)
      for(int c = 0; c < 6; ++c) {
        img[10 + r][20 + c] = float((r * 7 + c * 3) % 11);
        img0[r][c] = img[10 + r][20 + c];
      }
    Array<float, 2> coeffs, coeffs0, back;
    forwardDCT(coeffs, img);
    forwardDCT(coeffs0, img0);
    CHECK(std::abs(coeffs[11][23] - coeffs0[1][3]) < 1e-4f);
    inverseDCT(back, coeffs);
    for(int r = 0; r < 4; ++r)
      for(int c = 0; c < 6; ++c)
        CHECK(std::abs(back[10 + r][20 + c] - img[10 + r][20 + c]) < 1e-3f);
  }

  TEST_CASE("Boundary to polygon offsets every vertex once")
  {
    // A single pixel at (2,3): its polygon corners are the pixel corners, half a pixel from the centre.
    auto poly = toPolygon<float>(toBoundary(IndexRange<2>({{2, 2}, {3, 3}})));
    REQUIRE(poly.size() == 4);
    for(const auto &p : poly) {
      CHECK(std::abs(std::abs(p[0] - 2.0f) - 0.5f) < 1e-6f);
      CHECK(std::abs(std::abs(p[1] - 3.0f) - 0.5f) < 1e-6f);
    }
  }

  TEST_CASE("SubPixelPeakDetection")
  {
    Array<float,2> img({3,3} , 0);
    img[1][1] = 1;
    Point2f at = LocatePeakSubPixel(img,Index<2>({1,1}));
    ASSERT_FALSE(std::abs(at[0] - 1) > 0.000001f);
    ASSERT_FALSE(std::abs(at[1] - 1) > 0.000001f);
    img[1][0] = 0.5;
    img[2][1] = 0.5;

    at = LocatePeakSubPixel(img,Index<2>({1,1}));
    //cerr << "At=" << at << "\n";
    // Raised neighbours to the left (col 0) and below (row 2) pull the peak to row 1.1, col 0.9.
    ASSERT_FALSE(std::abs(at[0] - 1.1f) > 0.000001f);
    ASSERT_FALSE(std::abs(at[1] - 0.9f) > 0.000001f);
  }

  TEST_CASE("SubPixelPeakDetection1")
  {
    auto offset = locatePeakSubPixel1(0.0f,1.0f,0.0f);
    //SPDLOG_INFO("Offset:{}", offset);
    ASSERT_FLOAT_EQ(offset,0.0f);
    offset = locatePeakSubPixel1(0.5f,0.8f,0.3f);
    //SPDLOG_INFO("Offset:{}", offset);
    ASSERT_FLOAT_EQ(offset,-0.25f);
    offset = locatePeakSubPixel1(0.3f,0.8f,0.5f);
    ASSERT_FLOAT_EQ(offset,0.25f);
    //SPDLOG_INFO("Offset:{}", offset);
    offset = locatePeakSubPixel1(0.9f,1.0f,0.1f);
    ASSERT_FLOAT_EQ(offset,-0.5f);
    //SPDLOG_INFO("Offset:{}", offset);
    offset = locatePeakSubPixel1(0.1f,1.0f,0.9f);
    ASSERT_FLOAT_EQ(offset,0.5f);
    //SPDLOG_INFO("Offset:{}", offset);
    offset = locatePeakSubPixel1(9.0f,10.0f,1.0f);
    ASSERT_FLOAT_EQ(offset,-0.5f);
    //SPDLOG_INFO("Offset:{}", offset);

  }

  // Test 2x2 iterators.

  TEST_CASE("Array2Sqr2Iter")
  {

    Array<int, 2> data({4, 4});

    int count = 1;
    for(auto &ita : data)
      ita = count++;

    //SPDLOG_INFO("Data:{}", data);
    count = 0;
    int sqrs = 0;
    Array2dSqr2IterC<int> it(data);
    ASSERT_TRUE(it);
    ASSERT_EQ(it.DataBL(), 5);
    ASSERT_EQ(it.DataBR(), 6);
    ASSERT_EQ(it.DataTL(), 1);
    ASSERT_EQ(it.DataTR(), 2);
    for(; it.valid() && sqrs < 10; ++it, ++sqrs)
      count += it.DataBR() + it.DataBL() + it.DataTR() + it.DataTL();
    ASSERT_FALSE(it);
    ASSERT_FALSE(it.valid());
    ASSERT_EQ(sqrs, 9);
    ASSERT_EQ(count, 306);
  }

  TEST_CASE("Array2Sqr2Iter2")
  {

    Array<int, 2> data({4, 4});

    int count = 1;
    for(auto &ita : data)
      ita = count++;

    Array<int16_t,2> data2({4,4},1);

    Array2dSqr2Iter2C<int,int16_t> it2(data,data2);
    ASSERT_TRUE(it2.valid());
    CHECK(it2.DataTL1() == 1);
    CHECK(it2.DataTR1() == 2);
    ASSERT_EQ(it2.DataBL1(),5);
    ASSERT_EQ(it2.DataBR1(),6);
    ASSERT_EQ(it2.DataTL2(),1);
    ASSERT_EQ(it2.DataTR2(),1);
    ASSERT_EQ(it2.DataBL2(),1);
    ASSERT_EQ(it2.DataBR2(),1);
    int sqrs = 0;
    count = 0;
    for(;it2.valid() && sqrs < 10;++it2,sqrs++)
      count += it2.DataBR1() + it2.DataBL1() + it2.DataTR1() + it2.DataTL1() +
        it2.DataBR2() + it2.DataBL2() + it2.DataTR2() + it2.DataTL2();
    ASSERT_EQ(sqrs,9);
    ASSERT_EQ(count,342);

    Array2dSqr2Iter2C<int,int> it2a(data,data);
    ASSERT_EQ(it2a.DataTL1(),1);
    ASSERT_EQ(it2a.DataTR1(),2);
    ASSERT_EQ(it2a.DataBL1(),5);
    ASSERT_EQ(it2a.DataBR1(),6);
    for(;it2a;++it2a) {
      ASSERT_EQ(it2a.DataBR1(), it2a.DataBR2());
      ASSERT_EQ(it2a.DataTR1(), it2a.DataTR2());
      ASSERT_EQ(it2a.DataBL1(), it2a.DataBL2());
      ASSERT_EQ(it2a.DataTL1(), it2a.DataTL2());
    }
  }

  TEST_CASE("matchSumAbsDifference")
  {
    using ByteT = uint8_t;
    for(size_t imgSize = 3;imgSize < 19;imgSize++) {
      Array<ByteT,2> img1({imgSize,imgSize});
      Array<ByteT,2> img2({imgSize,imgSize});
      int v = 0;
      for(auto &it : img1)
        it = ByteT((v++ % 32) + 1);

      v = 0;
      for(auto &it : img2)
        it = ByteT((v++ % 32) + 2);
      auto sum = matchSumAbsDifference<ByteT,int>(img1,img2);
      SPDLOG_INFO("Sum {} = {}",imgSize, sum);
      CHECK(sum == int(imgSize * imgSize));
    }
  }

  TEST_CASE("imageExtend")
  {
    Array<int,2> img({3,3});
    int val = 0;
    for(auto &it : img)
      it = val++;

    {
      Array<int, 2> result;
      extendImageFill(result,img,2,-1);
      CHECK(result[result.range().min()] == -1);
      CHECK(result[result.range().max()] == -1);
    }

    {
      Array<int, 2> result;
      extendImageCopy(result, img, 2);
      CHECK(result.range() == img.range().expand(2));
      CHECK(result[result.range().min()] == img[img.range().min()]);
      CHECK(result[result.range().max()] == img[img.range().max()]);
    }

    {
      Array<int, 2> result;
      extendImageMirror(result,img,2);
      // Since we have a 3x3 image, the result should be 7x7, with outer corners mirroring the internal opposite corner.
      CHECK(result[result.range().min()] == img[img.range().max()]);
      CHECK(result[result.range().max()] == img[img.range().min()]);
    }

  }


  TEST_CASE("ZigZagIter")
  {
    SECTION("ZigZagIter on 3x3")
    {
      Array<int, 2> img({3, 3}, 0);
      int val = 1;
      for (ZigZagIterC it(img.range()); it.valid(); ++it) {
        CHECK(img.range().contains(*it));
        CHECK(img[*it] == 0);
        img[*it] = val++;
      }
      CHECK(img.range().elements() == size_t(val - 1));
      //SPDLOG_INFO("ZigZagIter:{}", img);
      CHECK(img[0][0] == 1);
      CHECK(img[0][1] == 2);
      CHECK(img[1][0] == 3);
      CHECK(img[2][0] == 4);
      CHECK(img[1][1] == 5);
      CHECK(img[0][2] == 6);
      CHECK(img[1][2] == 7);
      CHECK(img[2][1] == 8);
      CHECK(img[2][2] == 9);
    }

    SECTION("ZigZagIter on a range of sizes")
    {
      for (size_t imgSize = 1; imgSize < 19; imgSize++) {
        Array<int, 2> img({imgSize, imgSize}, 0);
        int val = 1;
        for (ZigZagIterC it(img.range()); it.valid(); ++it) {
          CHECK(img.range().contains(*it));
          CHECK(img[*it] == 0);
          img[*it] = val++;
        }
        CHECK(img.range().elements() == size_t(val - 1));
        //SPDLOG_INFO("ZigZagIter:{}", img);
      }
    }

  }


  TEST_CASE("DiscreteCosineTransform (forwardDCT)")
  {
    using RealT = float;
    Array<RealT,2> img({16,16});

    // Generate random image.
    auto Random1 = []() -> RealT { return RealT(rand() % 256) / 256; };
    for(auto &it : img)
      it = Random1();

    img[3][3] = 1;
    img[4][3] = 1;
    img[4][4] = 1;
    img[3][4] = 1;
    Array<RealT,2> res;
    forwardDCT(res, img);
    //SPDLOG_INFO("Res:{}", res);

    SECTION("Check reference inverse forwardDCT.")
    {
      Array<RealT, 2> rimg;
      inverseDCT(rimg, res);

      for (auto it = zip(rimg, img); it.valid(); ++it) {
        CHECK(std::abs(it.template data<0>() - it.template data<1>()) < 0.001f);
      }
    }

    SECTION("ChanDCT.")
    {
      ChanDCT chandct(unsigned(img.range(0).size()));
      Array<RealT, 2> cimg = chandct.forwardDCT(img);
      //SPDLOG_INFO("ChanRes:{}", cimg);
      for (auto it = zip(cimg, res); it.valid(); ++it) {
        CHECK(std::abs(it.template data<0>() - it.template data<1>()) < 0.001f);
      }

    }

    SECTION("VecRadDCT.")
    {
      // This only computes the first 6 coefficients.
      unsigned nPts = 6;
      VecRadDCT vrdct(unsigned(img.range(0).size()), nPts);
      Array<RealT, 2> cimg2;
      for (int i = 0; i < 50000; i++)
        cimg2 = vrdct.forwardDCT(img);

      //SPDLOG_INFO("VecRadRes:{}", cimg2);
      unsigned count = 0;
      for (auto it = zip(cimg2, res); it.valid(); ++it) {
        CHECK(std::abs(it.template data<0>() - it.template data<1>()) < 0.001f);
        count++;
        if(count > nPts)
          break;
      }
    }
  }

  TEST_CASE("ImagePyramid")
  {
    SECTION("ConstructAndFind")
    {
      using PixelT = uint8_t;
      //Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
      // Make an image
      Array<PixelT, 2> patch({27, 27}, 0);
      patch[2][2] = 1.0f;
      patch[2][3] = 1.0f;
      patch[3][2] = 1.0f;
      patch[3][3] = 1.0f;

      auto sumImg = SummedAreaTable<uint32_t>::buildTable(patch);
      auto pyramid = buildImagePyramid(patch, sumImg, 3, toVector<float>(2, 2), 0, 4);

      ASSERT_EQ(pyramid.numLevels(), 3u);
      auto level = pyramid.findAreaScale(0.33f);
      //SPDLOG_INFO("Level: {}", level);
      ASSERT_EQ(level, 1u);
    }SECTION("Construct")
    {
      // Try using the build method that constructs the summed area table.
      // Make an image
      Array<uint8_t, 2> patch({27, 27}, 0);
      patch[2][2] = 1.0f;
      patch[2][3] = 1.0f;
      patch[3][2] = 1.0f;
      patch[3][3] = 1.0f;

      auto pyramid = buildImagePyramid(patch, 3, toVector<float>(2, 2));

      ASSERT_EQ(pyramid.numLevels(), 3u);
      auto level = pyramid.findAreaScale(0.33f);
      SPDLOG_TRACE("Level: {}", level);
      ASSERT_EQ(level, 1u);
    }

    SECTION("ConstructFloat")
    {
      using PixelT = float;
      //Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
      // Make an image
      Array<PixelT, 2> patch({28, 28}, 0);

      clip(patch, patch.range().shrink(6)) = 1.0f;

      auto pyramid = buildImagePyramid(patch, 3, toVector<float>(2, 2), 0, 4);

      ASSERT_EQ(pyramid.numLevels(), 3u);
      auto level = pyramid.findAreaScale(0.33f);
      //SPDLOG_INFO("Level: {}", level);
      ASSERT_EQ(level, 1u);
      //    for(size_t i = 0;i < pyramid.numLevels();i++) {
      //      SPDLOG_INFO("Level:{} Img:{} ", i, pyramid.level(i).image());
      //    }

    }

    SECTION("ConstructOffset")
    {
      using PixelT = float;
      //Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
      // Make an image
      IndexRange<2> range({{4, 15}, {8, 19}});
      Array<PixelT, 2> patch(range, 0);

      clip(patch, patch.range().shrink(2)) = 1.0f;

      auto pyramid = buildImagePyramid(patch, 3, toVector<float>(2, 2), 0, 4);

      ASSERT_EQ(pyramid.numLevels(), 3u);
      auto level = pyramid.findAreaScale(0.33f);
      //SPDLOG_INFO("Level: {}", level);
      ASSERT_EQ(level, 1u);
      //    for(size_t i = 0;i < pyramid.numLevels();i++) {
      //      SPDLOG_INFO("Level:{} Img:{} ", i, pyramid.level(i).image());
      //    }

    }

    SECTION("Construct Negative Origin")
    {
      using PixelT = float;
      //Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
      // Make an image
      IndexRange<2> const range({{-20, 30}, {-30, 60}});
      Array<PixelT, 2> const patch(range, 0);

      auto pyramid = Ravl2::buildImagePyramid(patch, 12, Ravl2::toVector<float>(2, 2), 2, 300);

    }
  }
}