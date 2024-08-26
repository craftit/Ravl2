
#include <numbers>
#include <cereal/archives/json.hpp>

#include "checks.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawPolygon.hh"
#include "Ravl2/Geometry/Polygon2d.hh"
#include "Ravl2/Geometry/Polygon2dIter.hh"


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

TEST_CASE("Polygon2d")
{
  using namespace Ravl2;
  SECTION("Self intersection")
  {
    {
      Polygon2dC<float> poly;
      poly.push_back(toPoint<float>(10, 0));
      poly.push_back(toPoint<float>(10, 10));
      poly.push_back(toPoint<float>(0, 20));
      poly.push_back(toPoint<float>(-10, 0));

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

  SECTION("Overlap")
  {
    Polygon2dC<float> poly;
    poly.push_back(toPoint<float>(10, 0));
    poly.push_back(toPoint<float>(10, 10));
    poly.push_back(toPoint<float>(0, 10));
    poly.push_back(toPoint<float>(0, 0));

    CHECK(poly.isConvex());
    CHECK(poly.area() > 0);

    auto score = poly.Overlap(poly);
    CHECK(std::abs(score - 1.0f) < 0.000001f);

    Polygon2dC poly2 = poly;
    poly2 += toPoint<float>(100, 100);

    score = poly.Overlap(poly2);
    CHECK(std::abs(score) < 0.000001f);

    score = poly2.Overlap(poly);
    CHECK(std::abs(score) < 0.000001f);
  }

  SECTION("Ranges and Area")
  {
    Range<float, 2> range1({{0, 10},
			    {0, 10}});
    Polygon2dC<float> poly1 = toPolygon(range1);
    CHECK(poly1.size() == 4);
    CHECK(isNearZero(poly1.area() - 100));

    // Let's create a hole
    Polygon2dC<float> poly2= toPolygon(range1, BoundaryOrientationT::INSIDE_RIGHT);
    CHECK(poly2.size() == 4);
    CHECK(isNearZero(poly2.area() + 100));
    CHECK(!poly2.isConvex());
  }
}


TEST_CASE("Clip Polygon")
{
  using namespace Ravl2;
  Polygon2dC<float> poly;
  poly.push_back(toPoint<float>(10, 0));
  poly.push_back(toPoint<float>(10, 10));
  poly.push_back(toPoint<float>(0, 20));
  poly.push_back(toPoint<float>(-10, 0));

  CHECK(poly.isConvex());
  CHECK(poly.area() > 0);

  //SPDLOG_INFO("Test poly: {}", poly);

  SECTION("Self Clipping")
  {
    auto selfClip = poly.ClipByConvex(poly);

    CHECK(selfClip.size() == poly.size());
    CHECK(isNearZero(selfClip.area() - poly.area()));
  }

  SECTION("Clip Axis")
  {
    Polygon2dC resultPoly = poly.ClipByAxis(0, 1, true);
    SPDLOG_INFO("clipByAxis all: {}", resultPoly);
    CHECK(resultPoly.size() == poly.size());
    CHECK(isNearZero(resultPoly.area() - poly.area()));

    resultPoly = poly.ClipByAxis(-0.1f, 1, false);
    SPDLOG_INFO("none: {}", resultPoly);
    CHECK(resultPoly.empty());

    Range<float,1> scanRange(-11, 21);
    const float step = 0.1f;
    const int numDivisions = int_round(scanRange.size() / step);
    int testNum = 0;
    for(unsigned axis = 0; axis < 2; ++axis) {
      // Try different starting points.
      for(size_t startAt = 0; startAt < poly.size(); startAt++) {
        Polygon2dC<float> poly2;
        for(size_t i = 0; i < poly.size(); ++i) {
          poly2.push_back(poly[((i + startAt) % poly.size())]);
        }
        CHECK(poly2.size() == poly.size());
        CHECK(isNearZero(poly2.area() - poly.area()));

        for(int i = 0; i < numDivisions; ++i) {
          float threshold = scanRange.min() + float(i) * step;

          // Check axis version
          {
            auto resultPoly1 = poly2.ClipByAxis(threshold, axis, false);
            if(!resultPoly1.empty()) {
              CHECK(resultPoly1.isConvex());
            }
            auto resultPoly2 = poly2.ClipByAxis(threshold, axis, true);
            if(!resultPoly2.empty()) {
              CHECK(resultPoly2.isConvex());
            }
            CHECK(isNearZero((resultPoly1.area() + resultPoly2.area()) - poly2.area(), 1e-4f));
          }

          // Check line.
          {
            auto axisLine = LinePP2dC<float>::fromStartAndDirection(toPoint<float>(threshold, threshold), toVector<float>(axis == 1 ? 1 : 0, axis == 0 ? 1 : 0));
            auto resultPoly1 = poly2.ClipByLine(axisLine, BoundaryOrientationT::INSIDE_RIGHT);
            if(!resultPoly1.empty()) {
              CHECK(resultPoly1.isConvex());
            }
            auto resultPoly2 = poly2.ClipByLine(axisLine, BoundaryOrientationT::INSIDE_LEFT);
            if(!resultPoly2.empty()) {
              CHECK(resultPoly2.isConvex());
            }
            CHECK(isNearZero((resultPoly1.area() + resultPoly2.area()) - poly2.area(), 1e-4f));
          }

#if DODISPLAY && false
          SPDLOG_INFO("Areas: {}  {} (Sum:{})  {}", resultPoly1.area(), resultPoly2.area(), resultPoly1.area() + resultPoly2.area(), poly.area());
          {
            Array<int, 2> img({{-15, 15},
                               {-5, 25}},
                              0);
            DrawFilledPolygon(img, 1, poly);
            SPDLOG_INFO("Full: {}", img);
          }

          {
            Array<int, 2> img({{-15, 15},
                               {-5, 25}},
                              0);
            DrawFilledPolygon(img, 1, resultPoly1);
            SPDLOG_INFO("Poly1: {}", img);
          }

          {
            Array<int, 2> img({{-15, 15},
                               {-5, 25}},
                              0);
            DrawFilledPolygon(img, 1, resultPoly2);
            SPDLOG_INFO("Poly2: {}", img);
          }
#endif
          testNum++;
        }
      }
    }
    SPDLOG_INFO("Tested {} cases", testNum);
  }

#if 1
  SECTION("Clip Polygon")
  {
    Range<float, 2> range1({{0, 10},
                            {0, 10}});

    Range<float, 2> range2({{0, 10},
                            {0, 15}});

    Polygon2dC range1Poly = toPolygon(range1);
    CHECK(range1Poly.size() == 4);
    CHECK(isNearZero(range1Poly.area() - 100));
    CHECK(range1Poly.isConvex());

    Polygon2dC clippedConvex = poly.ClipByConvex(range1Poly);
    Polygon2dC clippedRange = poly.ClipByRange(range1);

    CHECK(clippedConvex.size() > 0);
    CHECK(clippedRange.size() > 0);

#if DODISPLAY
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
      //SPDLOG_INFO("clippedRange: {}", clippedRange);

      Array<int, 2> img({{-15, 15},
                         {-5,  25}}, 0);
      DrawFilledPolygon(img, 1, clippedRange);
      SPDLOG_INFO("clippedRange: {}", img);
    }
#endif


    auto score = clippedConvex.area();
    CHECK(std::abs(score - 100) < 1e-6f);

    score = clippedRange.area();
    CHECK(std::abs(score - 100) < 1e-6f);

    score = clippedConvex.Overlap(clippedRange);
    CHECK(std::abs(score - 1) < 1e-6f);

    score = clippedConvex.CommonOverlap(clippedRange);
    CHECK(std::abs(score - 1) < 1e-6f);

    // Clipping by two different routes should give the same result..
    clippedConvex = poly.ClipByConvex(Polygon2dC(range2));
    clippedRange = poly.ClipByRange(range2);

    score = clippedConvex.Overlap(clippedRange);
    CHECK(std::abs(score - 1) < 1e-6f);
  }
#endif

}

