// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_FONT_HEADER
#define RAVLIMAGE_FONT_HEADER 1
////////////////////////////////////////////////////////////////////////////
//! author="Charles Galambos"
//! docentry="Ravl.API.Images.Drawing"
//! example=exFont.cc

#include <vector>
#include "Ravl2/Array.hh"
#include "Ravl2/Array2dIter2.hh"

namespace Ravl2 {
  
  //: Font for rendering text.
  
  class FontC {
  public:
    FontC()
    {}
    //: Default constructor.
    // Creates an empty font.
    
    explicit FontC(bool);
    //: Load default font.
    
    FontC(std::vector<Array<ByteT,2> > &nGlyphs)
      : glyphs(nGlyphs)
    {}
    //: Constructor from array of image.
    
    Array<ByteT,2> &operator[](IntT let)
    { return glyphs[let]; }
    //: Access character.
    
    const Array<ByteT,2> &operator[](IntT let) const
    { return glyphs[let]; }
    //: Access character.
    
    std::vector<Array<ByteT,2> > &Glyphs()
    { return glyphs; }
    //: Access image array.
    
    const std::vector<Array<ByteT,2> > &Glyphs() const
    { return glyphs; }
    //: Access image array.
    
    bool IsValid() const
    { return glyphs.size() != 0; }
    //: Is this a valid font.
    
    Index<2> Center(const StringC &text) const;
    //: Get the offset to the centre of the string.
    
    Index<2> Size(const StringC &text) const;
    //: Compute the size of image required to render 'text'.
    
    unsigned Count() const
    { return glyphs.size(); }
    //: Count the number of glyphs in the font.
    
  protected:
    std::vector<Array<ByteT,2> > glyphs;
  };
  
  FontC LoadPSF1(const StringC &fontFile);
  //: Load PSF1 font.
  // If the file is not recognised an invalid FontC will be returned
  
  FontC LoadPSF2(const StringC &fontFile);
  //: Load PSF2 font.
  // If the file is not recognised an invalid FontC will be returned
  
  FontC &DefaultFont();
  //: Access default font.
  
  template<class DataT>
  void DrawText(const FontC &font,
		const DataT &value,
		const Index<2> &offset,
		const StringC &text,
		Array<DataT,2> &image) 
  {
    RavlAssert(font.IsValid());
    Index<2> at(offset);
    const char *p = text.chars();
    const char *eos = &p[text.length()];
    for(;p != eos;p++) {
      const Array<ByteT,2> &glyph = font[(unsigned) *((UByteT *) p)];
      IndexRange<2> drawRect = glyph.range(); // Get rectangle.
      drawRect.SetOrigin(at); 
      drawRect.clipBy(image.range());
      if(drawRect.area() <= 0) 
	continue;
      for(Array2dIter2C<DataT,ByteT> it(Array<DataT,2>(image,drawRect),glyph,false);it;it++) {
	if(it.data<1>() != 0)
	  it.data<0>() = value;
      }
      at[1] += glyph.range().range().size(1);
    }
  }

  //: Draw text on image.  
  // Text is positioned below and to right of "offset".

  template<class DataT>
  void DrawTextCenter(const FontC &font,
		const DataT &value,
		const Index<2> &centre,
		const StringC &text,
		Array<DataT,2> &image) 
  { 
    RavlAssert(font.IsValid());
    DrawText(font,value,centre - font.Center(text),text,image); 
  }
  //: Draw text on image, centred.  
  // Text is centred on "centre".


  template<class DataT>
  void DrawText(Array<DataT,2> &image,
                const DataT &value,
		const Index<2> &offset,
		const StringC &text
		) 
  { DrawText(DefaultFont(), value, offset, text, image); }
  //: Draw text on image.  
  // Text is positioned below and to right of "offset", using the default font.

  template<class DataT>
  void DrawTextCenter(Array<DataT,2> &image,
                const DataT &value,
		const Index<2> &centre,
		const StringC &text
		) 
  { DrawTextCenter(DefaultFont(), value, centre, text, image); }
  //: Draw text on image, centred.  
  // Text is centred on "centre", using the default font.
}

#endif
