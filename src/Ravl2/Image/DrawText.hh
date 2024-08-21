//
// Created by charles on 08/08/24.
//
// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! docentry="Ravl.API.Images.Drawing"
//! example=exFont.cc

#pragma once

#include <vector>
#include "Ravl2/Array.hh"
#include "Ravl2/Assert.hh"
#include "Ravl2/ArrayIterZip.hh"
#include "Ravl2/Image/BitmapFont.hh"
#include "Ravl2/Image/DrawMask.hh"

namespace Ravl2
{
  //! Draw text on image.
  //! Text is positioned below and to right of "offset".

  template <typename ArrayTargetT,
            typename ValueT,
            typename DataT = typename ArrayTargetT::value_type,
            unsigned N = ArrayTargetT::dimensions>
    requires WindowedArray<ArrayTargetT, DataT, N> && std::is_convertible_v<ValueT, DataT>
  void DrawText(ArrayTargetT &image,
                const BitmapFont &font,
                const ValueT &value,
                const Index<2> &offset,
                const std::string &text)
  {
    RavlAssert(font.IsValid());
    Index<2> at(offset);
    for(auto p : text) {
      const auto &glyph = font[uint8_t(p)];
      DrawMask(image, glyph, value, at);
      at[1] += glyph.range().range(1).size();
    }
  }

  //! Draw text on image, centred.
  //! Text is centred on "centre".

  template <typename ArrayTargetT,
            typename ValueT,
            typename DataT = typename ArrayTargetT::value_type,
            unsigned N = ArrayTargetT::dimensions>
    requires WindowedArray<ArrayTargetT, DataT, N> && std::is_convertible_v<ValueT, DataT>
  void DrawTextCenter(ArrayTargetT &image,
                      const BitmapFont &font,
                      const ValueT &value,
                      const Index<2> &centre,
                      const std::string &text)
  {
    RavlAssert(font.IsValid());
    DrawText(font, value, centre - font.Center(text), text, image);
  }

  //! Draw text on image.
  //! Text is positioned below and to right of "offset", using the default font.

  template <typename ArrayTargetT,
            typename ValueT,
            typename DataT = typename ArrayTargetT::value_type,
            unsigned N = ArrayTargetT::dimensions>
    requires WindowedArray<ArrayTargetT, DataT, N> && std::is_convertible_v<ValueT, DataT>
  void DrawText(ArrayTargetT &image,
                const ValueT &value,
                const Index<2> &offset,
                const std::string &text)
  {
    DrawText(DefaultFont(), value, offset, text, image);
  }

  //! Draw text on image, centred.
  //! Text is centred on "centre", using the default font.
  template <typename ArrayTargetT,
            typename ValueT,
            typename DataT = typename ArrayTargetT::value_type,
            unsigned N = ArrayTargetT::dimensions>
    requires WindowedArray<ArrayTargetT, DataT, N> && std::is_convertible_v<ValueT, DataT>
  void DrawTextCenter(ArrayTargetT &image,
                      const ValueT &value,
                      const Index<2> &centre,
                      const std::string &text)
  {
    DrawTextCenter(image, DefaultFont(), value, centre, text);
  }
}// namespace Ravl2
