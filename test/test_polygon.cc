
#include <numbers>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include <cereal/archives/json.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawPolygon.hh"
#include "Ravl2/Geometry/Polygon2d.hh"
#include "Ravl2/Geometry/Polygon2dIter.hh"

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a, b) CHECK((a) == (b))
#define EXPECT_NE(a, b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a, b) REQUIRE((a) == (b))
#define ASSERT_NE(a, b) REQUIRE_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a, b) REQUIRE(Ravl2::isNearZero((a) -(b)))

// If true render the polygon to an image for debugging
#define DODISPLAY 0

TEST_CASE("Polygon2dIter")
{
  using namespace Ravl2;
  SECTION("Iterate over polygon. ")
  {
    IndexRange<2> range(Index<2>(0, 0), Index<2>(10, 10));

    std::vector<std::tuple<int, IndexRange<1> >> expectedResult({
								  {1, IndexRange<1>(0, 1)},
								  {2, IndexRange<1>(0, 2)},
								  {2, IndexRange<1>(9, 9)},
								  {3, IndexRange<1>(0, 4)},
								  {3, IndexRange<1>(9, 9)},
								  {4, IndexRange<1>(0, 5)},
								  {4, IndexRange<1>(8, 9)},
								  {5, IndexRange<1>(0, 9)},
								  {6, IndexRange<1>(0, 9)},
								  {7, IndexRange<1>(0, 9)},
								  {8, IndexRange<1>(0, 9)},
								  {9, IndexRange<1>(0, 9)}
								}
							       );

    Polygon2dC<float> polygon;
    polygon.push_back(Point<float, 2>({0, 0}));
    polygon.push_back(Point<float, 2>({5, 7}));
    polygon.push_back(Point<float, 2>({0, 10}));
    polygon.push_back(Point<float, 2>({10, 10}));
    polygon.push_back(Point<float, 2>({10, 0}));
    unsigned int i = 0;
    for(Polygon2dIterC<float> it(polygon); it; ++it, ++i) {
//SPDLOG_INFO("{} {}", it[0], it.RowIndexRange());
      EXPECT_TRUE(range[0].contains(it.row()));
      EXPECT_TRUE(range[1].contains(it.rowIndexRange()));
      CHECK(i < expectedResult.size());
      EXPECT_EQ(it.row(), std::get<0>(expectedResult[i]));
      EXPECT_EQ(it.rowIndexRange(), std::get<1>(expectedResult[i]));
    }
    EXPECT_EQ(i, expectedResult.size());
  }
  SECTION("Cereal ")
  {
    Polygon2dC<float> polygon;
    polygon.push_back(Point<float, 2>({0, 0}));
    polygon.push_back(Point<float, 2>({5, 7}));
    polygon.push_back(Point<float, 2>({0, 10}));
    polygon.push_back(Point<float, 2>({10, 10}));
    polygon.push_back(Point<float, 2>({10, 0}));

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(polygon);
    }
//SPDLOG_INFO("Polygon2dC<float>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Polygon2dC<float> polygon2;
      iarchive(polygon2);
      CHECK(polygon.size() == polygon2.size());
      CHECK(polygon == polygon2);
    }
  }
}

TEST_CASE("Convex Polygon Overlap")
{

}

TEST_CASE("Polygon2d")
{
  using namespace Ravl2;
  SECTION("Self intersection")
  {
    {
      Polygon2dC<float> poly;
      poly.push_back(toPoint<float>(-10, 0));
      poly.push_back(toPoint<float>(0, 20));
      poly.push_back(toPoint<float>(10, 10));
      poly.push_back(toPoint<float>(10, 0));

      CHECK(!poly.IsSelfIntersecting());
      CHECK(poly.isConvex());
    }

    {
      Polygon2dC<float> poly;
      poly.push_back(toPoint<float>(0, 0));
      poly.push_back(toPoint<float>(10, 10));
      poly.push_back(toPoint<float>(10, 0));
      poly.push_back(toPoint<float>(0, 10));
      CHECK(poly.IsSelfIntersecting() == true);
      CHECK(!poly.isConvex());
    }
  }

  SECTION("Polygon2d Overlap")
  {
    Polygon2dC<float> poly;
    poly.push_back(toPoint<float>(0, 0));
    poly.push_back(toPoint<float>(0, 10));
    poly.push_back(toPoint<float>(10, 10));
    poly.push_back(toPoint<float>(10, 0));

    CHECK(poly.isConvex());

    auto score = poly.Overlap(poly);
    CHECK(std::abs(score - 1.0f) < 0.000001f);

    poly = poly.reverse();

    score = poly.Overlap(poly);
    CHECK(std::abs(score - 1.0f) < 0.000001f);

    Polygon2dC poly2 = poly;
    poly2 += toPoint<float>(100, 100);

    score = poly.Overlap(poly2);
    CHECK(std::abs(score) < 0.000001f);

    score = poly2.Overlap(poly);
    CHECK(std::abs(score) < 0.000001f);
  }
}


TEST_CASE("Clip Polygon")
{
  using namespace Ravl2;
  Polygon2dC<float> poly;
  poly.push_back(toPoint<float>(-10, 0));
  poly.push_back(toPoint<float>(0, 20));
  poly.push_back(toPoint<float>(10, 10));
  poly.push_back(toPoint<float>(10, 0));

  SPDLOG_INFO("Test poly: {}", poly);

  SECTION("Clip Axis")
  {
    Polygon2dC resultPoly = poly.ClipByAxis(0, 1, true);
    SPDLOG_INFO("clipByAxis all: {}", resultPoly);
    CHECK(resultPoly.size() == poly.size());
    CHECK(isNearZero(resultPoly.area() - poly.area()));

    resultPoly = poly.ClipByAxis(-0.1f, 1, false);
    SPDLOG_INFO("none: {}", resultPoly);
    CHECK(resultPoly.empty());

    resultPoly = poly.ClipByAxis(2, 1, false);
    SPDLOG_INFO("At 1: {}", resultPoly);
  }
#if 1
  SECTION("Clip Polygon")
  {
    Range<float, 2> range1({{0, 10},
                            {0, 10}});

    Range<float, 2> range2({{0, 10},
                            {0, 15}});

    Polygon2dC clippedConvex = poly.ClipByConvex(Polygon2dC(range1));
    Polygon2dC clippedRange = poly.ClipByRange(range1);


    {
      Array<int, 2> img({{-15, 15},
                         {-5,  25}}, 0);
      DrawFilledPolygon(img, 1, poly);
      SPDLOG_INFO("Poly: {}", img);
    }

    {
      Array<int, 2> img({{-15, 15},
                         {-5,  25}}, 0);
      Polygon2dC rect(range1);
      DrawFilledPolygon(img, 1, rect);
      SPDLOG_INFO("Rect: {}", img);
    }

    {
      Array<int, 2> img({{-15, 15},
                         {-5,  25}}, 0);
      DrawFilledPolygon(img, 1, clippedConvex);
      SPDLOG_INFO("clippedConvex: {}", img);
    }
    {
      SPDLOG_INFO("clippedRange: {}", clippedRange);

      Array<int, 2> img({{-15, 15},
                         {-5,  25}}, 0);
      DrawFilledPolygon(img, 1, clippedRange);
      SPDLOG_INFO("clippedRange: {}", img);
    }


    auto score = std::abs(clippedConvex.area());
    SPDLOG_INFO("clippedConvex area: {}   Signed:{} ", score, clippedConvex.area());
    CHECK(std::abs(score - 100) < 1e-6f);

    score = std::abs(clippedRange.area());
    SPDLOG_INFO("clippedRange: {}   Signed:{} ", score, clippedRange.area());
    CHECK(std::abs(score - 100) < 1e-6f);

    score = clippedConvex.Overlap(clippedRange);
    SPDLOG_INFO("clippedRange Overlap: {}  ", score);
    CHECK(std::abs(score - 1) < 1e-6f);

    // Clipping by two different routes should give the same result..
    clippedConvex = poly.ClipByConvex(Polygon2dC(range2));
    clippedRange = poly.ClipByRange(range2);



    score = clippedConvex.Overlap(clippedRange);
    SPDLOG_INFO("clippedRange Overlap: {}", score);
    CHECK(std::abs(score - 1) < 1e-6f);
  }
#endif
}

