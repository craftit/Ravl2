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

namespace Ravl2
{

  //: Font for rendering text.

  class BitmapFont
  {
  public:
    //: Default constructor.
    // Creates an empty font.
    BitmapFont() = default;

    //: Constructor from array of image.
    explicit BitmapFont(std::vector<Array<uint8_t, 2>> &nGlyphs)
        : glyphs(nGlyphs)
    {}

    //: Access character.
    Array<uint8_t, 2> &operator[](size_t let)
    {
      return glyphs[let];
    }

    //: Access character.
    const Array<uint8_t, 2> &operator[](size_t let) const
    {
      return glyphs[let];
    }

    //: Access image array.
    std::vector<Array<uint8_t, 2>> &Glyphs()
    {
      return glyphs;
    }

    //: Access image array.
    const std::vector<Array<uint8_t, 2>> &Glyphs() const
    {
      return glyphs;
    }

    //: Is this a valid font.
    bool IsValid() const
    {
      return glyphs.size() != 0;
    }

    //: Get the offset to the centre of the string.
    Index<2> Center(const std::string &text) const;

    //: Compute the size of image required to render 'text'.
    Index<2> Size(const std::string &text) const;

    //: Count the number of glyphs in the font.
    auto Count() const
    {
      return glyphs.size();
    }

  protected:
    std::vector<Array<uint8_t, 2>> glyphs;
  };

  //: Load PSF1 font.
  // If the file is not recognised an invalid BitmapFont will be returned
  BitmapFont LoadPSF1(const std::string &fontFile);

  //: Load PSF2 font.
  // If the file is not recognised an invalid BitmapFont will be returned
  BitmapFont LoadPSF2(const std::string &fontFile);

  //: Access default font.
  BitmapFont &DefaultFont();

}// namespace Ravl2
