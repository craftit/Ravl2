#include <catch2/catch_test_macros.hpp>

#include <spdlog/spdlog.h>

#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Image/WarpScale2d.hh"
#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Image/Array2Sqr2Iter.hh"
#include "Ravl2/Image/Array2Sqr2Iter2.hh"

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define CHECK_NE(a,b) CHECK_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a,b) CHECK((a) == (b))
#define EXPECT_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))
#define ASSERT_FALSE(a) REQUIRE_FALSE(a)
#define ASSERT_TRUE(a) REQUIRE(a)
#define ASSERT_FLOAT_EQ(a,b) REQUIRE(Ravl2::isNearZero((a) -(b)))


TEST_CASE("BilinearInterpolation", "[Image]")
{
  Ravl2::Array<float,2> img({4,4});
  img.fill(0.0f);
  img[1][1] = 1.0;

  for(int i = 0;i < 3;i++) {
    for(int j = 0;j < 3;j++) {
      auto value = interpolate_bilinear(img,std::array<float,2>({float(i),float(j)}));
      ASSERT_FLOAT_EQ(img[i][j],value);
    }
  }

  auto value1 = interpolate_bilinear(img,std::array<float,2>({0.5f,0.5f}));
  ASSERT_FLOAT_EQ(0.25f,value1);

  auto value2 = interpolate_bilinear(img,std::array<float,2>({1.5f,1.5f}));
  ASSERT_FLOAT_EQ(0.25f,value2);
}


TEST_CASE("PeakDetection", "[Image]")
{
  using namespace Ravl2;
  Array<int,2> img({10,10});
  img.fill(0);
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


TEST_CASE("SubPixelPeakDetection", "[Image]")
{
  using namespace Ravl2;
  Array<float,2> img({3,3});
  img.fill(0);
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

TEST_CASE("Array2Sqr2Iter", "[Image]")
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

TEST_CASE("Array2Sqr2Iter2", "[Image]")
{
  using namespace Ravl2;

  Array<int, 2> data({4, 4});

  int count = 1;
  for(auto &ita : data)
    ita = count++;

  Array<int16_t,2> data2({4,4});

  data2.fill(1);
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

