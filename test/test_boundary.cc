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


TEST_CASE("Follow some codes", "[CrackCodeC]")
{
  using namespace Ravl2;

  CrackCodeC xcc(CrackCodeT::CR_UP);
  CrackCodeC ycc(CrackCodeT::CR_RIGHT);
  RelativeCrackCodeT rcc = xcc.Relative(ycc);
  EXPECT_EQ(rcc, RelativeCrackCodeT::CR_TO_RIGHT);
  
  for(int i = 0;i < 4;i++) {
    CrackCodeC cc(i);
    int ip = i + 1;
    if(ip >= 4)
      ip -= 4;
    rcc = cc.Relative(cc);
    EXPECT_EQ(rcc, RelativeCrackCodeT::CR_AHEAD);
    CrackCodeC ipcc(ip);
    rcc = cc.Relative(ipcc);
    //std::cerr <<"CodeLeft=" << (int)rcc << "\n";
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_TO_LEFT);
    rcc = ipcc.Relative(cc);
    //std::cerr <<"CodeRight=" << (int)rcc << "\n";
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_TO_RIGHT);
    
    int ipp = i + 2;
    if(ipp >= 4)
      ipp -= 4;;
    CrackCodeC ippcc(ipp);
    rcc = ippcc.Relative(cc);
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_BACK);
    rcc = cc.Relative(ippcc);
    EXPECT_EQ(rcc,RelativeCrackCodeT::CR_BACK);
  }
}

TEST_CASE("Check relative directions", "[CrackCodeC]")
{
  using namespace Ravl2;

  Index<2> start(5,6);
  CrackC edge(start,CrackCodeT::CR_DOWN);
  Index<2> at = edge.RPixel();
  ONDEBUG(std::cerr <<"iAt=" << at << " Start=" << start << "\n");
  EXPECT_EQ(at, (start + Index<2>(0,-1)));
  EXPECT_EQ(start, edge.LPixel());
  // Go around a pixel clockwise.
  for(int i = 0;i < 5;i++) {
    edge.Begin() = edge.End();
    edge.TurnClock();
    ONDEBUG(std::cerr <<"At=" << edge.RPixel() << " Code:" << edge.Code() << "\n");
    EXPECT_NE(at, edge.LPixel());
  }
  // Go around a pixel counter clockwise.
  edge = CrackC(start,CrackCodeT::CR_DOWN);
  at = edge.LPixel();
  EXPECT_EQ(at,start)
  ONDEBUG(std::cerr <<"iAt=" << at << "\n");
  for(int i = 0;i < 5;i++) {
    edge.Begin() = edge.End();
    edge.TurnCClock();
    ONDEBUG(std::cerr <<"At=" << edge.LPixel() << " Code:" << edge.Code() << "\n");
    EXPECT_EQ(at, edge.LPixel());
  }
  //                               DOWN          RIGHT              UP                LEFT               None
  Index<2> offsets[5] = { Index<2>(0,-1),Index<2>(1,0),Index<2>(0,1),Index<2>(-1,0),Index<2>(0,0) };
  for(int i = 0;i < 5;i++) {
    edge = CrackC::fromPixel(start,CrackCodeC(i));
    //std::cerr <<" " << i << " RPixel=" << edge.RPixel() << "\n";
    EXPECT_EQ(edge.LPixel(), start);
    EXPECT_EQ(edge.RPixel(), (start + offsets[i]));
  }
}


TEST_CASE("Check things are working properly", "[Boundary]")
{
  using namespace Ravl2;

  SECTION("Bounding box and areas")
  {
    IndexRange<2> rect(IndexRange<1>({1, 3}), IndexRange<1>({2, 4}));
    BoundaryC bnd = toBoundary(rect, BoundaryTypeT::OUTSIDE);
    SPDLOG_INFO("Rect: {}  Bounds:{} ", rect, bnd);

    // Check the edges are closed and ordered
    {
      auto prevCode = bnd.edges().back();
      for(auto it : bnd.edges()) {
	//SPDLOG_INFO("Crack: {} ", it);
	CHECK(CrackStep(prevCode.at(), prevCode.crackCode()) == it.at());
	prevCode = it;
      }
    }

    //cout << "Bounds:\n " << bnd << "\n";
    CHECK(bnd.size() == 12);
    ONDEBUG(std::cout << "Area=" << bnd.area() << "\n");
    CHECK(bnd.area() == rect.area());
    IndexRange<2> tmpbb = bnd.BoundingBox();
    EXPECT_EQ(tmpbb, rect);
    bnd.BReverse();
    EXPECT_EQ(tmpbb, bnd.BoundingBox());
    EXPECT_EQ(bnd.area(), rect.area());
    bnd.Invert();
    ONDEBUG(cout << "RArea=" << bnd.area() << "\n");
    CHECK(-bnd.area() == rect.area());

    IndexRange<2> bb = bnd.BoundingBox();
    ONDEBUG(std::cerr << "Bounding box=" << bb << " Inv=" << tmpbb << "\n");
    CHECK(bb == rect.expand(1));
  }
  SECTION("Edge mid point")
  {
    for(int i =0;i < 5;i++) {
      BVertexC start({5,5});
      CrackC edge(start,CrackCodeC(i));
      auto m1 = ((toPoint<float>(edge.RPixel()) + toPoint<float>(edge.LPixel())) / 2.0f) + Point<float,2>({0.5,0.5});
      CHECK(isNearZero<float>(euclidDistance(m1,edge.MidPoint<float>())(),1e-5f));
    }
  }
  SECTION("Tracing a labeled region")
  {
    using PixelT = int;
    Array<PixelT,2> img({10,10});
    img.fill(99);

    // Setup a square in the middle of the image.
    auto rng = img.range().shrink(2);
    clip(img,rng).fill(10);
    SPDLOG_INFO("Image: {}", img);

    auto bounds = BoundaryC::traceBoundary(img,10);
    SPDLOG_INFO("Bounds Lengths: {}  ({})", bounds.size(), size_t((rng.range(0).size() + rng.range(1).size()) * 2));
    SPDLOG_INFO("Bounds: {} ", bounds);

    // Check the boundary is the same as the image.
    CHECK(bounds.size() == size_t((rng.range(0).size() + rng.range(1).size()) * 2));
    CHECK(bounds.area() == rng.area());
  }
}

#if 0

TEST(Image, OrderEdges)
{
  using namespace Ravl2;

  Array<int,2> emask({5,5});
  
  emask.fill(0);
  emask[1][1] = 1;
  emask[1][2] = 1;
  emask[1][3] = 1;
  emask[2][1] = 1;
  emask[3][1] = 1;
  emask[2][3] = 1;
  emask[3][2] = 1;
  
  BoundaryC bnds(emask,true);
  auto lst = bnds.OrderEdges();
  
  // std::cerr <<"Lst.size()=" << lst.size() << "\n";
  // std::cerr <<"Lst.First().size()=" << lst.First() << "\n";
  // std::cerr <<"Lst.Last() =" << lst.Last() << "\n";
  
  if(lst.size() != 2) return __LINE__;
  if((lst.First().size() + lst.Last().size()) != 16) return __LINE__;
  
  // Check its not a fluke, try a different orientation.
  
  emask.fill(0);
  emask[1][2] = 1;
  emask[1][3] = 1;
  emask[2][1] = 1;
  emask[3][1] = 1;
  emask[2][3] = 1;
  emask[3][2] = 1;
  emask[3][3] = 1;
  
  BoundaryC bnds2(emask,true);
  lst = bnds2.OrderEdges();
  if(lst.size() != 2) return __LINE__;
  if((lst.First().size() + lst.Last().size()) != 16) return __LINE__;
  
  BoundaryC bnds3;
  bnds3.push_back(CrackC(BVertexC(2,2),1));
  bnds3.push_back(CrackC(BVertexC(2,3),0));
  bnds3.push_back(CrackC(BVertexC(3,3),3));
  lst = bnds3.OrderEdges();
  if(lst.size() != 1) return __LINE__;
  if(lst.First().size() != 3) return __LINE__;
  // std::cerr <<"Lst.size()=" << lst.size() << "\n";
  // std::cerr <<"Lst.First().size()=" << lst.First() << "\n";

  BoundaryC bnds4;
  bnds4.push_back(CrackC(BVertexC(2,2),1));
  //bnds4.push_back(CrackC(BVertexC(2,3),0));
  bnds4.push_back(CrackC(BVertexC(3,3),3));
  bnds4.push_back(CrackC(BVertexC(3,2),2));
  lst = bnds4.OrderEdges();  
  if(lst.size() != 1) return __LINE__;
  if(lst.First().size() != 3) return __LINE__;
  //std::cerr <<"Lst.size()=" << lst.size() << "\n";
  //std::cerr <<"Lst.First().size()=" << lst.First() << "\n";
  
  return 0;
}
#endif