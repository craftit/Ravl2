// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="22/04/2002"

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Line2dIter.hh"
#include "Ravl2/Geometry/LinePP2d.hh"

namespace Ravl2
{

  //! @brief Draw a pixel wide line in an image.
  //! @param Dat The image to draw the line in.
  //! @param Value The value to set the pixels to.
  //! @param aLine The line to draw.
  template <typename ArrayT, typename CoordTypeT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  void DrawLine(ArrayT Dat, const DataT &Value, const LinePP2dC<CoordTypeT> &aLine)
  {
    LinePP2dC<CoordTypeT> line = aLine;
    if(!line.clipBy(toRange<CoordTypeT>(Dat.range())))
      return;
    for(Line2dIterC it(toIndex<2>(line.P1()), toIndex<2>(line.P2())); it; ++it)
      Dat[*it] = Value;
  }

  //! @brief Draw a pixel wide line between two pixels in an image.
  //! @param Dat The image to draw the line in.
  //! @param Value The value to set the pixels to.
  //! @param From The start pixel.
  //! @param To The end pixel.

  template <typename ArrayT, typename DataT = typename ArrayT::ValueT, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  void DrawLine(ArrayT Dat, const DataT &Value, const Index<2> &From, const Index<2> &To)
  {
    if(Dat.range().contains(From) && Dat.range().contains(To)) {
      // If both start and end are inside the image, all pixels in between are.
      for(Line2dIterC it(From, To); it; ++it) {
        Dat[*it] = Value;
      }
      return;
    }
    DrawLine(Dat, Value, LinePP2dC<float>(toPoint<float>(From), toPoint<float>(To)));
  }

  //! @brief Draw a pixel wide line in an image, shaded between two colours <code>valuefrom</code> and <code>valueto</code>
  //! This function requires that DataT has a working operator*(double) function
  //! @param Dat The image to draw the line in.
  //! @param ValueFrom The value to set the pixels to at the start of the line.
  //! @param ValueTo The value to set the pixels to at the end of the line.
  //! @param Line The line to draw.

  template <typename ArrayT, typename CoordTypeT, typename DataT = typename ArrayT::ValueT, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  void DrawLine(ArrayT &Dat, const DataT &ValueFrom, const DataT &ValueTo, const LinePP2dC<CoordTypeT> &Line)
  {
    LinePP2dC line = Line;
    Range<CoordTypeT, 2> frame(Dat.range().min(0), Dat.range().max(0), Dat.range().min(1), Dat.range().max(1));
    if(line.clipBy(frame)) {
      CoordTypeT length = line.Length();
      // If both start and end are inside the image, all pixels in between are.
      for(Line2dIterC it(line.P1(), line.P2()); it; ++it) {
        CoordTypeT alpha = euclidDistance(it.Data(), toIndex<2>(line.P1())) / length;
        Dat[*it] = DataT((ValueFrom * (1 - alpha)) + (ValueTo * alpha));
      }
    }
  }

  //! @brief  Draw a line in an image, shaded between two colours <code>valuefrom</code> and <code>valueto</code>
  //! This function requires that DataT has a working operator*(double) function
  //! @param dat The image to draw the line in.
  //! @param valueFrom The value to set the pixels to at the start of the line.
  //! @param valueTo The value to set the pixels to at the end of the line.
  //! @param from The start pixel.
  //! @param to The end pixel.

  template <typename ArrayT, typename CoordTypeT = float, typename DataT = typename ArrayT::ValueT, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  void DrawLine(ArrayT &dat, const DataT &valuefrom, const DataT &valueto, const Index<2> &from, const Index<2> &to)
  {
    DrawLine(dat, valuefrom, valueto, LinePP2dC<CoordTypeT>(toPoint<CoordTypeT>(from), toPoint<CoordTypeT>(to)));
  }
}// namespace Ravl2
