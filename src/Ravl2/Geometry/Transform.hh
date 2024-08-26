//
// Created by charles on 08/08/24.
//
// This file provides tools to be used with geometric transformations like Affine and Projection.

#pragma once

#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/Polygon2d.hh"

namespace Ravl2
{

  //! Transform a Range by a point transform to a PointSet

  template <typename RealT, typename TransformT, unsigned N>
    requires PointTransform<TransformT, RealT, N>
  [[nodiscard]] constexpr PointSet<RealT, N> transform(const TransformT &trans, const Range<RealT, N> &range)
  {
    // Go through each corner of the range and transform it
    PointSet<RealT, N> points;
    for(unsigned i = 0; i < (1 << N); i++) {
      Point<RealT, N> pnt;
      for(unsigned j = 0; j < N; j++) {
        pnt(j) = (i & (1 << j)) ? range.max(j) : range.min(j);
      }
      points.push_back(trans * pnt);
    }
    return points;
  }

  //! Compute the axis-aligned bounding box of a point set.

  template <typename RealT, unsigned N>
  [[nodiscard]] constexpr Range<RealT, N> boundingBox(const PointSet<RealT, N> &points)
  {
    Range<RealT, N> range = Range<RealT, N>::mostEmpty();
    for(const auto &pnt : points) {
      range.involve(pnt);
    }
    return range;
  }

  //! Transform a PointSet by a point transform
  template <typename RealT, typename TransformT, unsigned N>
    requires PointTransform<TransformT, RealT, N>
  [[nodiscard]] constexpr PointSet<RealT, N> transform(const TransformT &trans, const PointSet<RealT, N> &points)
  {
    PointSet<RealT, N> newPoints;
    for(const auto &pnt : points) {
      newPoints.push_back(trans * pnt);
    }
    return newPoints;
  }

}// namespace Ravl2