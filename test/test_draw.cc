
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/Image/DrawCircle.hh"
#include "Ravl2/Image/DrawLine.hh"
#include "Ravl2/Image/DrawFrame.hh"

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define CHECK_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a,b) CHECK((a) == (b))
#define EXPECT_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a,b) REQUIRE(Ravl2::isNearZero((a) -(b)))

TEST_CASE("Draw", "[DrawLine]")
{
  using namespace Ravl2;
  
  // Do some quick tests on the draw functions.
  Array<uint8_t,2> img({100,100}, 0);
  Index<2> from(10,10);
  Index<2> to(90,90);
  Index<2> out1(200,20);
  Index<2> out2(200,40);
  DrawLine(img,uint8_t(255),out1,out2);

  // Check draws outside the image don't crash or change the image.
  for(auto x : img) {
    EXPECT_EQ(x,0);
  }

  DrawLine(img,uint8_t(255),from,to);
  EXPECT_EQ(img[50][50],255);
  EXPECT_EQ(img[from],255);
  EXPECT_EQ(img[to],255);


  DrawCircle(img,uint8_t(254),Index<2>(50,50),30);

  DrawFrame(img,uint8_t(255),5,img.range());
  //Save("@X",img);
  EXPECT_EQ(img[0][0], 255);
  EXPECT_EQ(img[99][99], 255);
  EXPECT_EQ(img[15][10], 0);

  DrawFrame(img,uint8_t(128),img.range());
  EXPECT_EQ(img[0][0],128);
  EXPECT_EQ(img[99][99],128);
  EXPECT_EQ(img[0][99],128);
  EXPECT_EQ(img[99][0],128);

#if 0
  //Ellipse2dC ellipse(Point<RealT,2>(50,50),40,20,0);
  //DrawEllipse(img,(uint8_t) 255,ellipse);

#endif
}