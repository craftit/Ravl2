//
// Created by charles on 15/07/24.
//

#include <gtest/gtest.h>
#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Image/WarpScale2d.hh"
#include "Ravl2/Image/PeakDetector.hh"

TEST(Image, BilinearInterpolation)
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


TEST(Image, WarpScale2d)
{

}

TEST(Image, PeakDetection)
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


TEST(Image, SubPixelPeakDetection)
{
  using namespace Ravl2;
  Array<float,2> img({3,3});
  img.fill(0);
  img[1][1] = 1;
  Point2f at = LocatePeakSubPixel(img,Index<2>({1,1}));
  ASSERT_FALSE(std::abs(at[0] - 1) > 0.000001);
  ASSERT_FALSE(std::abs(at[1] - 1) > 0.000001);
  img[1][0] = 0.5;
  img[2][1] = 0.5;

  at = LocatePeakSubPixel(img,Index<2>({1,1}));
  //cerr << "At=" << at << "\n";
  ASSERT_FALSE(std::abs(at[0] - 0.9) > 0.000001);
  ASSERT_FALSE(std::abs(at[1] - 1.1) > 0.000001);
}
