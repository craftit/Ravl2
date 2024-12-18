// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Polygon.hh"
#include "Ravl2/Geometry/PolyApprox.hh"
#include "Ravl2/Geometry/Moments2.hh"
#include "Ravl2/Geometry/LinePP.hh"

namespace Ravl2
{

  template <typename RealT>
  Polygon<RealT>::Polygon(const Range<RealT, 2> &range, BoundaryOrientationT orientation)
  {
    this->reserve(4);
    if(orientation == BoundaryOrientationT::INSIDE_LEFT) {
      // Clockwise
      this->push_back(range.min());
      this->push_back(toPoint<RealT>(range[0].max(), range[1].min()));
      this->push_back(range.max());
      this->push_back(toPoint<RealT>(range[0].min(), range[1].max()));
    } else {
      // Counterclockwise
      this->push_back(range.min());
      this->push_back(toPoint<RealT>(range[0].min(), range[1].max()));
      this->push_back(range.max());
      this->push_back(toPoint<RealT>(range[0].max(), range[1].min()));
    }
  }
  
  template <typename RealT>
  RealT Polygon<RealT>::area() const
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
  [[nodiscard]] bool Polygon<RealT>::isConvex(BoundaryOrientationT orientation) const
  {
    if(this->size() < 3)
      return false;
    auto pLast = this->back();
    for(auto ptr = this->begin(); ptr != this->end(); ++ptr) {
      auto pNext = nextDataCrc(*this, ptr);
      // Look out for degenerate polygons
      assert((pLast - pNext).cwiseAbs().sum() > std::numeric_limits<RealT>::epsilon());
      assert((pLast - *ptr).cwiseAbs().sum() > std::numeric_limits<RealT>::epsilon());
      if(Line2PP<RealT>(pLast, pNext).IsPointInsideOn(*ptr, orientation))
        return false;
      pLast = *ptr;
    }
    return true;
  }

  template <typename RealT>
  Point<RealT, 2> Polygon<RealT>::centroid() const
  {
    RealT x = 0.0;
    RealT y = 0.0;
    if(!this->empty()) {
      auto pLast = this->back();
      for(auto ptr : *this) {
        RealT temp = pLast[0] * ptr[1] - ptr[0] * pLast[1];
        x += (pLast[0] + ptr[0]) * temp;
        y += (pLast[1] + ptr[1]) * temp;
        pLast = ptr;
      }
    }
    RealT scale = 1 / (6 * area());
    return toPoint<RealT>(x * scale, y * scale);
  }


  template <typename RealT>
  bool Polygon<RealT>::contains(const Point<RealT, 2> &p) const
  {

    RealT tolerance = 0;//std::numeric_limits<RealT>::epsilon();

    // Check singularities.
    size_t size = this->size();
    if(size == 0) return false;
    Point<RealT, 2> p1 = this->front();
    if(size == 1) return p == p1;

    // Take my test-line arbitrarily as parallel to y=0. Assumption is that
    // Point<RealT,2>(p[0]+100...) provides enough accuracy for the calculation
    // - not envisaged that this is a real problem
    Point<RealT, 2> secondPoint = toPoint<RealT>(p[0] + 100, p[1]);
    Line2PP testLine(p, secondPoint);

    // Just something useful for later, check whether the last point lies
    // to the left or left of my test-line
    bool leftof = testLine.IsPointToLeftOn(this->back());

    // For each edge (k,k+1) of this polygon count the intersection
    // with the polygon segments.
    int count = 0;

    auto pLast = this->back();
    for(auto k : *this) {
      Line2PP l2(pLast, k);
      // The point can lie on the boundary of the polygon.
      // This creates lots of edge cases. To avoid this, if the point is on the boundary
      // it is considered to be inside the polygon.
      if(l2.IsPointIn(p, tolerance)) {
        return true;
      }

      RealT intersect = l2.ParIntersection(testLine);

      // If l2 and test line are collinear then either the point lies on an
      // edge (checked already) or it acts as a vertex. I really
      // should check for the case, or it could throw ParInterSection
      if(testLine.IsPointOn(l2.P1(), tolerance)
         && testLine.IsPointOn(l2.P2(), tolerance)) {

        // Be sure to count each vertex just once
      } else if(intersect > 0
                && intersect <= 1
                && testLine.ParIntersection(l2) > 0) {
        count++;

        // Examine the case where test-line meets polygon at vertex "cusp"
        // iff test-line passes through a vertex and yet not into polygon
        // at that vertex _and_ the vertex lies to the left of my test point
        // then we count that vertex twice
      } else if(intersect == 0 && p[0] <= l2.P1()[0]
                && leftof == testLine.IsPointToLeftOn(l2.P2())) {
        count++;
      }

      // Set the flag for the case of the line passing through
      // the endpoint vertex
      //Vector<RealT,2> u2P = toVector<RealT>(testLine.P1()[1] - testLine.P2()[1],testLine.P2()[0] - testLine.P1()[0]);
      if(!std::isnan(intersect) && (intersect == 1))
        leftof = testLine.IsPointToLeft(l2.P1());
      pLast = k;
    }

    return (count % 2) == 1;
  }

  template <typename RealT>
  Polygon<RealT> Polygon<RealT>::clipByConvex(const Polygon &oth, BoundaryOrientationT othOrientation) const
  {
    if(oth.size() < 3)
      return Polygon();
    Polygon ret = *this;// FixMe:- This makes a poinless copy of the polygon.
    auto pLast = oth.back();
    for(auto ptr : oth) {
      ret = ret.clipByLine(Line2PP<RealT>(pLast, ptr), othOrientation);
      pLast = ptr;
    }
    return ret;
  }

  //! Add a point checking it isn't a duplicate of the last one.
  template <typename RealT>
  void Polygon<RealT>::addBack(const Point<RealT, 2> &pnt)
  {
    if(this->empty() || squaredEuclidDistance<RealT,2>(pnt, this->back()) > std::numeric_limits<RealT>::epsilon())
      this->push_back(pnt);
  }

  template <typename RealT>
  Polygon<RealT> Polygon<RealT>::clipByLine(const Line2PP<RealT> &line, BoundaryOrientationT lineOrientation) const
  {
    Polygon ret;
    if(this->size() < 3)// Empty polygon to start with ?
      return ret;
    ret.reserve(this->size() + 1);// Worst case we cut off a spike and add a point.
    //! Check we're not being given a degenerate line.
    assert(euclidDistance(line.P1(), line.P2()) > std::numeric_limits<RealT>::epsilon());
    Point<RealT, 2> st = this->back();
    for(auto pt : *this) {
      if(line.IsPointInsideOn(pt, lineOrientation)) {
        if(line.IsPointInsideOn(st, lineOrientation)) {
          ret.push_back(pt);
        } else {
          auto intersection = Line2PP(st, pt).innerIntersection(line);
          // This really should be true, but we'll check it anyway.
          if(intersection.has_value()) {
            ret.addBack(intersection.value());
          }
          ret.addBack(pt);
        }
      } else {
        if(line.IsPointInsideOn(st, lineOrientation)) {
          auto intersection = Line2PP(st, pt).innerIntersection(line);
          // This really should be true, but we'll check it anyway.
          if(intersection.has_value()) {
            ret.addBack(intersection.value());
          }
        }
      }
      st = pt;
    }
    // Avoid generating degenerate polygons.
    if(!ret.empty()) {
      // Check first and last are different.
      if(squaredEuclidDistance<RealT,2>(ret.front(), ret.back()) < std::numeric_limits<RealT>::epsilon()) {
        ret.pop_back();
      }
    }
    if(ret.size() < 3) {
      ret.clear();
    }
    return ret;
  }

  template <typename RealT>
  Polygon<RealT> Polygon<RealT>::clipByAxis(RealT threshold, unsigned axis, bool isGreater) const
  {
    RavlAssert(axis < 2);
    Polygon ret;
    // Empty polygon to start with ?
    if(this->empty())
      return ret;
    ret.reserve(this->size() + 1);// Worst case we cut off a spike and add a point.
    PointT st = this->back();

    auto axisLine = Line2PP<RealT>::fromStartAndDirection(toPoint<RealT>(threshold, threshold), toVector<RealT>(axis == 1 ? 1 : 0, axis == 0 ? 1 : 0));

    for(auto pt : (*this)) {
      if(isGreater ? ((pt)[axis] >= threshold) : ((pt)[axis] <= threshold)) {
        if(isGreater ? ((st)[axis] >= threshold) : ((st)[axis] <= threshold)) {
          ret.push_back(pt);
        } else {
          auto intersection = Line2PP(st, pt).innerIntersection(axisLine);
          // This really should be true, but we'll check it anyway.
          if(intersection.has_value()) {
            ret.addBack(intersection.value());
          }
          ret.addBack(pt);
        }
      } else {
        if(isGreater ? ((st)[axis] >= threshold) : ((st)[axis] <= threshold)) {
          auto intersection = Line2PP(st, pt).innerIntersection(axisLine);
          // This really should be true, but we'll check it anyway.
          if(intersection.has_value()) {
            ret.addBack(intersection.value());
          }
        }
      }
      st = pt;
    }
    if(!ret.empty()) {
      // Check first and last are different.
      if(squaredEuclidDistance<RealT,2>(ret.front(), ret.back()) < std::numeric_limits<RealT>::epsilon()) {
        ret.pop_back();
      }
    }
    // Avoid generating degenerate polygons.
    if(ret.size() < 3) {
      ret.clear();
    }
    return ret;
  }

  template <typename RealT>
  Polygon<RealT> Polygon<RealT>::clipByRange(const Range<RealT, 2> &rng) const
  {
    Polygon ret = this->clipByAxis(rng.min(0), 0, 1);
    ret = ret.clipByAxis(rng.max(1), 1, 0);
    ret = ret.clipByAxis(rng.max(0), 0, 0);
    ret = ret.clipByAxis(rng.min(1), 1, 1);
    return ret;
  }

  template <typename RealT>
  bool Polygon<RealT>::isSelfIntersecting() const
  {
    auto ft = this->begin();
    if(ft == this->end())
      return false;
    auto lt = this->end() - 1;
    const auto theEnd = this->end();
    {
      // first loop does all but last side
      Line2PP l1(*ft, nextDataCrc(*this, ft));
      auto it2 = ft;
      it2++;
      if(it2 != this->end()) {
        for(it2++; it2 != lt; it2++) {
          Line2PP l2(*it2, nextDataCrc(*this, it2));
          if(l1.HasInnerIntersection(l2))
            return true;
        }
      }
    }
    // then go to the last side for all subsequent iterations
    for(ft++; ft != lt; ft++) {
      Line2PP l1(*ft, nextDataCrc(*this, ft));
      auto it2 = ft;
      it2++;
      if(it2 != this->end()) {
        for(it2++; it2 != theEnd; it2++) {
          Line2PP l2(*it2, nextDataCrc(*this, it2));
          if(l1.HasInnerIntersection(l2))
            return true;
        }
      }
    }
    return false;
  }

  template <typename RealT>
  RealT Polygon<RealT>::perimeter() const
  {
    RealT perimeter = 0.0;
    if(!this->empty()) {
      auto pLast = this->back();
      for(auto ptr : *this) {
        perimeter += euclidDistance(pLast, ptr);
        pLast = ptr;
      }
    }
    return perimeter;
  }

  template <typename RealT>
  RealT Polygon<RealT>::overlap(const Polygon &poly) const
  {
    if(this->empty() || poly.empty())
      return 0;
    RealT thisArea = area();
    Polygon overlap = clipByConvex(poly);
    return overlap.area() / thisArea;
  }

  template <typename RealT>
  RealT Polygon<RealT>::commonOverlap(const Polygon &poly) const
  {
    if(this->empty() || poly.empty())
      return 0;
    RealT polyArea = poly.area();
    RealT thisArea = area();
    Polygon overlap = clipByConvex(poly);
    return overlap.area() / std::max(thisArea, polyArea);
  }

  template <typename RealT>
  Moments2<RealT> moments(const Polygon<RealT> &poly)
  {
    RealT m00 = 0.0;
    RealT m10 = 0.0;
    RealT m01 = 0.0;
    RealT m20 = 0.0;
    RealT m11 = 0.0;
    RealT m02 = 0.0;
    if(!poly.empty()) {
      auto p1 = poly.back();

      for(auto p2 : poly) {
        RealT p1_10 = p1[0];
        RealT p1_01 = p1[1];
        RealT p2_10 = p2[0];
        RealT p2_01 = p2[1];
        m00 += p1_10 * p2_01 - p2_10 * p1_01;
        RealT temp = p1_10 * p2_01 - p2_10 * p1_01;
        m10 += (p1_10 + p2_10) * temp;
        m01 += (p1_01 + p2_01) * temp;
        m20 += (sqr(p1_10) + p1_10 * p2_10 + sqr(p2_10)) * temp;
        m02 += (sqr(p1_01) + p1_01 * p2_01 + sqr(p2_01)) * temp;
        m11 += (2 * p1_10 * p1_01 + p1_10 * p2_01 + p2_10 * p1_01 + 2 * p2_10 * p2_01) * temp;
        p1 = p2;
      }

      m00 *= RealT(0.5);
      const RealT oneOver6 = RealT(1.0 / 6.0);
      m10 *= oneOver6;
      m01 *= oneOver6;
      const RealT oneOver12 = RealT(1.0 / 12.0);
      m20 *= oneOver12;
      m02 *= oneOver12;
      const RealT oneOver24 = RealT(1.0 / 24.0);
      m11 *= oneOver24;
    }
    return Moments2<RealT>(m00, m10, m01, m20, m11, m02);
  }

  // Instantiate the template for the types we will use.
  template Moments2<float> moments(const Polygon<float> &poly);
  template Moments2<double> moments(const Polygon<double> &poly);

  //! Let the compiler know that we will use these classes with the following types
  template class Polygon<float>;
  template class Polygon<double>;

}// namespace Ravl2
