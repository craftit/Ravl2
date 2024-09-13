// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Math.Geometry.2D"
//! author="Charles Galambos"

#include <numbers>
#include <random>
#include "checks.hh"
#include <cereal/archives/json.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/Geometry/Line2dIter.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Geometry/LinePP.hh"
#include "Ravl2/Geometry/LineABC2d.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/FitLine.hh"


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
  int countTo = intRound((std::numbers::pi_v<RealT> * 2) / step);
  for (int i = 0; i < countTo; ++i) {
    end = Index<2>(intRound(std::sin(x) * 10.0f), intRound(std::cos(x) * 10.0f));
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
    CHECK(std::abs(val - 0.5f) < 0.0001f);

    auto pnt = line2.innerIntersection(line1);
    CHECK(pnt.has_value());
    CHECK(squaredEuclidDistance(pnt.value(),toPoint<RealT>(0.5, 0.5)) < 0.0001f);

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
    CHECK_FALSE(sumOfSqr(pnt1 - l1.P1()) > 0.00001f);
    CHECK_FALSE(sumOfSqr(pnt2 - l1.P2()) > 0.00001f);

    // Should clip point 0.
    LinePP2dC l2(pnt0,pnt2);
    CHECK(l2.clipBy(rng));
    CHECK_FALSE(sumOfSqr(pnt0 - l2.P1()) < 0.00001f);
    CHECK_FALSE(sumOfSqr(pnt2 - l2.P2()) > 0.00001f);
    CHECK(rng.contains(l2.P1()));

    // Should clip point 1.
    LinePP2dC l3(pnt1,pnt3);
    CHECK(l3.clipBy(rng));
    CHECK_FALSE(sumOfSqr(pnt1 - l3.P1()) > 0.00001f);
    CHECK_FALSE(sumOfSqr(pnt3 - l3.P2()) < 0.00001f);
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


TEST_CASE("Fit LineABC")
{
  using namespace Ravl2;
  using RealT = float;
  std::vector<Point<RealT,2>> points;
  points.reserve(10);
  RealT twoPi = std::numbers::pi_v<RealT> * 2;

  //std::random_device dev;
  std::mt19937 rng(static_cast<std::mt19937::result_type>(random()));
  std::uniform_real_distribution<RealT> random50(-50.0, 50.0);
  std::normal_distribution<RealT> randomGauss(0.0, 1.0);

  RealT a = 0;
  RealT step = (twoPi/100);
  const auto iterCount = size_t(intRound(twoPi / step));
  for(size_t j = 0;j < iterCount;++j) {
    points.clear();
    RealT offx = random50(rng);
    RealT offy = random50(rng);
    RealT dx = std::cos(a) * 50;
    RealT dy = std::sin(a) * 50;
    //cerr << "Dx=" << dx << " Dy=" << dy << "\n";
    for(unsigned i = 0;i <10;++i)
      points.push_back(toPoint<RealT>(RealT(i) * dx + offx + randomGauss(rng), RealT(i) * dy + offy + randomGauss(rng)));

    LineABC2dC<RealT> line;
    auto res = fit(line, points);
    CHECK(res >= 0);
    CHECK(res < 5);
    //cerr << "Line=" << line << " Res=" << res <<"\n";
    for(unsigned i = 0;i < points.size();++i) {
      RealT dist = line.Distance(points[i]);
      Point<RealT,2> at = line.Projection(points[i]);
      RealT sep = euclidDistance(at, points[i]);
      CHECK(std::abs(sep - dist) < 0.0001f);
      //cerr << "Dist=" << dist << "\n";
      CHECK(dist < 5);
    }
    a += step;
  }
}
