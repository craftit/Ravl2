// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////////////
//! author="Radek Marik,Bill Christmas"

#include "Ravl2/Pixel/RGBcYUV.hh"

namespace Ravl2
{

  /*
   From RGB to YUV, the definition seems to be (by Google consensus)

   Y = 0.299R + 0.587G + 0.114B
   U = 0.492 (B-Y)
   V = 0.877 (R-Y)
  */

  const Matrix<float, 3, 3> ImageRGBtoYUVMatrixStd(
    {{0.2990000000000f, 0.5870000000000f, 0.1140000000000f},
     {-0.1471080000000f, -0.2888040000000f, 0.4359120000000f},
     {0.6147770000000f, -0.5147990000000f, -0.0999780000000f}});

  // So from YUV to RGB should just be the inverse:

  const Matrix<float, 3, 3> ImageYUVtoRGBMatrix(
    {{1.0000000000000f, 0.0000000000000f, 1.1402508551881f},
     {1.0000000000000f, -0.3947313749117f, -0.5808092090311f},
     {1.0000000000000f, 2.0325203252033f, 0.0000000000000f}});

  const Matrix<float, 3, 3> ImageRGBtoYUVMatrix = ImageRGBtoYUVMatrixStd * ((1. / 1.175));
#if 0

  IntT *UBLookup() {
    static IntT values[256];
    IntT *off = &(values[128]);
    for(int i = -128;i < 128;i++)
      off[i] = int_round((float) i * 2.0325203252033);
    return off;
  }

  IntT *VRLookup() {
    static IntT values[256];
    IntT *off = &(values[128]);
    for(int i = -128;i < 128;i++)
      off[i] = int_round((float) i * 1.1402508551881);
    return off;
  }

  IntT *UVGLookup() {
    static IntT values[256 * 256];
    for(int u = 0;u < 256;u++)
      for(int v = 0;v < 256;v++)
	values[u + 256 * v] = int_round((float) (u-128) * -0.3947313749117 + (float) (v-128) * -0.5808092090311);
    return &(values[128 + 256 * 128]);
  }
  
  const IntT *RGBcYUV_ubLookup = UBLookup();
  const IntT *RGBcYUV_vrLookup = VRLookup();
  const IntT *RGBcYUV_uvgLookup = UVGLookup();
#endif
}// namespace Ravl2
