//
// Created by charles galambos on 04/08/2024.
//

#pragma once

#include <QtCore/QSize>
#include <QtCore/QRect>
#include <QtCore/QPoint>

#include "Ravl2/Index.hh"

namespace Ravl2
{
  //! Convert a QRect to an IndexRange
  inline constexpr auto toIndexRange(const QRect &rect) {
    return IndexRange<2>(Index<2>(rect.top(), rect.left()), Index<2>(rect.bottom(), rect.right()));
  }

  //! Convert an IndexRange to a QRect
  inline constexpr auto toQRect(const IndexRange<2> &range) {
    return QRect(range.min(1), range.min(0), range.size(1), range.size(0));
  }

  //! Convert an Index to a QPoint
  inline constexpr auto toQSize(const Index<2> &index) {
    return QSize(index[1], index[0]);
  }

  //! Convert a QSize to an Index
  inline constexpr auto toIndex(const QSize &size) {
    return Index<2>(size.height(), size.width());
  }

  //! Convert an Index to a QPoint
  inline constexpr auto toQPoint(const Index<2> &index) {
    return QPoint(index[1], index[0]);
  }

  //! Convert a QPoint to an Index
  inline constexpr auto toIndex(const QPoint &point) {
    return Index<2>(point.y(), point.x());
  }


}
