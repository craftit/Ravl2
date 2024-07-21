// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Geometry/Euclidean/2D/Polygon2d.cc"

#include "Ravl/Polygon2d.hh"
#include "Ravl/Moments2d2.hh"

namespace RavlN {

  Polygon2dC::Polygon2dC(const RealRange2dC &range) {
    InsLast(range.TopLeft());
    InsLast(range.TopRight());
    InsLast(range.BottomRight());
    InsLast(range.BottomLeft());
  }


  RealT Polygon2dC::Area() const {
    RealT sum = 0.0;
    if (!IsEmpty()) {
      DLIterC<Point2dC> pLast(*this);
      pLast.Last();

      for (DLIterC<Point2dC> ptr(*this); ptr != pLast; ptr++)
        sum += ptr.Data().X() * ptr.NextData().Y() - ptr.NextData().X() * ptr.Data().Y();
      // close the polygon
      sum += pLast.Data().X() * pLast.NextCrcData().Y() - pLast.NextCrcData().X() * pLast.Data().Y();
    }
    return sum * 0.5;
  }
    
  Point2dC Polygon2dC::Centroid() const {
    RealT x = 0.0;
    RealT y = 0.0;
    if (!IsEmpty()) {
      DLIterC<Point2dC> pLast(*this);
      pLast.Last();

      for (DLIterC<Point2dC> ptr(*this); ptr != pLast; ptr++) {
        RealT temp = ptr.Data().X() * ptr.NextData().Y() - ptr.NextData().X() * ptr.Data().Y();
        x += (ptr.Data().X() + ptr.NextData().X()) * temp;
        y += (ptr.Data().Y() + ptr.NextData().Y()) * temp;
      }
      // close the polygon
      RealT temp = pLast.Data().X() * pLast.NextCrcData().Y() - pLast.NextCrcData().X() * pLast.Data().Y();
      x += (pLast.Data().X() + pLast.NextCrcData().X()) * temp;
      y += (pLast.Data().Y() + pLast.NextCrcData().Y()) * temp;
    }
    RealT scale = 1.0 / (6.0 * Area());
    return Point2dC(x * scale, y * scale);
  }

  // see http://www9.in.tum.de/forschung/fgbv/tech-reports/1996/FGBV-96-04-Steger.pdf for details
  Moments2d2C Polygon2dC::Moments() const {
    RealT m00 = 0.0;
    RealT m10 = 0.0;
    RealT m01 = 0.0;
    RealT m20 = 0.0;
    RealT m11 = 0.0;
    RealT m02 = 0.0;
    if (!IsEmpty()) {
      DLIterC<Point2dC> pLast(*this);
      pLast.Last();

      for (DLIterC<Point2dC> ptr(*this); ptr != pLast; ptr++) {
        Point2dC p1 = ptr.Data();
        Point2dC p2 = ptr.NextData();
        RealT p1_10 = p1.X(), p1_01 = p1.Y();
        RealT p2_10 = p2.X(), p2_01 = p2.Y();
        m00 += p1_10 * p2_01 - p2_10 * p1_01;
        RealT temp = p1_10 * p2_01 - p2_10 * p1_01;
        m10 += (p1_10 + p2_10) * temp;
        m01 += (p1_01 + p2_01) * temp;
        m20 += (Sqr(p1_10) + p1_10*p2_10 + Sqr(p2_10)) * temp;
        m02 += (Sqr(p1_01) + p1_01*p2_01 + Sqr(p2_01)) * temp;
        m11 += (2.0*p1_10*p1_01 + p1_10*p2_01 + p2_10*p1_01 + 2.0*p2_10*p2_01) * temp;
      }
      // close the polygon
      Point2dC p1 = pLast.Data();
      Point2dC p2 = pLast.NextCrcData();
      RealT p1_10 = p1.X(), p1_01 = p1.Y();
      RealT p2_10 = p2.X(), p2_01 = p2.Y();
      m00 += p1_10 * p2_01 - p2_10 * p1_01;
      RealT temp = p1_10 * p2_01 - p2_10 * p1_01;
      m10 += (p1_10 + p2_10) * temp;
      m01 += (p1_01 + p2_01) * temp;
      m20 += (Sqr(p1_10) + p1_10*p2_10 + Sqr(p2_10)) * temp;
      m02 += (Sqr(p1_01) + p1_01*p2_01 + Sqr(p2_01)) * temp;
      m11 += (2.0*p1_10*p1_01 + p1_10*p2_01 + p2_10*p1_01 + 2.0*p2_10*p2_01) * temp;
    }
    m00 *= 0.5;
    RealT oneOver6 = 1.0 / 6.0;
    m10 *= oneOver6;
    m01 *= oneOver6;
    RealT oneOver12 = 1.0 / 12.0;
    m20 *= oneOver12;
    m02 *= oneOver12;
    RealT oneOver24 = 1.0 / 24.0;
    m11 *= oneOver24;
    return Moments2d2C(m00, m10, m01, m20, m11, m02);
  }
  
  bool Polygon2dC::IsDiagonal(const DLIterC<Point2dC> & a, const DLIterC<Point2dC> & b, bool allowExternal) const {
    if (allowExternal) {
         
      Point2dC ap(a.Data());
      Point2dC bp(b.Data());
         
      // For each edge (k,k+1) of this polygon.
      for (DLIterC<Point2dC> k(*this); k; k++)
        {
          DLIterC<Point2dC> k1(k);
          k1.NextCrc();
          // Skip edges incident to a or b. 
          if ( ! (( k == a ) || ( k1 == a ) ||( k == b ) || ( k1 == b )) )
            if ( LinePP2dC(ap,bp).HasInnerIntersection(LinePP2dC(k.Data(), k1.Data())) )
              return false;
        }
      return true;
    }
    else {
      return IsInCone(a,b) && IsDiagonal(a,b,true);
    }
  }

  bool Polygon2dC::IsInCone(const DLIterC<Point2dC> & a, const DLIterC<Point2dC> & b) const {
    Point2dC pa(a.Data());
    Point2dC pan(a.NextCrcData());
    Point2dC pap(a.PrevCrcData());
    Point2dC pb(b.Data());

    // If 'pa' is a convex vertex ['pan' is left or on (pap, pa) ].
    if (LinePP2dC(pap, pa).IsPointToRightOn(pan))
      return LinePP2dC(pa, pb).IsPointToRight(pap) && LinePP2dC(pb, pa).IsPointToRight(pan);
    else
      // Assume (i-1,i,i+1) not collinear.
      // else 'pa' is reflex.
      return !(LinePP2dC(pa, pb).IsPointToRightOn(pan) && LinePP2dC(pb, pa).IsPointToRightOn(pap));
  }

  Polygon2dC Polygon2dC::ClipByConvex(const Polygon2dC &oth) const {
    if (oth.Size() < 3)
      return Polygon2dC();
    Polygon2dC ret = *this;
    DLIterC<Point2dC> pLast(oth);
    pLast.Last();
    for (DLIterC<Point2dC> ptr(oth); ptr != pLast; ptr++)
      ret = ret.ClipByLine(LinePP2dC(ptr.Data(), ptr.NextData()));
    // close the polygon
    ret = ret.ClipByLine(LinePP2dC(pLast.Data(), pLast.NextCrcData()));
    return ret;
  }

  Polygon2dC Polygon2dC::ClipByLine(const LinePP2dC &line) const {
    Polygon2dC ret;
    if (IsEmpty()) // Empty polygon to start with ?
      return ret;
    DLIterC<Point2dC> st(*this);
    st.Last();
    Point2dC intersection;
    for (DLIterC<Point2dC> pt(*this); pt; pt++) {
      if (line.IsPointToRightOn(*pt)) {
        if (line.IsPointToRightOn(*st)) {
          ret.InsLast(*pt);
        } else {
          if (line.Intersection(LinePP2dC(*st,*pt), intersection)) {
            ret.InsLast(intersection);
          }
          ret.InsLast(*pt);
        }
      } else {
        if (line.IsPointToRightOn(*st)) {
          if (line.Intersection(LinePP2dC(*st,*pt), intersection)) {
            ret.InsLast(intersection);
          }
        }
      }
      st = pt;
    }
    return ret;
  }

  Polygon2dC Polygon2dC::ClipByAxis(RealT threshold, UIntT axis, bool isGreater) const {
    RavlAssert(axis == 0 || axis == 1);
    Polygon2dC ret;
    if (IsEmpty()) // Empty polygon to start with ?
      return ret;
    DLIterC<Point2dC> st(*this);
    st.Last();
    Point2dC intersection;
    LinePP2dC line(Point2dC(threshold,threshold), Vector2dC(axis==1,axis==0));
    for (DLIterC<Point2dC> pt(*this); pt; pt++) {
      if (isGreater ? (*pt)[axis] >= threshold: (*pt)[axis] <= threshold) {
        if (isGreater ? (*st)[axis] >= threshold: (*st)[axis] <= threshold) {
          ret.InsLast(*pt);
        } else {
          if (line.Intersection(LinePP2dC(*st,*pt), intersection)) {
            ret.InsLast(intersection);
          }
          ret.InsLast(*pt);
        }
      } else {
        if (isGreater ? (*st)[axis] >= threshold: (*st)[axis] <= threshold) {
          if (line.Intersection(LinePP2dC(*st,*pt), intersection)) {
            ret.InsLast(intersection);
          }
        }
      }
      st = pt;
    }
    return ret;
  }

  Polygon2dC Polygon2dC::ClipByRange(const RealRange2dC &rng) const {
    Polygon2dC ret = *this;
    ret = ret.ClipByAxis(rng.TRow(), 0, 1);
    ret = ret.ClipByAxis(rng.RCol(), 1, 0);
    ret = ret.ClipByAxis(rng.BRow(), 0, 0);
    ret = ret.ClipByAxis(rng.LCol(), 1, 1);
    return ret;
  }

  bool Polygon2dC::Contains(const Point2dC & p) const {
      
    // Check singularities.
    SizeT size = Size();
    if (size == 0) return false;
    Point2dC p1(First());
    if (size == 1) return p == p1;
      
    // The point can lie on the boundary of the polygon.
    for (DLIterC<Point2dC> point(*this); point; point++) {
      if (LinePP2dC(point.Data(), point.NextCrcData()).IsPointIn(p))
        return true;
    }
      
    // Take my testline arbitrarily as parallel to y=0. Assumption is that 
    // Point2dC(p[0]+100...) provides enough accuracy for the calculation
    // - not envisaged that this is a real problem
    Point2dC secondPoint(p[0]+100,p[1]);
    LinePP2dC testLine(p, secondPoint);
      
    // Just something useful for later, check whether the last point lies
    // to the left or right of my testline
    bool leftof = testLine.IsPointToRight(Last());
      
    // For each edge (k,k+1) of this polygon count the instersection
    // with the polygon segments.
    int count = 0;
    for (DLIterC<Point2dC> k(*this); k; k++) {
      LinePP2dC l2(k.Data(), k.NextCrcData());
      RealT intersect = l2.ParIntersection(testLine);

      // If l2 and testline are collinear then either the point lies on an
      // edge (checked already) or it acts as a vertex. I really
      // should check for the case, or it could throw ParInterSection
      if (testLine.IsPointOn(l2.P1())
          && testLine.IsPointOn(l2.P2()));
         
      // Be sure to count each vertex just once
      else if (intersect > 0
               && intersect <=1
               && testLine.ParIntersection(l2) > 0) 
        count++;   
    
      // Examine the case where testline meets polygon at vertex "cusp"
      // iff testline passes through a vertex and yet not into polygon
      // at that vertex _and_ the vertex lies to the right of my test point
      // then we count that vertex twice
      else if (intersect == 0 && p[0] <= l2.P1()[0]
               && leftof == testLine.IsPointToRight(l2.P2()))
        count++;
    
      // Set the flag for the case of the line passing through 
      // the endpoint vertex
      Vector2dC u2P(testLine.P1()[1] - testLine.P2()[1],testLine.P2()[0] - testLine.P1()[0]); 
      if (!IsNan(intersect) && (intersect ==1))
        leftof = testLine.IsPointToRight(l2.P1());
    }
  
    return (count%2) == 1;
  }

   bool Polygon2dC::IsSelfIntersecting() const {
     DLIterC<Point2dC> ft(*this);
     DLIterC<Point2dC> lt(*this); lt.Last();
     // first loop does all but last side
     LinePP2dC l1(ft.Data(), ft.NextCrcData());
     DLIterC<Point2dC> it2 = ft; it2++;
     if (it2) {
       for (it2++; it2 != lt; it2++) {
         LinePP2dC l2(it2.Data(), it2.NextData());
         if (l1.HasInnerIntersection(l2))
           return true;
       }
     }
     // then go to the last side for all subsequent iterations
     for (ft++; ft != lt; ft++) {
       LinePP2dC l1(ft.Data(), ft.NextCrcData());
       DLIterC<Point2dC> it2 = ft; it2++;
       if (it2) {
         for (it2++; it2; it2++) {
           LinePP2dC l2(it2.Data(), it2.NextCrcData());
           if (l1.HasInnerIntersection(l2))
             return true;
         }
       }
     }
     return false;
   }

  RealT Polygon2dC::Perimeter() const {
    RealT perimeter = 0.0;
    DLIterC<Point2dC> it(*this);
    if(!it) return 0.0;
    Point2dC lastPoint = Last();
    for (; it; it++) {
      perimeter += it->EuclidDistance(lastPoint);
      lastPoint = *it;
    }
    return perimeter;
  }
  
  //: Measure the fraction of the polygons overlapping.
  //!return: 0= Not overlapping 1=this is completely covered by poly.
  
  RealT Polygon2dC::Overlap(const Polygon2dC &poly) const {
    if(IsEmpty() || poly.IsEmpty())
      return 0;
    RealT thisArea = Area();
    RealT polyArea = poly.Area();
    Polygon2dC localPoly = poly;
    Polygon2dC localThis = *this;
    if(thisArea > 0 && polyArea > 0) {
      localThis = Copy();
      localThis.Reverse();
      localPoly = poly.Copy();
      localPoly.Reverse();
    }
    
    Polygon2dC overlap = localThis.ClipByConvex(localPoly);
    return Abs(overlap.Area()) / Abs(thisArea);
  }

  //: Measure the fraction of the polygons overlapping as a fraction of the larger of the two polygons.
  //!return: 0= Not overlapping 1=If the two polygons are identical.
  
  RealT Polygon2dC::CommonOverlap(const Polygon2dC &poly) const {
    if(IsEmpty() || poly.IsEmpty())
      return 0;
    RealT polyArea = poly.Area();
    RealT thisArea = Area();
    Polygon2dC localPoly = poly;
    Polygon2dC localThis = *this;
    if(thisArea > 0 && polyArea > 0)  {
      localThis = Copy();
      localThis.Reverse();
      localPoly = poly.Copy();
      localPoly.Reverse();
    }
    
    Polygon2dC overlap = localThis.ClipByConvex(localPoly);
    return Abs(overlap.Area()) / Max(Abs(thisArea),Abs(polyArea));
  }
  
}
