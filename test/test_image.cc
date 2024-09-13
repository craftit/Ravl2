
#include "checks.hh"

#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Image/WarpScale.hh"
#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Image/Array2Sqr2Iter.hh"
#include "Ravl2/Image/Array2Sqr2Iter2.hh"
#include "Ravl2/Image/Matching.hh"
#include "Ravl2/Image/ImageExtend.hh"
#include "Ravl2/Image/ImagePyramid.hh"
#include "Ravl2/Image/ZigZagIter.hh"
#include "Ravl2/Image/DCT2d.hh"
#include "Ravl2/Image/Warp.hh"

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

TEST_CASE("Warp")
{
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

}


TEST_CASE("PeakDetection")
{
  using namespace Ravl2;
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


TEST_CASE("SubPixelPeakDetection")
{
  using namespace Ravl2;
  Array<float,2> img({3,3} , 0);
  img[1][1] = 1;
  Point2f at = LocatePeakSubPixel(img,Index<2>({1,1}));
  ASSERT_FALSE(std::abs(at[0] - 1) > 0.000001f);
  ASSERT_FALSE(std::abs(at[1] - 1) > 0.000001f);
  img[1][0] = 0.5;
  img[2][1] = 0.5;

  at = LocatePeakSubPixel(img,Index<2>({1,1}));
  //cerr << "At=" << at << "\n";
  ASSERT_FALSE(std::abs(at[0] - 0.9f) > 0.000001f);
  ASSERT_FALSE(std::abs(at[1] - 1.1f) > 0.000001f);
}

// Test 2x2 iterators.

TEST_CASE("Array2Sqr2Iter")
{
  using namespace Ravl2;

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
  using namespace Ravl2;

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
  using namespace Ravl2;
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
  using namespace Ravl2;
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
  using namespace Ravl2;
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
  using namespace Ravl2;
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
  using namespace Ravl2;

  SECTION("ConstructAndFind")
  {
    // Make an image
    Array<uint8_t,2> patch({27,27},0);
    patch[2][2] = 1.0f;
    patch[2][3] = 1.0f;
    patch[3][2] = 1.0f;
    patch[3][3] = 1.0f;

    auto sumImg =  SummedAreaTable<uint32_t>::buildTable(patch);
    auto pyramid = buildImagePyramid(patch, sumImg, 3, toVector<float>(2,2));

    ASSERT_EQ(pyramid.numLevels(), 3u);
    auto level = pyramid.findAreaScale(0.33f);
    SPDLOG_INFO("Level: {}", level);
    ASSERT_EQ(level, 1u);
  }
  SECTION("Construct")
  {
    // Try using the build method that constructs the summed area table.
    // Make an image
    Array<uint8_t,2> patch({27,27},0);
    patch[2][2] = 1.0f;
    patch[2][3] = 1.0f;
    patch[3][2] = 1.0f;
    patch[3][3] = 1.0f;

    auto pyramid = buildImagePyramid(patch, 3, toVector<float>(2,2));

    ASSERT_EQ(pyramid.numLevels(), 3u);
    auto level = pyramid.findAreaScale(0.33f);
    SPDLOG_INFO("Level: {}", level);
    ASSERT_EQ(level, 1u);

  }

}
