// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/LinePP2d.hh"

#define CTOP    0x1
#define CBOTTOM 0x2
#define CRIGHT  0x4
#define CLEFT   0x8

namespace Ravl2 
{

  template<typename RealT>
  static inline int ContainsCode(const Point<RealT,2> &pnt,const Range<RealT,2> &rng) {
    int ret = 0;
    if(pnt[0] > rng.BRow())
      ret |= CBOTTOM;
    else if(pnt[0] < rng.TRow())
      ret |= CTOP;
    if(pnt[1] > rng.RCol())
      ret |= CRIGHT;
    else if(pnt[1] < rng.LCol())
      ret |= CLEFT;
    return ret;
  }
  
  //: Clip line by given rectangle.
  // Returns false if no part of the line is in the rectangle.
  // Uses the Cohen and Sutherland line clipping algorithm.

  template<typename RealT>
  bool LinePP2dC<RealT>::ClipBy(const Range<RealT,2> &rng) {
    bool accept = false;
    int oc0 = ContainsCode(this->P1(),rng);
    int oc1 = ContainsCode(this->P2(),rng);
#if 0
    const RealT vscale = rng.Rows();
    const RealT hscale = rng.Cols();
    RealT diff = ;
    //if(IsSmall(diff,hscale)) // Avoid division by zero. 
    //  np[0] = 0;
    //else
#endif
    
    do {
      if(!(oc0 | oc1)) {
	accept = true;
	break;
      } 
      if(oc0 & oc1)
	break;
      Point<RealT,2> np;
      int oc = oc0 ? oc0 : oc1;
      if(oc & CTOP) {
	np[0] = rng.TRow();
	np[1] = this->P1()[1] + (this->P2()[1] - this->P1()[1]) * (rng.TRow() - this->P1()[0]) / (this->P2()[0] - this->P1()[0]);
      } else if(oc & CBOTTOM) {
	np[0] = rng.BRow();
	np[1] = this->P1()[1] + (this->P2()[1] - this->P1()[1]) * (rng.BRow() - this->P1()[0]) / (this->P2()[0] - this->P1()[0]);
      } else if(oc & CRIGHT) {
	np[0] = this->P1()[0] + (this->P2()[0] - this->P1()[0]) * (rng.RCol() - this->P1()[1]) / (this->P2()[1] - this->P1()[1]);
	np[1] = rng.RCol();
      } else { // CLEFT
	np[0] = this->P1()[0] + (this->P2()[0] - this->P1()[0]) * (rng.LCol() - this->P1()[1]) / (this->P2()[1] - this->P1()[1]);
	np[1] = rng.LCol();
      }
      if(oc == oc0) {
	this->P1() = np;
	oc0 = ContainsCode(this->P1(),rng);
      } else {
	this->P2() = np;
	oc1 = ContainsCode(this->P2(),rng);
      }
    } while(1) ;
    return accept;
  }

  template<typename RealT>
  bool LinePP2dC<RealT>::IsPointIn(const Point<RealT,2>& point) const {
    if ( ! IsPointOn(point))
      return false;

    // If ab not vertical, check betweenness on x; else on y.
    if ( this->P1()[0] != this->P2()[0] )
      return   ((this->P1()[0] <= point[0]) && (point[0] <= this->P2()[0]))
	||((this->P1()[0] >= point[0]) && (point[0] >= this->P2()[0]));
    else
      return   ((this->P1()[1] <= point[1]) && (point[1] <= this->P2()[1]))
	||((this->P1()[1] >= point[1]) && (point[1] >= this->P2()[1]));
  }

  template<typename RealT>
  Point<RealT,2> LinePP2dC<RealT>::Intersection(const LinePP2dC & l) const {
    Vector<RealT,2> n1(static_cast<Vector<RealT,2>>(this->Vector()).Perpendicular());
    Vector<RealT,2> n2(static_cast<Vector<RealT,2>>(l.Vector()).Perpendicular());
    RealT     d1  = - n1.Dot(this->FirstPoint());
    RealT     d2  = - n2.Dot(l.FirstPoint());
    RealT     det = n1.Cross(n2);
    if (IsAlmostZero(det))
      return Point<RealT,2>(0.0, 0.0);
    return Point<RealT,2>((n1[1]*d2 - n2[1]*d1)/det,
		    (n2[0]*d1 - n1[0]*d2)/det);
  }

  template<typename RealT>
  bool LinePP2dC<RealT>::Intersection(const LinePP2dC & l, Point<RealT,2> & here) const {
    Vector<RealT,2> n1(static_cast<Vector<RealT,2>>(this->Vector()).Perpendicular());
    Vector<RealT,2> n2(static_cast<Vector<RealT,2>>(l.Vector()).Perpendicular());
    RealT     det = n1.Cross(n2);
    if (IsAlmostZero(det))
      return false;
    RealT     d1  = - n1.Dot(this->FirstPoint());
    RealT     d2  = - n2.Dot(l.FirstPoint());
    here[0] = (n1[1]*d2 - n2[1]*d1)/det;
    here[1] = (n2[0]*d1 - n1[0]*d2)/det;
    return true;
  }
  
  //: Find the column position which itersects the given row.

  template<typename RealT>
  bool LinePP2dC<RealT>::IntersectRow(RealT row,RealT &col) const {
    Vector<RealT,2> dir = this->P2() - this->P1();
    row -= this->P1()[0];
    if(dir[0] == 0)
      return false;
    col = ((row * dir[1]) / dir[0]) + this->P1()[1];
    return true;
  }

  template<typename RealT>
  RealT LinePP2dC<RealT>::ParIntersection(const LinePP2dC & l) const {
    Vector<RealT,2> u2P(l.point[0][1] - l.point[1][1],l.point[1][0] - l.point[0][0]); // u2p = l.Vector().Perpendicular();
    return (l.FirstPoint()- this->FirstPoint()).Dot(u2P)/this->Vector().Dot(u2P);
  }

  template<typename RealT>
  bool LinePP2dC<RealT>::HasInnerIntersection(const LinePP2dC & l) const {
    RealT t = ParIntersection(l);
    return t >= 0 && t<=1;
  }

  template<typename RealT>
  RealT LinePP2dC<RealT>::DistanceWithin(const Point<RealT,2> & pt) const {
    RealT t = ParClosest(pt);
    if (t < 0.0) return this->P1().EuclideanDistance(pt);
    if (t > 1.0) return this->P2().EuclideanDistance(pt);
    return Distance(pt);
  }
   
}
