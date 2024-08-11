
#include <numbers>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include <cereal/archives/json.hpp>

#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Moments2.hh"
#include "Ravl2/Geometry/Circle.hh"
#include "Ravl2/Geometry/CircleIter.hh"
#include "Ravl2/Geometry/LineABC2d.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/ScaleTranslate.hh"

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define CHECK_NE(a,b) CHECK_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a,b) CHECK((a) == (b))
#define EXPECT_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a,b) REQUIRE(Ravl2::isNearZero((a) -(b)))

TEST_CASE("Moments")
{
  using namespace Ravl2;

  SECTION("AddPixel")
  {
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

  SECTION("Cereal IO")
  {
    Moments2<double> moments(1,2,3,4,5,6);

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(moments);
    }
    SPDLOG_INFO("Json: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Moments2<double> moments2;
      iarchive(moments2);
      ASSERT_FLOAT_EQ(moments2.M00(), 1);
      ASSERT_FLOAT_EQ(moments2.M10(), 2);
      ASSERT_FLOAT_EQ(moments2.M01(), 3);
      ASSERT_FLOAT_EQ(moments2.M20(), 4);
      ASSERT_FLOAT_EQ(moments2.M11(), 5);
      ASSERT_FLOAT_EQ(moments2.M02(), 6);
    }
  }
}

TEST_CASE("Vector and Matrix")
{
  using namespace Ravl2;
  SECTION( "Point<float,2> Cereal. ")
  {
    Point<float,2> p1({1,2});
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      //serialize(oarchive, p1);
      oarchive(p1);
    }
    SPDLOG_INFO("Point<float,2>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Point<float,2> p2;
      //serialize(iarchive, p2);
      iarchive(p2);
      CHECK(isNearZero(p1(0) - p2(0)));
      CHECK(isNearZero(p1(1) - p2(1)));
    }
  }
  SECTION( "Matrix<float,2,3> Cereal. ")
  {
    Matrix<float,2,3> m1({{1,2,3},{4,5,6}});
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(m1);
    }
    // SPDLOG_INFO("Matrix<float,2,3>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Matrix<float,2,3> m2;
      iarchive(m2);
      CHECK(isNearZero(m1(0,0) - m2(0,0)));
      CHECK(isNearZero(m1(0,1) - m2(0,1)));
      CHECK(isNearZero(m1(0,2) - m2(0,2)));
      CHECK(isNearZero(m1(1,0) - m2(1,0)));
      CHECK(isNearZero(m1(1,1) - m2(1,1)));
      CHECK(isNearZero(m1(1,2) - m2(1,2)));
    }
  }
}


TEST_CASE("CircleIter", "[CircleIterC]")
{
  using namespace Ravl2;
  using RealT = float;

  int i = 0;
  unsigned rad = 20;
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
  //SPDLOG_INFO("Center={} Radius={}", circle2.Centre(), circle2.Radius());
  float sqrMag = xt::sum(xt::square(Point<float,2>(circle2.Centre() - Point<float,2>({1,2}))))[0];
  CHECK(sqrMag < 0.01f);
  CHECK(std::abs(circle2.Radius() - 2) < 0.01f);

#if 0
  Circle2dC<float> circle;
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

TEST_CASE("Affine")
{
  using namespace Ravl2;
  SECTION( "Composition. ")
  {
    Affine<float, 2>
      a1 = affineFromScaleAngleTranslation(toVector<float>(2, 2), std::numbers::pi_v<float> / 2, toVector<float>(0, 0));
    Affine<float, 2> a2 = affineFromScaleAngleTranslation(toVector<float>(1, 1), 0.0f, toVector<float>(10, 20));
    Point<float, 2> p = toPoint<float>(0, 0);
    Point<float, 2> pnt0 = a2(a1(p));
    //SPDLOG_INFO("At: {} {} ", pnt0(0), pnt0(1));
    CHECK(euclidDistance(pnt0, toPoint<float>(10, 20)) < 0.001f);
    Point<float, 2> pnt = a1(a2(p));
    //SPDLOG_INFO("At: {} {} ", pnt(0), pnt(1));
    CHECK(euclidDistance(pnt, toPoint<float>(-40, 20)) < 0.001f);
    Point<float, 2> q = toPoint<float>(5, 4);
    CHECK(Ravl2::euclidDistance(a2(a1)(q), a2(a1(q))) < 0.001f);
  }
  SECTION( "Cereal. ")
  {
    Affine<float, 2>
      a1 = affineFromScaleAngleTranslation(toVector<float>(1, 2), std::numbers::pi_v<float> / 3, toVector<float>(4, 5));
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(a1);
    }
    //SPDLOG_INFO("Affine<float, 2>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Affine<float, 2> a2;
      iarchive(a2);
      CHECK(isNearZero(a1.Translation()[0] - a2.Translation()[0]));
      CHECK(isNearZero(a1.Translation()[1] - a2.Translation()[1]));
      CHECK(isNearZero(a1.SRMatrix()(0,0) - a2.SRMatrix()(0,0)));
      CHECK(isNearZero(a1.SRMatrix()(0,1) - a2.SRMatrix()(0,1)));
      CHECK(isNearZero(a1.SRMatrix()(1,0) - a2.SRMatrix()(1,0)));
      CHECK(isNearZero(a1.SRMatrix()(1,1) - a2.SRMatrix()(1,1)));
    }
  }

}

TEST_CASE("ScaleTranslate")
{
  using namespace Ravl2;
  SECTION( "Basic ops ")
  {
    ScaleTranslate<float, 2> a1(toVector<float>(2, 2), toVector<float>(1, 2));
    CHECK(euclidDistance(a1(toPoint<float>(0, 0)), toPoint<float>(1, 2)) < 0.001f);
    CHECK(euclidDistance(a1(toPoint<float>(1, 1)), toPoint<float>(3, 4)) < 0.001f);
    ScaleTranslate<float, 2> a2(toVector<float>(2, 1), toVector<float>(1, 2));
    CHECK(euclidDistance(a2(toPoint<float>(1, 1)), toPoint<float>(3, 3)) < 0.001f);
  }
  SECTION( "Cereal. ")
  {
    ScaleTranslate<float, 2> a1(toVector<float>(1, 2), toVector<float>(3, 4));
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(a1);
    }
    //SPDLOG_INFO("ScaleTranslate<float, 2>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      ScaleTranslate<float, 2> a2;
      iarchive(a2);
      CHECK(isNearZero(a1.translation()[0] - a2.translation()[0]));
      CHECK(isNearZero(a1.translation()[1] - a2.translation()[1]));
      CHECK(isNearZero(a1.scaleVector()[0] - a2.scaleVector()[0]));
      CHECK(isNearZero(a1.scaleVector()[1] - a2.scaleVector()[1]));
    }
  }
}
