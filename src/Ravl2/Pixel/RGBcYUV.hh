// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_RGBCYUV_HEADER
#define RAVL_RGBCYUV_HEADER 1
/////////////////////////////////////////////////////////////////////////

#include "Ravl2/Pixel/YUVValue.hh"
#include "Ravl2/Pixel/PixelRGB.hh"

namespace Ravl2
{
  extern const Matrix<float, 3, 3> ImageYUVtoRGBMatrix;
  // Matrix to convert YUV values to RGB.

  extern const Matrix<float, 3, 3> ImageRGBtoYUVMatrixStd;
  // Matrix to convert YUV values to RGB.

  extern const Matrix<float, 3, 3> ImageRGBtoYUVMatrix;
  // Matrix to convert YUV values to RGB.

#if 0
  inline RealYUVValueC::RealYUVValueC(const RealRGBValueC &v) 
  { Mul(ImageRGBtoYUVMatrixStd,v,*this); }
  
  inline RealRGBValueC::RealRGBValueC(const RealYUVValueC &v) 
  { Mul(ImageYUVtoRGBMatrix,v,*this); }


  extern const IntT *RGBcYUV_ubLookup;
  extern const IntT *RGBcYUV_vrLookup;
  extern const IntT *RGBcYUV_uvgLookup;
  
  inline void ByteYUV2RGB(ByteT y,SByteT u,SByteT v,ByteRGBValueC &pix) {
    IntT iy = (IntT) y;
    IntT iu = (IntT) u;
    IntT rv = (IntT) v;
    IntT tmp;
    tmp = iy + RGBcYUV_vrLookup[v];
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix[0] = (ByteT) tmp;
    
    tmp = iy + RGBcYUV_uvgLookup[iu + 256 * rv];
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix[1] = (ByteT) tmp;
    
    tmp = iy + RGBcYUV_ubLookup[u];
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix[2] = (ByteT) tmp;
  }
  //: Convert byte YUV values to byte RGB value.
  
  inline void ByteYUV2RGB2(ByteT y1,ByteT y2,SByteT u,SByteT v,ByteRGBValueC &pix1,ByteRGBValueC &pix2) {
    IntT iy1 = (IntT) y1;
    IntT iy2 = (IntT) y2;
    IntT iu = (IntT) u;
    IntT iv = (IntT) v;
    
    IntT tmp;
    IntT vr = RGBcYUV_vrLookup[iv];
    tmp = iy1 + vr;
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix1[0] = (ByteT) tmp;
    
    tmp = iy2 + vr;
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix2[0] = (ByteT) tmp;
    
    IntT uvg = RGBcYUV_uvgLookup[iu + 256 * iv];
    
    tmp = iy1 + uvg;
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix1[1] = (ByteT) tmp;
    
    tmp = iy2 + uvg;
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix2[1] = (ByteT) tmp;
    
    IntT ub = RGBcYUV_ubLookup[iu];
    
    tmp = iy1 + ub;
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix1[2] = (ByteT) tmp;
    
    tmp = iy2 + ub;
    if(RAVL_EXPECT(tmp < 0,0)) tmp = 0;
    if(RAVL_EXPECT(tmp > 255,0)) tmp = 255;
    pix2[2] = (ByteT) tmp;
  }
  //: Convert byte YUV422 values to byte RGB value.
#endif
}// namespace Ravl2

#endif
