// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImage
//! file="Ravl/Image/Base/Font.cc"

#include "Ravl/Image/Font.hh"
#include "Ravl/Stream.hh"
#include "Ravl/Image/PSFFont.h"
#include "Ravl/SArray1d.hh"
#include "Ravl/SArray1dIter.hh"
#include "Ravl/Resource.hh"

/////////////////////////////////////
// Font file information

/*! rcsid="$Id$" */
/*! lib=RavlImage */
/*! license=own */
/*! file="Ravl/Image/Base/PSFFont.h" */

/* /// PSF1 /////////////////////////////////////////////////////////// */

#define PSF1_MAGIC0     0x36
#define PSF1_MAGIC1     0x04

#define PSF1_MODE512    0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE    0x05

#define PSF1_SEPARATOR  0xFFFF
#define PSF1_STARTSEQ   0xFFFE

struct psf1_header {
  unsigned char magic[2];     /* Magic number */
  unsigned char mode;         /* PSF font mode */
  unsigned char charsize;     /* Character size */
};

/* /// PSF2 ////////////////////////////////////////////////////////// */

#define PSF2_MAGIC0     0x72
#define PSF2_MAGIC1     0xb5
#define PSF2_MAGIC2     0x4a
#define PSF2_MAGIC3     0x86

/* bits used in flags */
#define PSF2_HAS_UNICODE_TABLE 0x01

/* max version recognized so far */
#define PSF2_MAXVERSION 0

/* UTF8 separators */
#define PSF2_SEPARATOR  0xFF
#define PSF2_STARTSEQ   0xFE

struct psf2_header {
  unsigned char magic[4];
  unsigned int version;
  unsigned int headersize;    /* offset of bitmaps in file */
  unsigned int flags;
  unsigned int length;        /* number of glyphs */
  unsigned int charsize;      /* number of bytes for each character */
  unsigned int height, width; /* max dimensions of glyphs */
  /* charsize = height * ((width + 7) / 8) */
};


/////////////////////////////////////


#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlImageN {

  //: Load the default font.
  
  static FontC LoadDefaultFont() {
    StringC fontName = Resource("RAVL/Fonts","default8x16.psf");
    FontC defaultFont = LoadPSF1(fontName);
    if(!defaultFont.IsValid())
      std::cerr << "WARNING: Can't load default font '" << fontName << "'\n";
    return defaultFont;
  }
  
  //: Access default font.
  
  FontC &DefaultFont() {
    static FontC defaultFont = LoadDefaultFont();
    return defaultFont;
  }
  
  //: Load default font.
  
  FontC::FontC(bool)
  { (*this) = DefaultFont(); }
  
  //: Get the offset to the center of the string.
  
  Index2dC FontC::Center(const StringC &text) const {
    return Size(text)/2;
  }

  //: Compute the size of image required to render 'text'.
  
  Index2dC FontC::Size(const StringC &text) const {
    const char *at = text.chars();
    const char *eos = &(at[text.length()]);
    IntT maxHeight = 0;
    IntT cols = 0;
    for(;at != eos;at++) {
      const IndexRange2dC &ind = glyphs[*at].Frame(); 
      if((IntT) ind.Rows() > maxHeight)
	maxHeight = ind.Rows();
      cols += ind.Cols();
    }
    return Index2dC(maxHeight,cols);    
  }

  ////////////////////////////////////////////////////////////////
  
  FontC LoadPSF1(const StringC &fontFile) {
    ONDEBUG(cerr << "LoadPSF1() Loading font " << fontFile << "\n");
    psf1_header hdr; //: psf file
    
    IStreamC inf(fontFile);
    if(!inf) {
      std::cerr << "LoadPSF1(), Failed to open font file '" << fontFile << "'\n";
      return FontC();
    }
    
    // Read the header.
    
    inf.read((char *) &hdr,sizeof(psf1_header));    
    if((hdr.magic[0] != PSF1_MAGIC0) || (hdr.magic[1] != PSF1_MAGIC1))
      return FontC(); // Not a PSF1 font.
    int height = hdr.charsize;
    int ng = 255;
    if(hdr.mode & PSF1_MODE512)
      ng = 512;
    
    SArray1dC<ImageC<ByteT> > glyphs(ng);
    SArray1dC<ByteT > buf(height);
    for(SArray1dIterC<ImageC<ByteT> > it(glyphs);it;it++) {
      // Read glyph
      ImageC<ByteT> img(height,8);
      *it = img;
      inf.read((char *) &(buf[0]),height);
      for(IntT i=0;i < height;i++) {
	int dat = buf[i]; 
	for(IntT j = 0;j < 8;j++) {
	  if((dat >> (7-j)) & 1) 
	    img[i][j]=255;
	  else
	    img[i][j]=0;
	}
      }
    }
    
    return FontC(glyphs);
  }

  //: Load PSF2 font.
  
  FontC LoadPSF2(const StringC &fontFile) {
    ONDEBUG(cerr << "LoadPSF2() Loading font " << fontFile << "\n");
    IStreamC inf(fontFile);
    if(!inf) {
      std::cerr << "LoadPSF2(), Failed to open font file '" << fontFile << "'\n";
      return FontC();
    }
    psf2_header hdr;    
    inf.read((char *) &hdr,sizeof(hdr));
    if(hdr.magic[0] != PSF2_MAGIC0 || hdr.magic[1] != PSF2_MAGIC1 ||
       hdr.magic[2] != PSF2_MAGIC2 || hdr.magic[3] != PSF2_MAGIC3) {
      return FontC(); // Not a PSF2 font.
    }
    
    // Should byteswap header here if needed.
    
    SArray1dC<ImageC<ByteT> > glyphs(hdr.length);
    SArray1dC<ByteT > buf(hdr.charsize); 
    inf.seekg(hdr.headersize);
    for(SArray1dIterC<ImageC<ByteT> > it(glyphs);it;it++) {
      // Read glyph
      ImageC<ByteT> img(hdr.height,hdr.width);
      *it = img;
      inf.read((char *) &(buf[0]),hdr.charsize);
      int at = 0;
      // The following loop could be much faster, will do
      // something about it if anyone is interested.
      for(UIntT i=0;i < hdr.height;i++) {
	for(UIntT j = 0;j < hdr.width;j++) {
	  char dat = buf[at + (j/8)];
	  if((dat >> ((7-j) % 8)) & 1) 
	    img[i][j]=255;
	  else
	    img[i][j]=0;
	}
        
        at += ((hdr.width + 7) / 8);
      }
    }
    
    return FontC(glyphs);
  }
  
}
