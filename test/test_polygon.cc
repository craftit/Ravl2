
#include <numbers>
#include "Ravl2/IO/Cereal.hh"

#include "checks.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawPolygon.hh"
#include "Ravl2/Image/DrawCross.hh"
#include "Ravl2/Geometry/Polygon.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/Geometry/PolygonRasterIter.hh"
#include "Ravl2/Geometry/Moments2.hh"
#include "Ravl2/Geometry/PolyLine.hh"
#include "Ravl2/Geometry/PolyApprox.hh"

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

    Polygon<float> polygon;
    polygon.push_back(Point<float, 2>({0, 0}));
    polygon.push_back(Point<float, 2>({5, 7}));
    polygon.push_back(Point<float, 2>({0, 10}));
    polygon.push_back(Point<float, 2>({10, 10}));
    polygon.push_back(Point<float, 2>({10, 0}));
    unsigned int i = 0;
    for(PolygonRasterIter<float> it(polygon); it; ++it, ++i) {
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
    Polygon<float> polygon;
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
//SPDLOG_INFO("Polygon<float>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Polygon<float> polygon2;
      iarchive(polygon2);
      CHECK(polygon.size() == polygon2.size());
      CHECK(polygon == polygon2);
    }
  }
}

TEST_CASE("Polygon")
{
  using namespace Ravl2;
  SECTION("Self intersection")
  {
    {
      Polygon<float> poly;
      poly.push_back(toPoint<float>(10, 0));
      poly.push_back(toPoint<float>(10, 10));
      poly.push_back(toPoint<float>(0, 20));
      poly.push_back(toPoint<float>(-10, 0));

      CHECK(!poly.isSelfIntersecting());
      CHECK(poly.isConvex());
    }

    {
      Polygon<float> poly;
      poly.push_back(toPoint<float>(0, 0));
      poly.push_back(toPoint<float>(10, 10));
      poly.push_back(toPoint<float>(10, 0));
      poly.push_back(toPoint<float>(0, 10));
      CHECK(poly.isSelfIntersecting() == true);
      CHECK(!poly.isConvex());
    }
  }

  SECTION("Contains")
  {
    Polygon<float> poly;
    poly.push_back(toPoint<float>(10, 0));
    poly.push_back(toPoint<float>(10, 10));
    poly.push_back(toPoint<float>(0, 10));
    poly.push_back(toPoint<float>(0, 0));

    CHECK(poly.isConvex());
    CHECK(poly.area() > 0);
    EXPECT_FLOAT_EQ(poly.perimeter(),40.0f);
    CHECK(euclidDistance(poly.centroid(),toPoint<float>(5,5)) < 1e-6f);


    auto center = poly.centroid();
    CHECK(poly.contains(center));
    CHECK(!poly.contains(toPoint<float>(20,20)));
    CHECK(!poly.contains(toPoint<float>(5,20)));
    CHECK(!poly.contains(toPoint<float>(20,5)));
    CHECK(!poly.contains(toPoint<float>(-5,5)));
    CHECK(!poly.contains(toPoint<float>(-5,-5)));


    // Poly contains uses boundary counting, so orientation doesn't matter.
    auto rPoly = poly.reverse();
    CHECK(rPoly.size() == poly.size());
    EXPECT_FLOAT_EQ(rPoly.area(),-poly.area());
    CHECK(rPoly.contains(center));
    CHECK(!rPoly.contains(toPoint<float>(20,20)));
    CHECK(!rPoly.contains(toPoint<float>(5,20)));
    CHECK(!rPoly.contains(toPoint<float>(20,5)));
    CHECK(!rPoly.contains(toPoint<float>(-5,5)));
    CHECK(!rPoly.contains(toPoint<float>(-5,-5)));
  }

  SECTION("Contains 2")
  {
    Polygon<float> poly({Point2f ({396.887756f,  296.020416f}) ,{ 429.030609f,  538.367371f}, { 243.31633f,  538.877563f}, { 240.765305f,  297.040802f}});

    Point2f pnt = {318.115112f,  385.001129f};

//    {
//      Array<uint8_t , 2> img({600,600}, 0);
//      DrawPolygon(img, 255, poly);
//      DrawCross(img, 128, toIndex(pnt),5);
//      save("dlib://poly", img);
//    }
    CHECK(poly.contains(pnt));

  }

  SECTION("Moments")
  {
    Polygon<double> poly({Point2d ({396.887756,  296.020416}) ,{ 429.030609,  538.367371}, { 243.31633,  538.877563}, { 240.765305,  297.040802}});

    auto val = moments(poly);
    SPDLOG_TRACE("Moments: {} Centroid:{} ", val, val.centroid());
    SPDLOG_TRACE("Centroid:{} ", poly.centroid());

    CHECK((val.centroid() - poly.centroid()).cwiseAbs().sum() < 1e-6);

  }



  SECTION("overlap")
  {
    Polygon<float> poly;
    poly.push_back(toPoint<float>(10, 0));
    poly.push_back(toPoint<float>(10, 10));
    poly.push_back(toPoint<float>(0, 10));
    poly.push_back(toPoint<float>(0, 0));

    CHECK(poly.isConvex());
    CHECK(poly.area() > 0);


    auto score = poly.overlap(poly);
    CHECK(std::abs(score - 1.0f) < 0.000001f);

    Polygon poly2 = poly;
    poly2 += toPoint<float>(100, 100);

    score = poly.overlap(poly2);
    CHECK(std::abs(score) < 0.000001f);

    score = poly2.overlap(poly);
    CHECK(std::abs(score) < 0.000001f);
  }

  SECTION("Ranges and Area")
  {
    Range<float, 2> range1({{0, 10},
			    {0, 10}});
    Polygon<float> poly1 = toPolygon(range1);
    CHECK(poly1.size() == 4);
    CHECK(isNearZero(poly1.area() - 100));

    // Let's create a hole
    Polygon<float> poly2= toPolygon(range1, BoundaryOrientationT::INSIDE_RIGHT);
    CHECK(poly2.size() == 4);
    CHECK(isNearZero(poly2.area() + 100));
    CHECK(!poly2.isConvex());
  }


}


TEST_CASE("Clip Polygon")
{
  using namespace Ravl2;
  Polygon<float> poly;
  poly.push_back(toPoint<float>(10, 0));
  poly.push_back(toPoint<float>(10, 10));
  poly.push_back(toPoint<float>(0, 20));
  poly.push_back(toPoint<float>(-10, 0));

  CHECK(poly.isConvex());
  CHECK(poly.area() > 0);

  //SPDLOG_INFO("Test poly: {}", poly);

  SECTION("Self Clipping")
  {
    auto selfClip = poly.clipByConvex(poly);

    CHECK(selfClip.size() == poly.size());
    CHECK(isNearZero(selfClip.area() - poly.area()));
  }

  SECTION("Clip Axis")
  {
    Polygon resultPoly = poly.clipByAxis(0, 1, true);
    SPDLOG_TRACE("clipByAxis all: {}", resultPoly);
    CHECK(resultPoly.size() == poly.size());
    CHECK(isNearZero(resultPoly.area() - poly.area()));

    resultPoly = poly.clipByAxis(-0.1f, 1, false);
    SPDLOG_TRACE("none: {}", resultPoly);
    CHECK(resultPoly.empty());

    Range<float,1> scanRange(-11, 21);
    const float step = 0.1f;
    const int numDivisions = intRound(scanRange.size() / step);
    [[maybe_unused]] int testNum = 0;
    for(unsigned axis = 0; axis < 2; ++axis) {
      // Try different starting points.
      for(size_t startAt = 0; startAt < poly.size(); startAt++) {
        Polygon<float> poly2;
        for(size_t i = 0; i < poly.size(); ++i) {
          poly2.push_back(poly[((i + startAt) % poly.size())]);
        }
        CHECK(poly2.size() == poly.size());
        CHECK(isNearZero(poly2.area() - poly.area()));

        for(int i = 0; i < numDivisions; ++i) {
          float threshold = scanRange.min() + float(i) * step;

          // Check axis version
          {
            auto resultPoly1 = poly2.clipByAxis(threshold, axis, false);
            if(!resultPoly1.empty()) {
              CHECK(resultPoly1.isConvex());
            }
            auto resultPoly2 = poly2.clipByAxis(threshold, axis, true);
            if(!resultPoly2.empty()) {
              CHECK(resultPoly2.isConvex());
            }
            CHECK(isNearZero((resultPoly1.area() + resultPoly2.area()) - poly2.area(), 1e-4f));
          }

          // Check line.
          {
            auto axisLine = Line2PP<float>::fromStartAndDirection(toPoint<float>(threshold, threshold), toVector<float>(axis == 1 ? 1 : 0, axis == 0 ? 1 : 0));
            auto resultPoly1 = poly2.clipByLine(axisLine, BoundaryOrientationT::INSIDE_RIGHT);
            if(!resultPoly1.empty()) {
              CHECK(resultPoly1.isConvex());
            }
            auto resultPoly2 = poly2.clipByLine(axisLine, BoundaryOrientationT::INSIDE_LEFT);
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
    SPDLOG_TRACE("Tested {} cases", testNum);
  }

#if 1
  SECTION("Clip Polygon")
  {
    Range<float, 2> range1({{0, 10},
                            {0, 10}});

    Range<float, 2> range2({{0, 10},
                            {0, 15}});

    Polygon range1Poly = toPolygon(range1);
    CHECK(range1Poly.size() == 4);
    CHECK(isNearZero(range1Poly.area() - 100));
    CHECK(range1Poly.isConvex());

    Polygon clippedConvex = poly.clipByConvex(range1Poly);
    Polygon clippedRange = poly.clipByRange(range1);

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
      Polygon rect(range1);
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

    score = clippedConvex.overlap(clippedRange);
    CHECK(std::abs(score - 1) < 1e-6f);

    score = clippedConvex.commonOverlap(clippedRange);
    CHECK(std::abs(score - 1) < 1e-6f);

    // Clipping by two different routes should give the same result..
    clippedConvex = poly.clipByConvex(Polygon(range2));
    clippedRange = poly.clipByRange(range2);

    score = clippedConvex.overlap(clippedRange);
    CHECK(std::abs(score - 1) < 1e-6f);
  }
#endif

}

TEST_CASE("Approximate Polygon")
{
  using namespace Ravl2;
#if 0
  SECTION("Approx Simple")
  {
    PolyLine<float,2> poly;
    poly.push_back(toPoint<float>(0, 0));
    poly.push_back(toPoint<float>(10, 0));
    poly.push_back(toPoint<float>(20, 0));
    
    auto simplifed = poly.approx(1.0f);
    CHECK(simplifed.size() == 2);
  }
  
  
  SECTION("Approx 3")
  {
    PolyLine<float,2> poly;
    poly.push_back(toPoint<float>(0, 0));
    poly.push_back(toPoint<float>(10, 2));
    poly.push_back(toPoint<float>(20, 0));
    
    auto simplifed = poly.approx(1.0f);
    SPDLOG_INFO("Simplified: {}", simplifed);
    CHECK(simplifed.size() == 3);
  }
  
  SECTION("Approx 4")
  {
    PolyLine<float,2> poly;
    poly.push_back(toPoint<float>(0, 0));
    poly.push_back(toPoint<float>(10, 2));
    poly.push_back(toPoint<float>(20, 0));
    poly.push_back(toPoint<float>(30, 0));
    poly.push_back(toPoint<float>(40, 0));
    
    auto simplifed = poly.approx(1.0f);
    SPDLOG_INFO("Simplified: {}", simplifed);
    CHECK(simplifed.size() == 4);
  }
#endif
}


