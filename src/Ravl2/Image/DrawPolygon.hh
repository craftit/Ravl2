// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="James Smith"
//! date="27/10/2002"

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawLine.hh"
#include "Ravl2/Geometry/Polygon.hh"
#include "Ravl2/Geometry/PolygonRasterIter.hh"

namespace Ravl2
{

  //! @brief Draw a filled polygon into the image
  //! @param dat The image to draw into
  //! @param value The value to draw
  //! @param poly The polygon to draw
  template <typename ArrayT, typename CoordT = float, typename DataT,typename PixelT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT, PixelT, 2> && std::is_convertible_v<DataT, PixelT>
  void DrawFilledPolygon(ArrayT &dat, const DataT &value, const Polygon<CoordT> &poly)
  {
    // Draw one-colour polygon
    for(PolygonRasterIter<CoordT> it(poly); it.valid(); ++it) {
      if(!dat.range(0).contains(it.row()))
        continue;
      IndexRange<1> range = clip(it.rowIndexRange(),dat.range(0));
      if(range.empty())
        continue;
      fill(clipUnsafe(dat[it.row()], range), PixelT(value));
    }
  }

  //! @brief Draw a poly line into the image.
  //! @param dat The image to draw into
  //! @param value The value to draw
  //! @param poly The polygon to draw

  template <typename ArrayT, typename CoordT = float, typename DataT, typename PixelT = ArrayT::value_type>
    requires WindowedArray<ArrayT, PixelT, 2> && std::is_convertible_v<DataT, PixelT>
  void DrawPolygon(ArrayT &dat, const DataT &value, const Polygon<CoordT> &poly)
  {
    // Draw individual lines
    auto end = poly.end();
    auto last = poly.back();
    for(auto it = poly.begin(); it != end; it++) {
      DrawLine(dat, PixelT(value), Line2PP<CoordT>(last, *it));
      last = *it;
    }
  }

  //! @brief Draw a filled, shaded polygon into the image
  //! This function requires that DataT has a working operator*(double) and += function
  //! @param dat The image to draw into
  //! @param values The pixel values to interpolate between, one per vertex of the polygon
  //! @param poly The polygon to draw

  template <typename ArrayT, typename CoordT = float, typename DataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT, DataT, 2>
  void DrawShadedPolygon(ArrayT &dat, const std::vector<DataT> &values, const Polygon<CoordT> &poly)
  {
    if(values.size() != poly.size())
      throw std::runtime_error("DrawPolygon: values.size() != poly.size()");
    if(poly.size() < 2)
      return;
    auto valuesEnd = values.end();
    // Draw shaded polygon
    for(PolygonRasterIter<CoordT> it(dat, poly); it; it++) {
      auto pnt = toPoint<float>(it.index());
      // Calculate barycentric coords
      auto coord = poly.BarycentricCoordinate(pnt);
      // Calculate interpolated value
      DataT value {};
      auto cit = coord.begin();
      auto vit = values.begin();
      while(vit != valuesEnd) {
        value += DataT(vit.Data() * cit.Data());
        cit++;
        vit++;
      }
      // Set value
      *it = value;
    }
  }

  //! @brief Draw a shaded line polygon into the image
  //! This function requires that DataT has a working operator*(double) function
  //! @param dat The image to draw into
  //! @param values The pixel values to interpolate between, one per vertex of the polygon
  //! @param poly The polygon to draw

  template <typename ArrayT, typename CoordT = float, typename DataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT, DataT, 2>
  void DrawPolygon(ArrayT &dat, const std::vector<DataT> &values, const Polygon<CoordT> &poly)
  {
    if(values.size() != poly.size())
      throw std::runtime_error("DrawPolygon: values.size() != poly.size()");
    if(poly.size() < 2)
      return;
    // Draw individual lines
    auto val = values.begin();
    auto last = poly.last();
    auto lastValue = values.last();
    for(auto pnt : poly) {
      DrawLine(dat, lastValue, *val, last, pnt);
      val++;
    }
  }

}// namespace Ravl2
