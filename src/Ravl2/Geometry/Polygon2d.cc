// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Polygon2d.hh"
#include "Ravl2/Geometry/Moments2.hh"
#include "Ravl2/Geometry/LinePP.hh"

namespace Ravl2
{

  template <typename RealT>
  Polygon2dC<RealT>::Polygon2dC(const Range<RealT, 2> &range, BoundaryOrientationT orientation)
  {
    this->reserve(4);
    if(orientation == BoundaryOrientationT::INSIDE_LEFT) {
      // Clockwise
      this->push_back(range.min());
      this->push_back(toPoint<RealT>(range[0].max(),range[1].min()));
      this->push_back(range.max());
      this->push_back(toPoint<RealT>(range[0].min(),range[1].max()));
    } else {
      // Counter clockwise
      this->push_back(range.min());
      this->push_back(toPoint<RealT>(range[0].min(), range[1].max()));
      this->push_back(range.max());
      this->push_back(toPoint<RealT>(range[0].max(), range[1].min()));
    }
  }

  template <typename RealT>
  RealT Polygon2dC<RealT>::area() const
  {
    RealT sum = 0.0;
    if(!this->empty()) {
      auto pLast = this->back();
      for(auto ptr : *this) {
        sum += pLast[0] * ptr[1] - ptr[0] * pLast[1];
        pLast = ptr;
      }
    }
    return sum * RealT(0.5);
  }

  template <typename RealT>
  [[nodiscard]] bool Polygon2dC<RealT>::isConvex(BoundaryOrientationT orientation) const
  {
    if(this->size() < 3)
      return false;
    auto pLast = this->back();
    for(auto ptr = this->begin(); ptr != this->end(); ++ptr) {
      auto pNext = nextDataCrc(*this, ptr);
      // Look out for degenerate polygons
      assert(squaredEuclidDistance(pLast, pNext) > std::numeric_limits<RealT>::epsilon());
      assert(squaredEuclidDistance(pLast, *ptr) > std::numeric_limits<RealT>::epsilon());
      if (LinePP2dC<RealT>(pLast, pNext).IsPointInsideOn(*ptr,orientation))
        return false;
      pLast = *ptr;
    }
    return true;
  }

  template <typename RealT>
  Point<RealT, 2> Polygon2dC<RealT>::Centroid() const
  {
    RealT x = 0.0;
    RealT y = 0.0;
    if(!this->empty()) {
      auto pLast = this->back();
      for(auto ptr : *this) {
        RealT temp = pLast[0] * ptr[1] - ptr[0] * pLast[1];
        x += (pLast[0] + ptr[0]) * temp;
        y += (pLast[1] + ptr[1]) * temp;
      }
    }
    RealT scale = 1 / (6 * area());
    return toPoint<RealT>(x * scale, y * scale);
  }


  template<typename RealT>
  Moments2<RealT> moments(const Polygon2dC<RealT> &poly)
  {
    RealT m00 = 0.0;
    RealT m10 = 0.0;
    RealT m01 = 0.0;
    RealT m20 = 0.0;
    RealT m11 = 0.0;
    RealT m02 = 0.0;
    if (!poly.empty()) {
      const auto pLast = poly.last();

      for(auto it : poly) {
	Point<RealT, 2> p1 = pLast;
	Point<RealT, 2> p2 = it;
	RealT p1_10 = p1[0], p1_01 = p1[1];
	RealT p2_10 = p2[0], p2_01 = p2[1];
	m00 += p1_10 * p2_01 - p2_10 * p1_01;
	RealT temp = p1_10 * p2_01 - p2_10 * p1_01;
	m10 += (p1_10 + p2_10) * temp;
	m01 += (p1_01 + p2_01) * temp;
	m20 += (Sqr(p1_10) + p1_10 * p2_10 + sqr(p2_10)) * temp;
	m02 += (Sqr(p1_01) + p1_01 * p2_01 + sqr(p2_01)) * temp;
	m11 += (2.0 * p1_10 * p1_01 + p1_10 * p2_01 + p2_10 * p1_01 + 2.0 * p2_10 * p2_01) * temp;
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
    }
    return Moments2d2C(m00, m10, m01, m20, m11, m02);
  }

  template<typename RealT>
  bool Polygon2dC<RealT>::contains(const Point<RealT,2> & p) const
  {

    RealT tolerance = 0; //std::numeric_limits<RealT>::epsilon();

    // Check singularities.
    size_t size = this->size();
    if (size == 0) return false;
    Point<RealT,2> p1 = this->front();
    if (size == 1) return p == p1;

    // The point can lie on the boundary of the polygon.
    {
      auto pLast = this->back();
      for (auto point : *this) {
        if (LinePP2dC<RealT>(pLast, point).IsPointIn(p, tolerance))
          return true;
        pLast = point;
      }
    }

    // Take my testline arbitrarily as parallel to y=0. Assumption is that
    // Point<RealT,2>(p[0]+100...) provides enough accuracy for the calculation
    // - not envisaged that this is a real problem
    Point<RealT,2> secondPoint = toPoint<RealT>(p[0]+100,p[1]);
    LinePP2dC testLine(p, secondPoint);

    // Just something useful for later, check whether the last point lies
    // to the left or left of my testline
    bool leftof = testLine.IsPointToLeftOn(this->back());

    // For each edge (k,k+1) of this polygon count the intersection
    // with the polygon segments.
    int count = 0;

    auto pLast = this->back();
    for (auto k : *this) {
      LinePP2dC l2(pLast, k);
      RealT intersect = l2.ParIntersection(testLine);

      // If l2 and testline are collinear then either the point lies on an
      // edge (checked already) or it acts as a vertex. I really
      // should check for the case, or it could throw ParInterSection
      if (testLine.IsPointOn(l2.P1(),tolerance)
          && testLine.IsPointOn(l2.P2(),tolerance)) {

        // Be sure to count each vertex just once
      } else if (intersect > 0
                 && intersect <= 1
                 && testLine.ParIntersection(l2) > 0) {
        count++;

          // Examine the case where testline meets polygon at vertex "cusp"
          // iff testline passes through a vertex and yet not into polygon
          // at that vertex _and_ the vertex lies to the left of my test point
          // then we count that vertex twice
        } else if (intersect == 0 && p[0] <= l2.P1()[0]
               && leftof == testLine.IsPointToLeftOn(l2.P2())) {
        count++;
      }

      // Set the flag for the case of the line passing through
      // the endpoint vertex
      //Vector<RealT,2> u2P = toVector<RealT>(testLine.P1()[1] - testLine.P2()[1],testLine.P2()[0] - testLine.P1()[0]);
      if (!std::isnan(intersect) && (intersect ==1))
        leftof = testLine.IsPointToLeft(l2.P1());
    }

    return (count%2) == 1;
  }

  template<typename RealT>
  Polygon2dC<RealT> Polygon2dC<RealT>::ClipByConvex(const Polygon2dC &oth,BoundaryOrientationT othOrientation) const
  {
    if (oth.size() < 3)
      return Polygon2dC();
    Polygon2dC ret = *this; // FixMe:- This makes a poinless copy of the polygon.
    auto pLast = oth.back();
    for (auto ptr : oth) {
      ret = ret.ClipByLine(LinePP2dC<RealT>(pLast, ptr),othOrientation);
      pLast = ptr;
    }
    return ret;
  }

  //! Add a point checking it isn't a duplicate of the last one.
  template<typename RealT>
  void Polygon2dC<RealT>::addBack(const Point<RealT, 2> &pnt)
  {
    if(this->empty() || squaredEuclidDistance(pnt, this->back()) > std::numeric_limits<RealT>::epsilon())
      this->push_back(pnt);
  }


  template<typename RealT>
  Polygon2dC<RealT> Polygon2dC<RealT>::ClipByLine(const LinePP2dC<RealT> &line, BoundaryOrientationT lineOrientation) const
  {
    Polygon2dC ret;
    if (this->size() < 3) // Empty polygon to start with ?
      return ret;
    ret.reserve(this->size()+1); // Worst case we cut off a spike and add a point.
    //! Check we're not being given a degenerate line.
    assert(euclidDistance(line.P1(),line.P2()) > std::numeric_limits<RealT>::epsilon());
    Point<RealT,2> st = this->back();
    for (auto pt : *this) {
      if (line.IsPointInsideOn(pt,lineOrientation)) {
        if (line.IsPointInsideOn(st,lineOrientation)) {
          ret.push_back(pt);
        } else {
	  auto intersection = LinePP2dC(st, pt).innerIntersection(line);
	  if(intersection.has_value()) {
            ret.addBack(intersection.value());
	  }
          ret.addBack(pt);
        }
      } else {
        if (line.IsPointInsideOn(st,lineOrientation)) {
	  auto intersection = LinePP2dC(st, pt).innerIntersection(line);
	  if (intersection.has_value()) {
	    ret.addBack(intersection.value());
	  }
        }
      }
      st = pt;
    }
    // Avoid generating degenerate polygons.
    if(!ret.empty()) {
      // Check first and last are different.
      if(squaredEuclidDistance(ret.front(),ret.back()) < std::numeric_limits<RealT>::epsilon()) {
        ret.pop_back();
      }
    }
    if(ret.size() < 3) {
      ret.clear();
    }
    return ret;
  }


  template<typename RealT>
  Polygon2dC<RealT> Polygon2dC<RealT>::ClipByAxis(RealT threshold, unsigned axis, bool isGreater) const
  {
    RavlAssert(axis < 2);
    Polygon2dC ret;
    // Empty polygon to start with ?
    if (this->empty())
      return ret;
    ret.reserve(this->size()+1); // Worst case we cut off a spike and add a point.
    PointT st = this->back();

    auto axisLine = LinePP2dC<RealT>::fromStartAndDirection(toPoint<RealT>(threshold, threshold), toVector<RealT>(axis == 1 ? 1 : 0, axis == 0 ? 1 : 0));

    for (auto pt : (*this)) {
      if (isGreater ? ((pt)[axis] >= threshold): ((pt)[axis] <= threshold)) {
        if (isGreater ? ((st)[axis] >= threshold): ((st)[axis] <= threshold)) {
          ret.push_back(pt);
        } else {
          auto intersection = LinePP2dC(st, pt).innerIntersection(axisLine);
          if(intersection.has_value()) {
            ret.addBack(intersection.value());
          }
          ret.addBack(pt);
        }
      } else {
        if (isGreater ? ((st)[axis] >= threshold): ((st)[axis] <= threshold)) {
          auto intersection = LinePP2dC(st, pt).innerIntersection(axisLine);
          if (intersection.has_value()) {
            ret.addBack(intersection.value());
          }
        }
      }
      st = pt;
    }
    if(!ret.empty()) {
      // Check first and last are different.
      if(squaredEuclidDistance(ret.front(),ret.back()) < std::numeric_limits<RealT>::epsilon()) {
        ret.pop_back();
      }
    }
    // Avoid generating degenerate polygons.
    if(ret.size() < 3) {
      ret.clear();
    }
    return ret;
  }


  template<typename RealT>
  Polygon2dC<RealT> Polygon2dC<RealT>::ClipByRange(const Range<RealT,2> &rng) const
  {
    Polygon2dC ret = this->ClipByAxis(rng.min(0), 0, 1);
    ret = ret.ClipByAxis(rng.max(1), 1, 0);
    ret = ret.ClipByAxis(rng.max(0), 0, 0);
    ret = ret.ClipByAxis(rng.min(1), 1, 1);
    return ret;
  }


  template<typename RealT>
   bool Polygon2dC<RealT>::IsSelfIntersecting() const
   {
     auto ft = this->begin();
     if(ft == this->end())
       return false;
     auto lt = this->end()-1;
     const auto theEnd = this->end();
     {
       // first loop does all but last side
       LinePP2dC l1(*ft, nextDataCrc(*this, ft));
       auto it2 = ft;
       it2++;
       if (it2 != this->end()) {
         for (it2++; it2 != lt; it2++) {
           LinePP2dC l2(*it2, nextDataCrc(*this, it2));
           if (l1.HasInnerIntersection(l2))
             return true;
         }
       }
     }
     // then go to the last side for all subsequent iterations
     for (ft++; ft != lt; ft++) {
       LinePP2dC l1(*ft, nextDataCrc(*this, ft));
       auto it2 = ft; it2++;
       if (it2 != this->end()) {
         for (it2++; it2 != theEnd; it2++) {
           LinePP2dC l2(*it2, nextDataCrc(*this,it2));
           if (l1.HasInnerIntersection(l2))
             return true;
         }
       }
     }
     return false;
   }


  template<typename RealT>
  RealT Polygon2dC<RealT>::Perimeter() const
  {
    RealT perimeter = 0.0;
    if(!this->empty()) {
      auto pLast = this->back();
      for(auto ptr : *this) {
        perimeter += euclidDistance(pLast,ptr);
        pLast = ptr;
      }
    }
    return perimeter;
  }


  template<typename RealT>
  RealT Polygon2dC<RealT>::Overlap(const Polygon2dC &poly) const
  {
    if(this->empty() || poly.empty())
      return 0;
    RealT thisArea = area();
    Polygon2dC overlap = ClipByConvex(poly);
    return overlap.area() / thisArea;
  }

  template<typename RealT>
  RealT Polygon2dC<RealT>::CommonOverlap(const Polygon2dC &poly) const
  {
    if(this->empty() || poly.empty())
      return 0;
    RealT polyArea = poly.area();
    RealT thisArea = area();
    Polygon2dC overlap = ClipByConvex(poly);
    return overlap.area() / std::max(thisArea,polyArea);
  }

  //! Let the compiler know that we will use these classes with the following types
  template class Polygon2dC<float>;
  template class Polygon2dC<double>;

}// namespace Ravl2
