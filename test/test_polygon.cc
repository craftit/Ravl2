
#include <numbers>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include <cereal/archives/json.hpp>

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
//SPDLOG_INFO("{} {}", it.Row(), it.RowIndexRange());
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

#if 0
TEST_CASE("Scan polygon")
{
  UIntT count = 0;
#if DODISPLAY
  ByteT drawVal = 255;
  ImageC<ByteT> img(105,105);
  img.Fill(0);
#endif
  Polygon2dC poly;
#if 1
  poly.InsLast(Point2dC(5, 10));
  poly.InsLast(Point2dC(40, 50));
  poly.InsLast(Point2dC(100, 20));

  for(ScanPolygon2dC it(poly); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:1",img);
  cerr << " ---------- Test 2 --------------------------- \n";
#endif
  //cerr << "Entries=" << count <<"\n";

  count = 0;
#if DODISPLAY
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(20, 40));
  poly.InsLast(Point2dC(40, 20));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:2",img);
  cerr << " ---------- Test 3 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 30));
  poly.InsLast(Point2dC(40, 40));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(30, 10));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:3",img);
  cerr << " ---------- Test 4 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(10, 90));
  poly.InsLast(Point2dC(90, 90));
  poly.InsLast(Point2dC(90, 10));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:4",img);
#endif

#if DODISPLAY
  cerr << " ---------- Test 5 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(10, 20));
  poly.InsLast(Point2dC(20, 20));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(30, 30));
  poly.InsLast(Point2dC(30, 10));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
#if 0
    if(it.Data().Size() > 0.001 &&
       !poly.Contains(Point2dC(it.Row(),it.Data().Center())))
      return __LINE__;
#endif
    count++;
  }
#if DODISPLAY
  Save("@X:5",img);
#endif

#if DODISPLAY
  cerr << " ---------- Test 6 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(10, 20));
  poly.InsLast(Point2dC(20, 20));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(10, 30));
  poly.InsLast(Point2dC(10, 40));
  poly.InsLast(Point2dC(40, 40));
  poly.InsLast(Point2dC(40, 30));
  poly.InsLast(Point2dC(30, 30));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(40, 20));
  poly.InsLast(Point2dC(40, 10));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:6",img);
#endif

#if DODISPLAY
  cerr << " ---------- Test 7 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 20));
  poly.InsLast(Point2dC(10, 30));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(20, 40));
  poly.InsLast(Point2dC(30, 40));
  poly.InsLast(Point2dC(30, 30));
  poly.InsLast(Point2dC(40, 30));
  poly.InsLast(Point2dC(40, 20));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(30, 10));
  poly.InsLast(Point2dC(20, 10));
  poly.InsLast(Point2dC(20, 20));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center()))) {
      //Save("@X:7",img);
      return __LINE__;
    }
    count++;
  }
#if DODISPLAY
  Save("@X:7",img);
#endif

#if DODISPLAY
  cerr << " ---------- Test 8 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(10, 40));
  poly.InsLast(Point2dC(20, 40));
  poly.InsLast(Point2dC(20, 30));
  //poly.InsLast(Point2dC(30,30));
  poly.InsLast(Point2dC(30, 30));
  poly.InsLast(Point2dC(30, 40));
  poly.InsLast(Point2dC(40, 40));
  poly.InsLast(Point2dC(40, 10));
  poly.InsLast(Point2dC(30, 10));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(20, 20));
  poly.InsLast(Point2dC(20, 10));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center()))) {
      std::cerr << " Row:" << it.Row() << " Span:" << it.Data() << "\n";
      //  Save("@X:8",img);
      return __LINE__;
    }
    count++;
  }
#if DODISPLAY
  Save("@X:8",img);
#endif

#if DODISPLAY
  cerr << " ---------- Test 9 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(10, 30));
  poly.InsLast(Point2dC(30, 40));
  poly.InsLast(Point2dC(10, 50));
  poly.InsLast(Point2dC(40, 40));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(40, 20));
  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:9",img);
#endif

#if DODISPLAY
  cerr << " ---------- Test 10 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();

  poly.InsLast(Point2dC(20, 40));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(40, 40));
  poly.InsLast(Point2dC(50, 10));
  poly.InsLast(Point2dC(40, 30));
  poly.InsLast(Point2dC(30, 10));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(10, 10));

  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:10",img);
#endif
#if DODISPLAY
  cerr << " ---------- Test 11 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(10, 60));
  poly.InsLast(Point2dC(50, 60));
  poly.InsLast(Point2dC(50, 40));
  poly.InsLast(Point2dC(40, 40));
  poly.InsLast(Point2dC(40, 50));
  poly.InsLast(Point2dC(30, 50));
  poly.InsLast(Point2dC(30, 40));
  poly.InsLast(Point2dC(20, 40));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(30, 30));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(40, 20));
  poly.InsLast(Point2dC(40, 30));
  poly.InsLast(Point2dC(50, 30));
  poly.InsLast(Point2dC(50, 10));

  for(ScanPolygon2dC it(poly, 1); it; it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center())))
      return __LINE__;
    count++;
  }
#if DODISPLAY
  Save("@X:11",img);
#endif
#endif
#if DODISPLAY
  cerr << " ---------- Test 12 --------------------------- \n";
  img = ImageC<ByteT>(105,105);
  img.Fill(0);
#endif
  poly.Empty();
  int stop = 1000;

  poly.InsLast(Point2dC(10, 50));
  poly.InsLast(Point2dC(30, 50));
  poly.InsLast(Point2dC(30, 40));
  poly.InsLast(Point2dC(20, 40));
  poly.InsLast(Point2dC(20, 30));
  poly.InsLast(Point2dC(30, 30));
  poly.InsLast(Point2dC(30, 20));
  poly.InsLast(Point2dC(40, 20));
  poly.InsLast(Point2dC(40, 30));
  poly.InsLast(Point2dC(50, 30));
  poly.InsLast(Point2dC(50, 40));
  poly.InsLast(Point2dC(40, 40));
  poly.InsLast(Point2dC(40, 50));
  poly.InsLast(Point2dC(60, 50));
  poly.InsLast(Point2dC(60, 10));
  poly.InsLast(Point2dC(10, 10));

  for(ScanPolygon2dC it(poly, 1); it && (stop-- > 0); it++) {
#if DODISPLAY
    DrawLine(img,drawVal,Index2dC(it.Row(),it.Data().Min()),Index2dC(it.Row(),it.Data().Max()));
#endif
    //cerr << " " << it.Row() << " " << it.Data() << "\n";
#if 1
    if(it.Data().Size() > 0.001 &&
      !poly.Contains(Point2dC(it.Row(), it.Data().Center()))) {
#if DODISPLAY
      Save("@X:12",img);
#endif
      return __LINE__;
    }
#endif
    count++;
  }
#if DODISPLAY
  Save("@X:12",img);
#endif
  if(stop <= 0) return __LINE__;
  return 0;
}

int
testOverlap()
{
  cerr << "testOverlap, Called. \n";
  Polygon2dC poly;
  poly.InsLast(Point2dC(0, 0));
  poly.InsLast(Point2dC(0, 10));
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(10, 0));

  RealT score = poly.Overlap(poly);
  if(Abs(score - 1.0) > 0.000001) return __LINE__;

  poly.Reverse();

  score = poly.Overlap(poly);
  if(Abs(score - 1.0) > 0.000001) return __LINE__;

  Polygon2dC poly2 = poly.Copy();
  poly2 += Point2dC(100, 100);

  score = poly.Overlap(poly2);
  if(Abs(score) > 0.000001) return __LINE__;

  score = poly2.Overlap(poly);
  if(Abs(score) > 0.000001) return __LINE__;

  return 0;
}

TEST_CASE("Clip Polygon")
{
  Polygon2dC poly;
  poly.InsLast(Point2dC(-10, 0));
  poly.InsLast(Point2dC(0, 20));
  poly.InsLast(Point2dC(10, 10));
  poly.InsLast(Point2dC(10, 0));

  RealRange2dC range1(10, 10);
  RealRange2dC range2(10, 15);
  Polygon2dC clippedConvex = poly.ClipByConvex(Polygon2dC(range1));
  Polygon2dC clippedRange = poly.ClipByRange(range1);

  RealT score = Abs(clippedConvex.Area());
  if(Abs(score - 100) > 1e-6) return __LINE__;

  score = Abs(clippedRange.Area());
  if(Abs(score - 100) > 1e-6) return __LINE__;

  score = clippedConvex.Overlap(clippedRange);
  if(Abs(score - 1) > 1e-6) return __LINE__;

  clippedConvex = poly.ClipByConvex(Polygon2dC(range2));
  clippedRange = poly.ClipByRange(range2);

  score = clippedConvex.Overlap(clippedRange);
  if(Abs(score - 1) > 1e-6) return __LINE__;

}

#endif