// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Geometry/Euclidean/2D/testLine2d.cc"
//! docentry="Ravl.API.Math.Geometry.2D"
//! author="Charles Galambos"
//! userlevel=Develop

#include "Ravl/Line2dIter.hh"
#include "Ravl/StdMath.hh"
#include "Ravl/Stream.hh"
#include "Ravl/Moments2d2.hh"
#include "Ravl/Curve2dLineSegment.hh"
#include "Ravl/StdConst.hh"
#include "Ravl/LinePP2d.hh"
#include "Ravl/LineABC2d.hh"
#include "Ravl/RealRange2d.hh"
#include "Ravl/Array1d.hh"
#include "Ravl/Random.hh"

using namespace RavlN;

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

int testMoments();
int testLines();
int testLine2d();
int testClip2d();
int testLineFitLSQ();

int main() {
  int ln;
  if((ln = testLines()) != 0) {
    cerr << "Test failed line " << ln << "\n";
    return 1;
  }
  if((ln = testLine2d()) != 0) {
    cerr << "Test failed line " << ln << "\n";
    return 1;
  }
  if((ln = testClip2d()) != 0) {
    cerr << "Test failed line " << ln << "\n";
    return 1;
  }
  if((ln = testLineFitLSQ()) != 0) {
    cerr << "Test failed line " << ln << "\n";
    return 1;
  }
  cerr << "Test passed ok. \n";
  return 0;
}

int testLines() {  
  cout << "Checking basic lines. \n";
  Index2dC start(0,0);
  Index2dC end(10,5);
  int c = 0;
  ONDEBUG(cerr << "From " << start << " to " << end << "\n");
  for(Line2dIterC it(start,end);it && c < 20;it++,c++) {
    ONDEBUG(cerr << "Line " << it.Data() << "\n");
  }
  if(c > 19) return __LINE__;  
  for(RealT x = 0;x < RavlConstN::pi * 2; x += 0.1) {
    end = Index2dC(Round(Sin(x) * 10.0),Round(Cos(x) * 10.0));
    c = 0;
    ONDEBUG(cerr << "From " << start << " to " << end << "\n");
    for(Line2dIterC it(start,end);it && c < 20;it++,c++) {
      ONDEBUG(cerr << "Line " << it.Data() << "\n");
    }
    if(c > 11) return __LINE__;
  }

  LinePP2dC line(Point2dC(0.3,0.7), Point2dC(10.3,15.7));
  c = 0;
  for(Line2dIterC it(line);it && c < 20;it++,c++) {
    ONDEBUG(cerr << "Line " << it.Data() << "\n");
    cout  << "Line " << it.Data() << "\n";
  }
  if(c > 16) return __LINE__;  

  return 0;
}

int testLine2d() {
  cout << "Checking misc line methods. \n";
  Point2dC org(321,123);
  for(int i = 0;i < 359;i++) {
    RealT angle = ((RealT) i/180.0) * RavlConstN::pi;
    Vector2dC vec = Angle2Vector2d(angle) * 4.3;
    //cerr << "Vec=" << vec << "\n";
    Point2dC end(org + vec);
    Curve2dLineSegmentC line1(org,end);
    //cerr << "End=" << line1.Closest(line1.EndPnt()) - line1.Start()  << "\n";
    if(line1.Closest(line1.EndPnt()) - line1.Start() <= 0) return __LINE__;
    
    LinePP2dC linepp(org,end);
    //cerr << "Angle=" << linepp.Angle() << " Angle2=" << angle << "\n";
    if(i < 90) 
      if(Abs(linepp.Angle() - angle) > 0.001) return __LINE__;
  }
  
  LinePP2dC line1(Point2dC(0,0),Point2dC(1,1));
  if(!line1.IsPointToRight(Point2dC(1,0))) return __LINE__;
  if(line1.IsPointToRight(Point2dC(0,1))) return __LINE__;
  
  LinePP2dC line2(Point2dC(0,1),Point2dC(1,0));
  if(!line2.IsPointToRight(Point2dC(0,0))) return __LINE__;
  if(line2.IsPointToRight(Point2dC(1,1))) return __LINE__;
  
  RealT cres;
  if(!line1.IntersectRow(0.5,cres)) return __LINE__;
  if(Abs(cres - 0.5) > 0.000001) return __LINE__;
  
  if(!line2.IntersectRow(0.5,cres)) return __LINE__;
  if(Abs(cres - 0.5) > 0.000001) return __LINE__;
  
  if(!line2.IntersectRow(0.2,cres)) return __LINE__;
  if(Abs(cres - 0.8) > 0.000001) return __LINE__;
  
  RealT val = line2.ParIntersection(line1);
  cerr << "Val=" << val << "\n";
  if(Abs(val - 0.5) > 0.0001) return __LINE__;

  // check that exception gets thrown for zero-length line
  LinePP2dC line3(Point2dC(0,0),Point2dC(0,0));
  try {
    RealT d = line3.DistanceWithin(Point2dC(1,1));
    cerr << "Failed to throw exception on zero length line\n";
    return __LINE__;
  }
  catch (...) {
  }
  return 0;
}

int testClip2d() {
  cout << "Checking clipping. \n";
  RealRange2dC rng(0,15,10,25);
  Point2dC pnt0(0,0);
  Point2dC pnt1(5,15);
  Point2dC pnt2(10,20);
  Point2dC pnt3(35,30);
  Point2dC pnt4(40,45);
  Point2dC pnt5(5,30);
  Point2dC pnt6(7.5,25);

  // This line shouldn't be clipped.
  LinePP2dC l1(pnt1,pnt2);
  if(!l1.ClipBy(rng)) return __LINE__;
  if((pnt1 - l1.P1()).SumOfSqr() > 0.00001) return __LINE__;
  if((pnt2 - l1.P2()).SumOfSqr() > 0.00001) return __LINE__;

  // Should clip point 0.
  LinePP2dC l2(pnt0,pnt2);
  if(!l2.ClipBy(rng)) return __LINE__;
  if((pnt0 - l2.P1()).SumOfSqr() < 0.00001) return __LINE__;
  if((pnt2 - l2.P2()).SumOfSqr() > 0.00001) return __LINE__;
  if(!rng.Contains(l2.P1())) return __LINE__;

  // Should clip point 1.
  LinePP2dC l3(pnt1,pnt3);
  if(!l3.ClipBy(rng)) return __LINE__;
  if((pnt1 - l3.P1()).SumOfSqr() > 0.00001) return __LINE__;
  if((pnt3 - l3.P2()).SumOfSqr() < 0.00001) return __LINE__;
  if(!rng.Contains(l3.P2())) return __LINE__;
  
  // Line entirely outside region.
  LinePP2dC l4(pnt3,pnt4);
  if(l4.ClipBy(rng)) return __LINE__;

  RealRange2dC rng2(0.51,239.49,0.51,239.49);
  LinePP2dC l5(Point2dC(1.5,0.5),Point2dC(1.5,720.5));
  //cerr << " CP1=" << l5.P1() << " CP2=" << l5.P2() << "\n";
  if(!l5.ClipBy(rng2)) return __LINE__;
  //cerr << " CP1=" << l5.P1() << " CP2=" << l5.P2() << "\n";
  if(!rng2.Contains(l5.P1())) return __LINE__;
  if(!rng2.Contains(l5.P2())) return __LINE__;

  // Test for bug now corrected - should clip point 2
  LinePP2dC l6(pnt2,pnt5);
  if(!l6.ClipBy(rng)) return __LINE__;
  //cerr << pnt6 << " " << l6.P2() << endl;
  if((pnt6 - l6.P2()).SumOfSqr() > 0.00001) return __LINE__;

  return 0;
}

int testLineFitLSQ() {
  
  Array1dC<Point2dC> points(10);
  UIntT i;
  RealT res,twoPi = RavlConstN::pi * 2;
  for(RealT a = 0;a < twoPi;a += (twoPi/100)) {
    RealT offx = Random1() * 100 - 50;
    RealT offy = Random1() * 100 - 50;
    RealT dx = Cos(a) * 50;
    RealT dy = Sin(a) * 50;
    //cerr << "Dx=" << dx << " Dy=" << dy << "\n";
    for(i = 0;i < points.Size();i++)
      points[i] = Point2dC(i * dx + offx + RandomGauss(), i * dy + offy + RandomGauss());
    LineABC2dC line;
    line.FitLSQ(points,res);
    //cerr << "Line=" << line << " Res=" << res <<"\n";
    for(i = 0;i < points.Size();i++) {
      RealT dist = line.Distance(points[i]);
      Point2dC at = line.Projection(points[i]);
      RealT sep = at.EuclidDistance(points[i]);
      if(Abs(sep - dist) > 0.0001) return __LINE__;
      //cerr << "Dist=" << dist << "\n";
      if(dist > 5) return __LINE__;
    }
  }

  return 0;
}
