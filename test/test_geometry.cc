
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/Moments2.hh"
#include "Ravl2/Geometry/Circle.hh"
#include "Ravl2/Geometry/Polygon2d.hh"
#include "Ravl2/Geometry/Polygon2dIter.hh"
#include "Ravl2/Geometry/CircleIter.hh"
#include "Ravl2/Geometry/LineABC2d.hh"

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define CHECK_NE(a,b) CHECK_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a,b) CHECK((a) == (b))
#define EXPECT_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a,b) REQUIRE(Ravl2::isNearZero((a) -(b)))

TEST_CASE("Moments", "[Moments2]")
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

TEST_CASE("PolygonIter", "[Polygon2dC]")
{
  using namespace Ravl2;
  IndexRange<2> range(Index<2>(0,0), Index<2>(10,10));

  std::vector<std::tuple<int,IndexRange<1> >> expectedResult ({
        {1,IndexRange<1>(0,1)},
        {2,IndexRange<1>(0,2)},
        {2,IndexRange<1>(9,9)},
        {3,IndexRange<1>(0,4)},
        {3,IndexRange<1>(9,9)},
        {4,IndexRange<1>(0,5)},
        {4,IndexRange<1>(8,9)},
        {5,IndexRange<1>(0,9)},
        {6,IndexRange<1>(0,9)},
        {7,IndexRange<1>(0,9)},
        {8,IndexRange<1>(0,9)},
        {9,IndexRange<1>(0,9)}
          });

  Polygon2dC<float> polygon;
  polygon.push_back(Point<float,2>({0,0}));
  polygon.push_back(Point<float,2>({5,7}));
  polygon.push_back(Point<float,2>({0,10}));
  polygon.push_back(Point<float,2>({10,10}));
  polygon.push_back(Point<float,2>({10,0}));
  unsigned int i = 0;
  for(Polygon2dIterC<float> it(polygon); it; ++it, ++i) {
    //SPDLOG_INFO("{} {}", it.Row(), it.RowIndexRange());
    EXPECT_TRUE(range[0].contains(it.Row()));
    EXPECT_TRUE(range[1].contains(it.RowIndexRange()));
    CHECK(i < expectedResult.size());
    EXPECT_EQ(it.Row(), std::get<0>(expectedResult[i]));
    EXPECT_EQ(it.RowIndexRange(), std::get<1>(expectedResult[i]));
  }
  EXPECT_EQ(i, expectedResult.size());
}


TEST_CASE("CircleIter", "[CircleIterC]")
{
  using namespace Ravl2;
  int i = 0;
  int rad = 20;
  RealT maxDist = 0;
  Point<float,2> origin({0,0});
  Point<float,2> at;
  for(CircleIterC it(rad);it.IsElm();it.Next(),i++) {
    at[0] = float(it.Data()[0]);
    at[1] = float(it.Data()[1]);
    RealT dst =  euclidDistance<float,2>(at,origin);
    RealT diff = std::abs(RealT(rad) - dst);
    //cerr << "Dst:" << Abs((RealT) rad - dst) << " ";
    CHECK(diff < 0.5f); // Should never be greater than 0.5 !
    if(diff > maxDist)
      maxDist = diff;
    //cout << it.Data() << std::endl;
  }
  //SPDLOG_INFO("Points:{} Largest error:{}", i, maxDist);
  EXPECT_EQ(i,112);
}

TEST_CASE("Circle2", "[Circle2]")
{
  using namespace Ravl2;

  std::vector<Point<float,2>> pnts(5);
  pnts[0] = Point<float,2>({1,0});
  pnts[1] = Point<float,2>({-1,2});
  pnts[2] = Point<float,2>({3,2});
  pnts[3] = Point<float,2>({1,4});
  pnts[4] = Point<float,2>({1,4.01f}); // Just to add a slight error.


  Circle2dC<float> circle2;
  EXPECT_TRUE(circle2.Fit(pnts[0],pnts[1],pnts[2]));
  SPDLOG_INFO("Center={} Radius={}", circle2.Centre(), circle2.Radius());
  float sqrMag = xt::sum(xt::square(Point<float,2>(circle2.Centre() - Point<float,2>({1,2}))))[0];
  CHECK(sqrMag < 0.01f);
  CHECK(std::abs(circle2.Radius() - 2) < 0.01f);

  Circle2dC<float> circle;
#if 0
  RealT residual;
  if(!circle.FitLSQ(pnts,residual))
    return __LINE__;
  //cerr << "Residual=" << residual << "\n";
  //cerr << "Center=" << circle.Centre() << " Radius=" << circle.Radius() << "\n";
  if(Point<float,2>(circle.Centre() - Point<float,2>(1,2)).SumOfSqr() > 0.01)
    return __LINE__;
  if(Abs(circle.Radius() - 2) > 0.01)
    return __LINE__;
#endif
}
