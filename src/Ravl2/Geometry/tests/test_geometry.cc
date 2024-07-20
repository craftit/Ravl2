#include <gtest/gtest.h>

#include "Ravl2/Geometry/Moments2.hh"

TEST(Geometry, Moments)
{
  using namespace Ravl2;

  Moments2<double> moments;
  moments.addPixel(Index<2>(0, 0));
  moments.addPixel(Index<2>(1, 0));
  moments.addPixel(Index<2>(0, 1));
  moments.addPixel(Index<2>(1, 1));

  auto principal_axis = moments.principalAxisSize();
  ASSERT_FLOAT_EQ(principal_axis[0], 0.25);
  ASSERT_FLOAT_EQ(principal_axis[1], 0.25);
  ASSERT_FLOAT_EQ(Moments2<double>::elongatedness(principal_axis), 0.0);
  ASSERT_FLOAT_EQ(moments.centroid()[0], 0.5);
  ASSERT_FLOAT_EQ(moments.centroid()[1], 0.5);

}