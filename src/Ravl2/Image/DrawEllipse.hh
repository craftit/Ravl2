// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2004, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include <numbers>
#include "Ravl2/Geometry/Ellipse2d.hh"
#include "Ravl2/Image/DrawPolygon.hh"

namespace Ravl2
{
  //! @brief Draw an ellipse.
  //! @param  image - Image to draw into.
  //! @param  value - Value to draw.
  //! @param  ellipse - Ellipse to draw.
  //! This breaks the ellipse into 30 segments and draws as polygon. It could do
  //! with a better way of choosing this number.

  template <class DataT, typename RealT>
  void DrawEllipse(Array<DataT, 2> &image, const DataT &value, const Ellipse2dC<RealT> &ellipse)
  {
    auto [major, minor] = ellipse.size();
    if((major + minor) < 3) {// Very small ?
      auto at = toIndex(ellipse.Centre());
      if(image.range().contains(at)) {
        image[at] = value;
      }
      return;
    }
    RealT step = 2 * std::numbers::pi_v<RealT> / (major + minor);
    Polygon<RealT> poly;
    for(RealT a = 0; a < 2 * std::numbers::pi_v<RealT>; a += step)
      poly.push_back(ellipse.point(a));
    DrawPolygon(image, value, poly);
  }

  //! @brief Draw a filled ellipse.
  //! @param  image - Image to draw into.
  //! @param  value - Value to draw.
  //! @param  ellipse - Ellipse to draw.
  //! This breaks the ellipse into 30 segments and draws as polygon. It could do
  //! with a better way of choosing this number.

  template <class DataT, typename RealT>
  void DrawFilledEllipse(Array<DataT, 2> &image, const DataT &value, const Ellipse2dC<RealT> &ellipse)
  {
    RealT maj, min;
    ellipse.Size(maj, min);
    if((maj + min) < 3) {// Very small ?
      Index<2> at = ellipse.Centre();
      if(image.range().contains(at))
        image[at] = value;
      return;
    }
    RealT step = 2 * std::numbers::pi_v<RealT> / (maj + min);
    Polygon<RealT> poly;
    for(RealT a = 0; a < 2 * std::numbers::pi_v<RealT>; a += step)
      poly.push_back(ellipse.Point(a));
    DrawFilledPolygon(image, value, poly, fill);
  }

}// namespace Ravl2
