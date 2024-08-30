//
// Created by charles on 08/08/24.
//
// This file provides tools to be used with geometric transformations like Affine and Projection.

#pragma once

#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/Polygon.hh"

namespace Ravl2
{

  //! Generate a rotation matrix for a 2D rotation
  template <typename RealT>
  [[nodiscard]] constexpr Matrix<RealT, 2, 2> rotationMatrix2(RealT angle)
  {
    RealT c = std::cos(angle);
    RealT s = std::sin(angle);
    return Matrix<RealT, 2, 2> {{c, -s}, {s, c}};
  }

  //! Generate a rotation matrix for a 3D rotation
  template <typename RealT>
  [[nodiscard]] constexpr Matrix<RealT, 3, 3> rotationMatrix3(const Vector<RealT, 3> &axis, RealT angle)
  {
    RealT c = std::cos(angle);
    RealT s = std::sin(angle);
    RealT t = 1 - c;
    RealT x = axis(0);
    RealT y = axis(1);
    RealT z = axis(2);
    return Matrix<RealT, 3, 3> {
      {t * x * x + c, t * x * y - s * z, t * x * z + s * y},
      {t * x * y + s * z, t * y * y + c, t * y * z - s * x},
      {t * x * z - s * y, t * y * z + s * x, t * z * z + c}};
  }

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