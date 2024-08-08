// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include <vector>
#include <exception>
#include <spdlog/spdlog.h>
#include <fstream>
#include "Ravl2/Image/BitmapFont.hh"
#include "Ravl2/Resource.hh"

/////////////////////////////////////

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

namespace Ravl2 {

  //: Load the default font.
  
  static BitmapFont LoadDefaultFont() {
    std::string fontName = findFileResource("fonts","default8x16.psf");
    if(fontName.empty()) {
      SPDLOG_ERROR("Can't find default font");
      throw std::runtime_error("Can't find default font");
    }
    BitmapFont defaultFont = LoadPSF1(fontName);
    if(!defaultFont.IsValid()) {
      SPDLOG_ERROR("Can't load default font '{}'", fontName);
      throw std::runtime_error("Can't load default font");
    }
    return defaultFont;
  }
  
  //: Access default font.
  
  BitmapFont &DefaultFont() {
    static BitmapFont defaultFont = LoadDefaultFont();
    return defaultFont;
  }
  
  //: Load default font.
  
  BitmapFont::BitmapFont(bool)
  { (*this) = DefaultFont(); }
  
  //: Get the offset to the center of the string.
  
  Index<2> BitmapFont::Center(const std::string &text) const {
    auto theSize = Size(text);
    return toIndex(theSize[0] / 2, theSize[1] / 2);
  }

  //: Compute the size of image required to render 'text'.
  
  Index<2> BitmapFont::Size(const std::string &text) const {
    const char *at = text.data();
    const char *eos = &(at[text.length()]);
    int maxHeight = 0;
    int cols = 0;
    for(;at != eos;at++) {
      const IndexRange<2> &ind = glyphs[size_t(*at)].range();
      if(ind.range(0).size() > maxHeight)
	maxHeight = ind.range(0).size();
      cols += ind.range(1).size();
    }
    return Index<2>({maxHeight,cols});
  }

  ////////////////////////////////////////////////////////////////
  
  BitmapFont LoadPSF1(const std::string &fontFile) {
    ONDEBUG(std::cerr << "LoadPSF1() Loading font " << fontFile << "\n");
    psf1_header hdr {}; //: psf file
    
    std::ifstream inf(fontFile);
    if(!inf) {
      SPDLOG_ERROR("Failed to open font file '{}'", fontFile);
      return BitmapFont();
    }
    
    // Read the header.
    
    inf.read(reinterpret_cast<char *>( &hdr),sizeof(psf1_header));
    if((hdr.magic[0] != PSF1_MAGIC0) || (hdr.magic[1] != PSF1_MAGIC1))
      return {}; // Not a PSF1 font.
    size_t height = hdr.charsize;
    size_t ng = 255;
    if(hdr.mode & PSF1_MODE512)
      ng = 512;
    
    std::vector<Array<uint8_t,2> > glyphs(ng);
    std::vector<uint8_t > buf(height);
    for(auto it: glyphs) {
      // Read glyph
      Array<uint8_t,2> img({height,8});
      it = img;
      inf.read(reinterpret_cast<char *>(&(buf[0])),std::streamsize(height));
      for(size_t i=0;i < height;++i) {
	int dat = buf[i]; 
	for(int j = 0;j < 8;++j) {
	  if((dat >> (7-j)) & 1) 
	    img[int(i)][j]=255;
	  else
	    img[int(i)][j]=0;
	}
      }
    }
    
    return BitmapFont(glyphs);
  }

  //: Load PSF2 font.
  
  BitmapFont LoadPSF2(const std::string &fontFile)
  {
    std::ifstream inf(fontFile);
    if(!inf) {
      SPDLOG_ERROR("Failed to open font file '{}'", fontFile);
      return {};
    }
    psf2_header hdr {};
    inf.read(reinterpret_cast<char *>(&hdr),sizeof(hdr));
    if(hdr.magic[0] != PSF2_MAGIC0 || hdr.magic[1] != PSF2_MAGIC1 ||
       hdr.magic[2] != PSF2_MAGIC2 || hdr.magic[3] != PSF2_MAGIC3) {
      SPDLOG_ERROR("Invalid magic number in font file '{}'", fontFile);
      return {}; // Not a PSF2 font.
    }
    
    // Should byteswap header here if needed.
    
    std::vector<Array<uint8_t,2> > glyphs(hdr.length);
    std::vector<uint8_t > buf(hdr.charsize); 
    inf.seekg(hdr.headersize);
    for(auto & it : glyphs) {
      // Read glyph
      Array<uint8_t,2> img({hdr.height,hdr.width});
      it = img;
      inf.read(reinterpret_cast<char *>(&(buf[0])),hdr.charsize);
      int at = 0;
      // The following loop could be much faster, will do
      // something about it if anyone is interested.
      for(int i=0;i < int(hdr.height);++i) {
	for(int j = 0;j < int(hdr.width);++j) {
	  auto dat = buf[size_t(at + (j/8))];
	  if((dat >> ((7-j) % 8)) & 1) 
	    img[i][j]=255;
	  else
	    img[i][j]=0;
	}
        at += int((hdr.width + 7) / 8);
      }
    }
    
    return BitmapFont(glyphs);
  }
  
}
