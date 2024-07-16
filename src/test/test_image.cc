//
// Created by charles on 15/07/24.
//

#include <gtest/gtest.h>
#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Image/WarpScale2d.hh"

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
