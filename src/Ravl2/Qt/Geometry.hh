//
// Created by charles galambos on 04/08/2024.
//

#pragma once

#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtGui/QPolygon>

#include "Ravl2/Qt/Index.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/Polygon2d.hh"
#include "Ravl2/Geometry/LinePP2d.hh"

namespace Ravl2
{
  //! Convert a QRectF to a Range
  template <typename RealT = float>
  inline constexpr auto toRange(const QRectF &rect)
  {
    return Range<RealT, 2>(toPoint<float>(rect.top(), rect.left()), toPoint<float>(rect.bottom(), rect.right()));
  }

  //! Convert a Range to a QRectF
  template <typename RealT>
  inline constexpr auto toQRectF(const Range<RealT, 2> &range)
  {
    return QRectF(qreal(range.min(1)), qreal(range.min(0)), qreal(range.size(1)), qreal(range.size(0)));
  }

  //! Convert a Point to a QPointF
  template <typename RealT>
  inline constexpr auto toQPointF(const Point<RealT, 2> &point)
  {
    return QPointF(qreal(point[1]), qreal(point[0]));
  }

  //! Convert a Point to a QPoint
  template <typename IndexT>
  inline constexpr auto toQPoint(const Point<IndexT, 2> &point)
  {
    return QPoint(point[1], point[0]);
  }

  //! Convert a QPointF to a Point
  template <typename RealT = float>
  inline constexpr auto toPoint(const QPointF &point)
  {
    return toPoint<RealT>(point.y(), point.x());
  }

  //! Convert a QPolygon to a Polygon
  template <typename RealT = float>
  inline constexpr auto toPolygon(const QPolygonF &polygon)
  {
    Polygon2dC<RealT> result;
    for(const auto &point : polygon) {
      result.push_back(toPoint<RealT>(point));
    }
    return result;
  }

  //! Convert a QPolygon to a Polygon
  template <typename IndexT = int>
  inline constexpr auto toPolygon(const QPolygon &polygon)
  {
    Polygon2dC<IndexT> result;
    for(const auto &point : polygon) {
      result.push_back(toPoint(point));
    }
    return result;
  }

  //! Convert an integer Polygon to a QPolygon
  template <typename IndexT>
    requires std::is_integral<IndexT>::value
  inline auto toQPolygon(const Polygon2dC<IndexT> &polygon)
  {
    QPolygon result;
    for(const auto &point : polygon) {
      result << toQPoint(point);
    }
    return result;
  }

  //! Convert a real Polygon to a QPolygonF
  template <typename RealT>
    requires std::is_floating_point<RealT>::value
  inline auto toQPolygon(const Polygon2dC<RealT> &polygon)
  {
    QPolygonF result;
    for(const auto &point : polygon) {
      result << toQPointF(point);
    }
    return result;
  }

  // Declare some common instantiations
  extern template auto toQPolygon<int>(const Polygon2dC<int> &polygon);
  extern template auto toQPolygon<float>(const Polygon2dC<float> &polygon);
  extern template auto toQPolygon<double>(const Polygon2dC<double> &polygon);

}// namespace Ravl2
