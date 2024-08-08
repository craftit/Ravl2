// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_FONT_HEADER
#define RAVLIMAGE_FONT_HEADER 1
////////////////////////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! lib=RavlImage
//! author="Charles Galambos"
//! docentry="Ravl.API.Images.Drawing"
//! file="Ravl/Image/Base/Font.hh"
//! example=exFont.cc

#include "Ravl/SArray1d.hh"
#include "Ravl/Image/Image.hh"
#include "Ravl/Array2dIter2.hh"

namespace RavlImageN {
  
  //! userlevel=Normal
  //: Font for rendering text.
  
  class FontC {
  public:
    FontC()
    {}
    //: Default constructor.
    // Creates an empty font.
    
    explicit FontC(bool);
    //: Load default font.
    
    FontC(SArray1dC<ImageC<ByteT> > &nGlyphs)
      : glyphs(nGlyphs)
    {}
    //: Constructor from array of image.
    
    ImageC<ByteT> &operator[](IntT let)
    { return glyphs[let]; }
    //: Access character.
    
    const ImageC<ByteT> &operator[](IntT let) const
    { return glyphs[let]; }
    //: Access character.
    
    SArray1dC<ImageC<ByteT> > &Glyphs()
    { return glyphs; }
    //: Access image array.
    
    const SArray1dC<ImageC<ByteT> > &Glyphs() const
    { return glyphs; }
    //: Access image array.
    
    bool IsValid() const
    { return glyphs.Size() != 0; }
    //: Is this a valid font.
    
    Index2dC Center(const StringC &text) const;
    //: Get the offset to the centre of the string.
    
    Index2dC Size(const StringC &text) const;
    //: Compute the size of image required to render 'text'.
    
    UIntT Count() const
    { return glyphs.Size(); }
    //: Count the number of glyphs in the font.
    
  protected:
    SArray1dC<ImageC<ByteT> > glyphs;
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
		const Index2dC &offset,
		const StringC &text,
		ImageC<DataT> &image) 
  {
    RavlAssert(font.IsValid());
    Index2dC at(offset);
    const char *p = text.chars();
    const char *eos = &p[text.length()];
    for(;p != eos;p++) {
      const ImageC<ByteT> &glyph = font[(UIntT) *((UByteT *) p)];
      IndexRange2dC drawRect = glyph.Frame(); // Get rectangle.
      drawRect.SetOrigin(at); 
      drawRect.ClipBy(image.Frame());
      if(drawRect.Area() <= 0) 
	continue;
      for(Array2dIter2C<DataT,ByteT> it(Array2dC<DataT>(image,drawRect),glyph,false);it;it++) {
	if(it.Data2() != 0)
	  it.Data1() = value;
      }
      at.Col() += glyph.Frame().Cols();
    }
  }

  //: Draw text on image.  
  // Text is positioned below and to right of "offset".

  template<class DataT>
  void DrawTextCenter(const FontC &font,
		const DataT &value,
		const Index2dC &centre,
		const StringC &text,
		ImageC<DataT> &image) 
  { 
    RavlAssert(font.IsValid());
    DrawText(font,value,centre - font.Center(text),text,image); 
  }
  //: Draw text on image, centred.  
  // Text is centred on "centre".


  template<class DataT>
  void DrawText(ImageC<DataT> &image,
                const DataT &value,
		const Index2dC &offset,
		const StringC &text
		) 
  { DrawText(DefaultFont(), value, offset, text, image); }
  //: Draw text on image.  
  // Text is positioned below and to right of "offset", using the default font.

  template<class DataT>
  void DrawTextCenter(ImageC<DataT> &image,
                const DataT &value,
		const Index2dC &centre,
		const StringC &text
		) 
  { DrawTextCenter(DefaultFont(), value, centre, text, image); }
  //: Draw text on image, centred.  
  // Text is centred on "centre", using the default font.
}

#endif
