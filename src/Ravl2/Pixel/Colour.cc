
#include "Ravl2/Pixel/Colour.hh"
#include "Ravl2/IO/TypeConverter.hh"

// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik,Bill Christmas,Charles Galambos"

#include "Ravl2/Pixel/Colour.hh"

namespace Ravl2
{
  void initColourConversion()
  {
    // This is a no-op function to ensure that the colour conversion tables are initialised.
  }

  /*
   From RGB to YUV, the definition seems to be (by Google consensus)

   Y = 0.299R + 0.587G + 0.114B
   U = 0.492 (B-Y)
   V = 0.877 (R-Y)
  */

  const Matrix<float, 3, 3> ColorConversion::mImageRGBtoYUVMatrixStd(
    {{0.2990000000000f, 0.5870000000000f, 0.1140000000000f},
     {-0.1471080000000f, -0.2888040000000f, 0.4359120000000f},
     {0.6147770000000f, -0.5147990000000f, -0.0999780000000f}});

  // So from YUV to RGB should just be the inverse:

  const Matrix<float, 3, 3> ColorConversion::mImageYUVtoRGBMatrix(
    {{1.0000000000000f, 0.0000000000000f, 1.1402508551881f},
     {1.0000000000000f, -0.3947313749117f, -0.5808092090311f},
     {1.0000000000000f, 2.0325203252033f, 0.0000000000000f}});

  const Matrix<float, 3, 3> ColorConversion::mImageRGBtoYUVMatrix = ColorConversion::mImageRGBtoYUVMatrixStd * ((1. / 1.175));

  namespace
  {
    constexpr auto
    UBLookup()
    {
      std::array<int, 256> values {};
      auto *off = &(values[128]);
      for(int i = -128; i < 128; i++)
        off[i] = int_round(float(i) * 2.0325203252033f);
      return values;
    }

    constexpr auto VRLookup()
    {
      std::array<int, 256> values {};
      auto *off = &(values[128]);
      for(int i = -128; i < 128; i++)
        off[i] = int_round(float(i) * 1.1402508551881f);
      return values;
    }

    constexpr auto UVGLookup()
    {
      std::array<int, 256 * 256> values {};
      for(int u = 0; u < 256; u++)
        for(int v = 0; v < 256; v++)
          values[unsigned(u + 256 * v)] = int_round(float(u - 128) * -0.3947313749117f + float(v - 128) * -0.5808092090311f);
      //return &(values[128 + 256 * 128]);
      return values;
    }

    [[maybe_unused]] bool g_reg1 = registerConversion(convert<Array<PixelY8,2>,Array<PixelYUV8,2>>, 0.33f);
    [[maybe_unused]] bool g_reg2 = registerConversion(convert<Array<PixelY8,2>,Array<PixelRGB8,2>>, 0.33f);
    [[maybe_unused]] bool g_reg3 = registerConversion(convert<Array<PixelRGB8,2>,Array<PixelYUV8,2>>, 0.75f);
    [[maybe_unused]] bool g_reg4 = registerConversion(convert<Array<PixelYUV8,2>,Array<PixelRGB8,2>>, 0.75f);
    [[maybe_unused]] bool g_reg5 = registerConversion(convert<Array<PixelYUV8,2>,Array<PixelY8,2>>, 1.0f);
    [[maybe_unused]] bool g_reg6 = registerConversion(convert<Array<PixelRGB8,2>,Array<PixelY8,2>>, 1.0f);
    [[maybe_unused]] bool g_reg7 = registerConversion(convert<Array<PixelRGB8,2>,Array<PixelBGR8,2>>, 1.0f);
    [[maybe_unused]] bool g_reg8 = registerConversion(convert<Array<PixelBGR8,2>,Array<PixelRGB8,2>>, 1.0f);
    [[maybe_unused]] bool g_reg9 = registerConversion(convert<Array<PixelRGB8,2>,Array<PixelRGBA8,2>>, 1.0f);
    [[maybe_unused]] bool g_reg10 = registerConversion(convert<Array<PixelBGRA8,2>,Array<PixelRGBA8,2>>, 1.0f);
    [[maybe_unused]] bool g_reg11 = registerConversion(convert<Array<PixelBGRA8,2>,Array<PixelBGRA8,2>>, 1.0f);
    [[maybe_unused]] bool g_reg12 = registerConversion(convert<Array<PixelD32F,2>,Array<PixelD16,2>>, 1.0f);

  }// namespace

  const std::array<int, 256> ColorConversion::mRGBcYUV_ubLookup = UBLookup();
  const std::array<int, 256> ColorConversion::mRGBcYUV_vrLookup = VRLookup();
  const std::array<int, 256 * 256> ColorConversion::mRGBcYUV_uvgLookup = UVGLookup();

  template void convert(Array<PixelY8, 2> &dest, const Array<PixelYUV8, 2> &src);
  template void convert(Array<PixelY8, 2> &dest, const Array<PixelRGB8, 2> &src);
  template void convert(Array<PixelYUV8, 2> &dest, const Array<PixelY8, 2> &src);
  template void convert(Array<PixelRGB8, 2> &dest, const Array<PixelYUV8, 2> &src);
  template void convert(Array<PixelYUV8, 2> &dest, const Array<PixelRGB8, 2> &src);
  template void convert(Array<PixelRGB8, 2> &dest, const Array<PixelY8, 2> &src);
  template void convert(Array<PixelRGB8, 2> &dest, const Array<PixelBGR8, 2> &src);
  template void convert(Array<PixelBGR8, 2> &dest, const Array<PixelRGB8, 2> &src);
  template void convert(Array<PixelRGB8, 2> &dest, const Array<PixelRGBA8, 2> &src);
  template void convert(Array<PixelBGRA8, 2> &dest, const Array<PixelRGBA8, 2> &src);
  template void convert(Array<PixelRGBA8, 2> &dest, const Array<PixelBGRA8, 2> &src);
  template void convert(Array<PixelD32F, 2> &dest, const Array<PixelD16, 2> &src);


}// namespace Ravl2
