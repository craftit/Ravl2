// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Math.Geometry.2D"
//! author="Charles Galambos"

#include <numbers>
#include <catch2/catch_test_macros.hpp>
#include <cereal/archives/json.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/Geometry/Line2dIter.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/LinePP.hh"
#include "Ravl2/Geometry/LineABC2d.hh"
#include "Ravl2/Geometry/Range.hh"


#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

TEST_CASE("Line2dIter")
{
  using namespace Ravl2;
  using RealT = float;
  Index<2> start(0, 0);
  Index<2> end(10, 5);
  auto rng = IndexRange<2>(start,start).expand(11);

  int c = 0;
  for (Line2dIterC it(start, end); it.valid() && c < 20; it++, c++) {
    CHECK(rng.contains(it.Data()));
  }
  CHECK (c <= 19);
  RealT x = 0;
  RealT step = 0.1f;
  int countTo = int_round((std::numbers::pi_v<RealT> * 2)/step);
  for (int i = 0; i < countTo; ++i) {
    end = Index<2>(int_round(std::sin(x) * 10.0f), int_round(std::cos(x) * 10.0f));
    c = 0;
    for (Line2dIterC it(start, end); it.valid() && c < 20; it++, c++) {
      CHECK(rng.contains(it.Data()));
    }
    CHECK (c <= 11);
    x += step;
  }

  rng = IndexRange<2>(start,start).expand(20);

  LinePP2dC<RealT> line(toPoint<RealT>(0.3, 0.7), toPoint<RealT>(10.3, 15.7));
  c = 0;
  for (Line2dIterC it(line);it.valid() && c < 20; it++, c++) {
    CHECK(rng.contains(it.Data()));
  }
  CHECK (c <= 16);
}


TEST_CASE("LinePP")
{
  using namespace Ravl2;
  using RealT = float;

#if 0
  Point<RealT,2> org = toPoint<RealT>(321,123);
  for(int i = 0;i < 359;i++) {
    RealT angle = ((RealT) i/180.0) * std::numbers::pi_v<RealT>;
    Vector<RealT,2> vec = Angle2Vector2d(angle) * 4.3;
    //cerr << "Vec=" << vec << "\n";
    Point<RealT,2> end(org + vec);
    Curve2dLineSegmentC line1(org,end);
    //cerr << "End=" << line1.Closest(line1.EndPnt()) - line1.Start()  << "\n";
    CHECK_FALSE(line1.Closest(line1.EndPnt()) - line1.Start() <= 0) return __LINE__;
    
    LinePP2dC linepp(org,end);
    //cerr << "Angle=" << linepp.Angle() << " Angle2=" << angle << "\n";
    CHECK_FALSE(i < 90)
    CHECK_FALSE(std::abs(linepp.Angle() - angle) > 0.001) return __LINE__;
  }
#endif
  SECTION("Orientation and intersection")
  {
    LinePP2dC line1(toPoint<RealT>(0, 0), toPoint<RealT>(1, 1));
    CHECK(line1.IsPointToRight(toPoint<RealT>(1, 0)));
    CHECK_FALSE(line1.IsPointToRight(toPoint<RealT>(0, 1)));

    LinePP2dC line2(toPoint<RealT>(0, 1), toPoint<RealT>(1, 0));
    CHECK(line2.IsPointToRight(toPoint<RealT>(0, 0)));
    CHECK_FALSE(line2.IsPointToRight(toPoint<RealT>(1, 1)));

    RealT cres;
    CHECK(line1.IntersectRow(0.5f, cres));
    CHECK_FALSE(std::abs(cres - 0.5f) > 0.000001f);

    CHECK(line2.IntersectRow(0.5f, cres));
    CHECK_FALSE(std::abs(cres - 0.5f) > 0.000001f);

    CHECK(line2.IntersectRow(0.2f, cres));
    CHECK_FALSE(std::abs(cres - 0.8f) > 0.000001f);

    RealT val = line2.ParIntersection(line1);
    CHECK_FALSE(std::abs(val - 0.5f) > 0.0001f);

    // check that exception gets thrown for zero-length line
    LinePP2dC line3(toPoint<RealT>(0, 0), toPoint<RealT>(0, 0));
    RealT d = 0;
    CHECK_THROWS(d = line3.DistanceWithin(toPoint<RealT>(1, 1)));
    CHECK(d == 0);
  }
  SECTION("Clipping")
  {
    Range<float,2> rng({{0,15},{10,25}});
    Point<RealT,2> pnt0 = toPoint<RealT>(0,0);
    Point<RealT,2> pnt1 = toPoint<RealT>(5,15);
    Point<RealT,2> pnt2 = toPoint<RealT>(10,20);
    Point<RealT,2> pnt3 = toPoint<RealT>(35,30);
    Point<RealT,2> pnt4 = toPoint<RealT>(40,45);
    Point<RealT,2> pnt5 = toPoint<RealT>(5,30);
    Point<RealT,2> pnt6 = toPoint<RealT>(7.5,25);

    // This line shouldn't be clipped.
    LinePP2dC l1(pnt1,pnt2);
    CHECK(l1.clipBy(rng));
    CHECK_FALSE(sumOfSqr(pnt1 - l1.P1())() > 0.00001f);
    CHECK_FALSE(sumOfSqr(pnt2 - l1.P2())() > 0.00001f);

    // Should clip point 0.
    LinePP2dC l2(pnt0,pnt2);
    CHECK(l2.clipBy(rng));
    CHECK_FALSE(sumOfSqr(pnt0 - l2.P1())() < 0.00001f);
    CHECK_FALSE(sumOfSqr(pnt2 - l2.P2())() > 0.00001f);
    CHECK(rng.contains(l2.P1()));

    // Should clip point 1.
    LinePP2dC l3(pnt1,pnt3);
    CHECK(l3.clipBy(rng));
    CHECK_FALSE(sumOfSqr(pnt1 - l3.P1())() > 0.00001f);
    CHECK_FALSE(sumOfSqr(pnt3 - l3.P2())() < 0.00001f);
    CHECK(rng.contains(l3.P2()));

    // Line entirely outside region.
    LinePP2dC l4(pnt3,pnt4);
    CHECK_FALSE(l4.clipBy(rng));

    Range<float,2> rng2({{0.51f,239.49f},{0.51f,239.49f}});
    LinePP2dC l5(toPoint<RealT>(1.5,0.5),toPoint<RealT>(1.5,720.5));
    CHECK(l5.clipBy(rng2));
    CHECK(rng2.contains(l5.P1()));
    CHECK(rng2.contains(l5.P2()));

    // Test for bug now corrected - should clip point 2
    LinePP2dC l6(pnt2,pnt5);
    CHECK(l6.clipBy(rng));
    CHECK_FALSE((pnt6 - l6.P2())() > 0.00001f);
  }

}

#if 0

int testLineFitLSQ() {
  
  Array<Point<RealT,2>,1> points(10);
  unsigned i;
  RealT res,twoPi = std::numbers::pi_v<RealT> * 2;
  for(RealT a = 0;a < twoPi;a += (twoPi/100)) {
    RealT offx = Random1() * 100 - 50;
    RealT offy = Random1() * 100 - 50;
    RealT dx = std::cos(a) * 50;
    RealT dy = std::sin(a) * 50;
    //cerr << "Dx=" << dx << " Dy=" << dy << "\n";
    for(i = 0;i < points.size();i++)
      points[i] = toPoint<RealT>(i * dx + offx + RandomGauss(), i * dy + offy + RandomGauss());
    LineABC2dC line;
    line.FitLSQ(points,res);
    //cerr << "Line=" << line << " Res=" << res <<"\n";
    for(i = 0;i < points.size();i++) {
      RealT dist = line.Distance(points[i]);
      Point<RealT,2> at = line.Projection(points[i]);
      RealT sep = at.EuclidDistance(points[i]);
      CHECK_FALSE(std::abs(sep - dist) > 0.0001);
      //cerr << "Dist=" << dist << "\n";
      CHECK_FALSE(dist > 5);
    }
  }

  return 0;
}
#endif