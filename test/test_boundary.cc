// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Image/Segmentation/CrackCode.hh"
#include "Ravl2/Image/Segmentation/Crack.hh"
#include "Ravl2/Image/Segmentation/Boundary.hh"
#include "Ravl2/Array.hh"

#define DODEBUG	0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define CHECK_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a,b) CHECK((a) == (b))
#define EXPECT_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a,b) REQUIRE(Ravl2::isNearZero((a) -(b)))


TEST_CASE("CrackCode following")
{
  using namespace Ravl2;

  CrackCode xcc(CrackCodeT::CR_UP);
  CrackCode ycc(CrackCodeT::CR_RIGHT);
  RelativeCrackCodeT rcc = xcc.relative(ycc);
  EXPECT_EQ(rcc, RelativeCrackCodeT::CR_TO_RIGHT);
  
  for(int i = 0;i < 4;i++) {
    CrackCode cc(i);
    int ip = i + 1;
    if(ip >= 4)
      ip -= 4;
    rcc = cc.relative(cc);
    EXPECT_EQ(rcc, RelativeCrackCodeT::CR_AHEAD);
    CrackCode ipcc(ip);
    rcc = cc.relative(ipcc);
    //std::cerr <<"CodeLeft=" << (int)rcc << "\n";
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_TO_LEFT);
    rcc = ipcc.relative(cc);
    //std::cerr <<"CodeRight=" << (int)rcc << "\n";
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_TO_RIGHT);
    
    int ipp = i + 2;
    if(ipp >= 4)
      ipp -= 4;;
    CrackCode ippcc(ipp);
    rcc = ippcc.relative(cc);
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_BACK);
    rcc = cc.relative(ippcc);
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_BACK);
  }
}

TEST_CASE("CrackCode relative directions")
{
  using namespace Ravl2;

  Index<2> start(5,6);
  CrackC edge(start,CrackCodeT::CR_DOWN);
  Index<2> at = edge.rightPixel();
  ONDEBUG(std::cerr <<"iAt=" << at << " Start=" << start << "\n");
  EXPECT_EQ(at, (start + Index<2>(0,-1)));
  EXPECT_EQ(start, edge.leftPixel());
  // Go around a pixel clockwise.
  for(int i = 0;i < 5;i++) {
    edge.vertexBegin() = edge.vertexEnd();
    edge.turnClock();
    ONDEBUG(std::cerr << "At=" << edge.rightPixel() << " code:" << edge.Code() << "\n");
    EXPECT_NE(at, edge.leftPixel());
  }
  // Go around a pixel counter clockwise.
  edge = CrackC(start,CrackCodeT::CR_DOWN);
  at = edge.leftPixel();
  EXPECT_EQ(at,start)
  ONDEBUG(std::cerr <<"iAt=" << at << "\n");
  for(int i = 0;i < 5;i++) {
    edge.vertexBegin() = edge.vertexEnd();
    edge.turnCClock();
    ONDEBUG(std::cerr << "At=" << edge.leftPixel() << " code:" << edge.Code() << "\n");
    EXPECT_EQ(at, edge.leftPixel());
  }
  //                               DOWN          RIGHT              UP                LEFT               None
  Index<2> offsets[5] = { Index<2>(0,-1),Index<2>(1,0),Index<2>(0,1),Index<2>(-1,0),Index<2>(0,0) };
  for(int i = 0;i < 5;i++) {
    edge = CrackC::fromPixel(start, CrackCode(i));
    //std::cerr <<" " << i << " RPixel=" << edge.RPixel() << "\n";
    EXPECT_EQ(edge.leftPixel(), start);
    EXPECT_EQ(edge.rightPixel(), (start + offsets[i]));
  }
}


TEST_CASE("CrackCode properties")
{
  using namespace Ravl2;

  SECTION("Bounding box and areas")
  {
    IndexRange<2> rect(IndexRange<1>({1, 3}), IndexRange<1>({2, 4}));
    Boundary bnd = toBoundary(rect, BoundaryOrientationT::INSIDE_LEFT);
    //SPDLOG_INFO("Rect: {}  Bounds:{} ", rect, bnd);

    // Check the edges are closed and ordered
    {
      auto prevCode = bnd.edges().back();
      for(auto it : bnd.edges()) {
	//SPDLOG_INFO("Crack: {} ", it);
	CHECK(crackStep(prevCode.at(), prevCode.crackCode()) == it.at());
	prevCode = it;
      }
    }

    //cout << "Bounds:\n " << bnd << "\n";
    CHECK(bnd.size() == 12);
    ONDEBUG(std::cout << "Area=" << bnd.area() << "\n");
    CHECK(bnd.area() == rect.area());
    IndexRange<2> tmpbb = bnd.boundingBox();
    EXPECT_EQ(tmpbb, rect);
    bnd.BReverse();
    EXPECT_EQ(tmpbb, bnd.boundingBox());
    EXPECT_EQ(bnd.area(), rect.area());
    bnd.invert();
    ONDEBUG(cout << "RArea=" << bnd.area() << "\n");
    CHECK(-bnd.area() == rect.area());

    IndexRange<2> bb = bnd.boundingBox();
    ONDEBUG(std::cerr << "Bounding box=" << bb << " Inv=" << tmpbb << "\n");
    CHECK(bb == rect.expand(1));
  }
  SECTION("Edge mid point")
  {
    for(int i =0;i < 5;i++) {
      BoundaryVertex start({5, 5});
      CrackC edge(start, CrackCode(i));
      auto m1 = ((toPoint<float>(edge.rightPixel()) + toPoint<float>(edge.leftPixel())) / 2.0f) + Point<float,2>({0.5, 0.5});
      CHECK(isNearZero<float>(euclidDistance(m1,edge.MidPoint<float>()),1e-5f));
    }
  }
  SECTION("Tracing a labeled region")
  {
    using PixelT = int;
    Array<PixelT,2> img({10,10}, 99);

    // Setup a square in the middle of the image.
    auto rng = img.range().shrink(2);
    Ravl2::fill(clip(img,rng),10);
    SPDLOG_INFO("Image: {}", img);

    auto bounds = Boundary::traceBoundary(img, 10);

    for(auto it : bounds.edges()) {
      //SPDLOG_INFO("Edge: {}  Left:{} ", it,it.LPixel());
      CHECK(img[it.leftPixel()] == 10);
      CHECK(img[it.rightPixel()] != 10);
    }

    //SPDLOG_INFO("Bounds Lengths: {}  ({})", bounds.size(), size_t((rng.range(0).size() + rng.range(1).size()) * 2));
    //SPDLOG_INFO("Bounds: {} ", bounds);

    // Check the boundary is the same as the image.
    CHECK(bounds.size() == size_t((rng.range(0).size() + rng.range(1).size()) * 2));
    CHECK(bounds.area() == rng.area());
  }
}


TEST_CASE("Boundary order edges")
{
  using namespace Ravl2;

  Array<int,2> emask({5,5}, 0);
  
  emask[1][1] = 1;
  emask[1][2] = 1;
  emask[1][3] = 1;
  emask[2][1] = 1;
  emask[3][1] = 1;
  emask[2][3] = 1;
  emask[3][2] = 1;
  
  Boundary bnds = Boundary::traceBoundary(emask,1);
  auto lst = bnds.orderEdges();
  
  // std::cerr <<"Lst.size()=" << lst.size() << "\n";
  // std::cerr <<"Lst.First().size()=" << lst.First() << "\n";
  // std::cerr <<"Lst.Last() =" << lst.Last() << "\n";
  
  CHECK(lst.size() == 2);
  CHECK((lst.front().size() + lst.back().size()) == 16);
  
  // Check it's not a fluke, try a different orientation.
  
  fill(emask, 0);
  emask[1][2] = 1;
  emask[1][3] = 1;
  emask[2][1] = 1;
  emask[3][1] = 1;
  emask[2][3] = 1;
  emask[3][2] = 1;
  emask[3][3] = 1;
  
  Boundary bnds2 = Boundary::traceBoundary(emask,1);
  lst = bnds2.orderEdges();
  CHECK(lst.size() == 2);
  CHECK((lst.front().size() + lst.back().size()) == 16);

  {
    std::vector<CrackC> bnds3;
    bnds3.push_back(CrackC(BoundaryVertex(2, 2), CrackCodeT::CR_RIGHT));
    bnds3.push_back(CrackC(BoundaryVertex(2, 3), CrackCodeT::CR_DOWN));
    bnds3.push_back(CrackC(BoundaryVertex(3, 3), CrackCodeT::CR_LEFT));
    lst = Boundary(bnds).orderEdges();
    REQUIRE(lst.size() == 1);
    CHECK(lst.front().size() == 3);
    // std::cerr <<"Lst.size()=" << lst.size() << "\n";
    // std::cerr <<"Lst.First().size()=" << lst.First() << "\n";
  }

  {
    std::vector<CrackC> bnds4;
    bnds4.push_back(CrackC(BoundaryVertex(2, 2), CrackCodeT::CR_RIGHT));
    //bnds4.push_back(CrackC(BoundaryVertex(2,3),0));
    bnds4.push_back(CrackC(BoundaryVertex(3, 3), CrackCodeT::CR_LEFT));
    bnds4.push_back(CrackC(BoundaryVertex(3, 2), CrackCodeT::CR_UP));
    lst = Boundary(bnds4).orderEdges();
    REQUIRE(lst.size() == 1);
    CHECK(lst.front().size() == 3);
    //std::cerr <<"Lst.size()=" << lst.size() << "\n";
    //std::cerr <<"Lst.First().size()=" << lst.First() << "\n";
  }

}
