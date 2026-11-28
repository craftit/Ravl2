// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include <algorithm>
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
    this->push_back(range.min());
    if(orientation == BoundaryOrientationT::INSIDE_LEFT) {
      // Clockwise
      this->push_back(toPoint<RealT>(range[0].max(), range[1].min()));
      this->push_back(range.max());
      this->push_back(toPoint<RealT>(range[0].min(), range[1].max()));
    } else {
      // Counterclockwise
      this->push_back(toPoint<RealT>(range[0].min(), range[1].max()));
      this->push_back(range.max());
      this->push_back(toPoint<RealT>(range[0].max(), range[1].min()));
    }
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
      for(auto &ptr : *this) {
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
    const size_t size = this->size();
    if(size == 0) return false;
    if(size == 1) return p == this->front();

    // Points on the boundary count as inside.
    auto pLast = this->back();
    for(const auto &k : *this) {
      if(Line2PP<RealT>(pLast, k).IsPointIn(p, RealT(0)))
        return true;
      pLast = k;
    }

    // Crossing number along a ray from p in the +dimension-0 direction. Each edge is treated as half-open in
    // dimension 1, so a vertex on the ray is counted once and edges along the ray are skipped; this handles
    // the vertex and collinear-edge cases without special-casing them.
    bool inside = false;
    pLast = this->back();
    for(const auto &k : *this) {
      const bool aAbove = pLast[1] > p[1];
      const bool bAbove = k[1] > p[1];
      if(aAbove != bAbove) {
        const RealT crossing = pLast[0] + (p[1] - pLast[1]) * (k[0] - pLast[0]) / (k[1] - pLast[1]);
        if(p[0] < crossing)
          inside = !inside;
      }
      pLast = k;
    }
    return inside;
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

  namespace
  {
    //! Twice the signed area of triangle (a, b, c); positive when c is to the left of a->b.
    template <typename RealT>
    RealT orientation(const Point<RealT, 2> &a, const Point<RealT, 2> &b, const Point<RealT, 2> &c)
    {
      return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    }

    //! For c collinear with a->b, is it within the segment's bounding box?
    template <typename RealT>
    bool withinSegmentBox(const Point<RealT, 2> &a, const Point<RealT, 2> &b, const Point<RealT, 2> &c)
    {
      return std::min(a[0], b[0]) <= c[0] && c[0] <= std::max(a[0], b[0]) && std::min(a[1], b[1]) <= c[1] && c[1] <= std::max(a[1], b[1]);
    }

    //! Do the closed segments p1-p2 and q1-q2 cross or touch?
    template <typename RealT>
    bool segmentsIntersect(const Point<RealT, 2> &p1, const Point<RealT, 2> &p2, const Point<RealT, 2> &q1, const Point<RealT, 2> &q2)
    {
      const RealT d1 = orientation(q1, q2, p1);
      const RealT d2 = orientation(q1, q2, p2);
      const RealT d3 = orientation(p1, p2, q1);
      const RealT d4 = orientation(p1, p2, q2);
      if(((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;
      return (d1 == 0 && withinSegmentBox(q1, q2, p1)) || (d2 == 0 && withinSegmentBox(q1, q2, p2))
        || (d3 == 0 && withinSegmentBox(p1, p2, q1)) || (d4 == 0 && withinSegmentBox(p1, p2, q2));
    }
  }// namespace

  template <typename RealT>
  bool Polygon<RealT>::isSelfIntersecting() const
  {
    // Compare every pair of non-adjacent sides as segments. Triangles and smaller can't self-intersect.
    const size_t n = this->size();
    if(n < 4)
      return false;
    const auto &pts = *this;
    for(size_t i = 0; i < n; ++i) {
      const auto &a1 = pts[i];
      const auto &a2 = pts[(i + 1) % n];
      for(size_t j = i + 2; j < n; ++j) {
        if(i == 0 && j == n - 1)
          continue;// sides n-1 and 0 share vertex 0
        if(segmentsIntersect(a1, a2, pts[j], pts[(j + 1) % n]))
          return true;
      }
    }
    return false;
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

  namespace
  {
    //! Cross product (O->A) x (O->B); >0 = left turn in the (axis0, axis1) basis.
    template <typename RealT>
    RealT hullCross(const Point<RealT, 2> &o, const Point<RealT, 2> &a, const Point<RealT, 2> &b)
    {
      return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
    }
  }// namespace

  //! Andrew's monotone-chain convex hull, O(n log n). The result is normalised to
  //! INSIDE_LEFT (positive signed area) so it composes with clipByConvex/overlap and
  //! matches the orientation PolyAnnotation gives ground-truth polygons on load.
  //! Fewer than three points are returned unchanged (no hull is defined).
  template <typename RealT>
  Polygon<RealT> ConvexHull(std::vector<Point<RealT, 2>> &&points)
  {
    const long n = static_cast<long>(points.size());
    if(n < 3) {
      return Polygon<RealT>(std::move(points));
    }
    std::sort(points.begin(), points.end(), [](const Point<RealT, 2> &a, const Point<RealT, 2> &b) {
      return a[0] < b[0] || (a[0] == b[0] && a[1] < b[1]);
    });
    std::vector<Point<RealT, 2>> hull(static_cast<size_t>(2 * n));
    long k = 0;
    for(long i = 0; i < n; ++i) {// lower hull
      while(k >= 2 && hullCross<RealT>(hull[size_t(k - 2)], hull[size_t(k - 1)], points[size_t(i)]) <= 0) {
        --k;
      }
      hull[size_t(k++)] = points[size_t(i)];
    }
    const long lower = k + 1;
    for(long i = n - 2; i >= 0; --i) {// upper hull
      while(k >= lower && hullCross<RealT>(hull[size_t(k - 2)], hull[size_t(k - 1)], points[size_t(i)]) <= 0) {
        --k;
      }
      hull[size_t(k++)] = points[size_t(i)];
    }
    hull.resize(size_t(k - 1));// last point repeats the first
    Polygon<RealT> result(std::move(hull));
    if(result.area() < 0) {
      result = result.reverse();
    }
    return result;
  }

  template <typename RealT>
  Polygon<RealT> ConvexHull(const std::vector<Point<RealT, 2>> &points)
  {
    std::vector<Point<RealT, 2>> copy(points);
    return ConvexHull<RealT>(std::move(copy));
  }

  // Instantiate the template for the types we will use.
  template Moments2<float> moments(const Polygon<float> &poly);
  template Moments2<double> moments(const Polygon<double> &poly);

  template Polygon<float> ConvexHull(std::vector<Point<float, 2>> &&points);
  template Polygon<double> ConvexHull(std::vector<Point<double, 2>> &&points);
  template Polygon<float> ConvexHull(const std::vector<Point<float, 2>> &points);
  template Polygon<double> ConvexHull(const std::vector<Point<double, 2>> &points);

  //! Let the compiler know that we will use these classes with the following types
  template class Polygon<float>;
  template class Polygon<double>;

}// namespace Ravl2
