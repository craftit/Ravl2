//
// Created by charles galambos on 06/08/2024.
//

#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"
#include "Ravl2/IO/Cereal.hh"

namespace Ravl2
{
  // This helps ensure that the colour conversions are linked if the user uses any of the pixel classes.
  void initPixel()
  {
    initColourConversion();
  }


  //! Get name for a channel.
  std::string_view toString(ImageChannel channel)
  {
    switch(channel) {
      case ImageChannel::Red: return "Red";
      case ImageChannel::Green: return "Green";
      case ImageChannel::Blue: return "Blue";
      case ImageChannel::Alpha: return "Alpha";
      case ImageChannel::Luminance: return "Luminance";
      case ImageChannel::ChrominanceU: return "ChrominanceU";
      case ImageChannel::ChrominanceV: return "ChrominanceV";
      case ImageChannel::Intensity: return "Intensity";
      case ImageChannel::Luminance2: return "Luminance2";
      case ImageChannel::Hue: return "Hue";
      case ImageChannel::Saturation: return "Saturation";
      case ImageChannel::Value: return "Value";
      case ImageChannel::Lightness: return "Lightness";
      case ImageChannel::Count: return "Count";
      case ImageChannel::Label: return "Label";
      case ImageChannel::Depth: return "Depth";
      case ImageChannel::Signal: return "Signal";
      case ImageChannel::Unused: return "Unused";
    }
    return "Unknown";
  }

  std::ostream &operator<<(std::ostream &strm, const ImageChannel &channel)
  {
    strm << toString(channel);
    return strm;
  }

  std::istream &operator>>(std::istream &strm, ImageChannel &channel)
  {
    std::string str;
    strm >> str;
    if(str == "Red") channel = ImageChannel::Red;
    else if(str == "Green")
      channel = ImageChannel::Green;
    else if(str == "Blue")
      channel = ImageChannel::Blue;
    else if(str == "Alpha")
      channel = ImageChannel::Alpha;
    else if(str == "Luminance")
      channel = ImageChannel::Luminance;
    else if(str == "ChrominanceU")
      channel = ImageChannel::ChrominanceU;
    else if(str == "ChrominanceV")
      channel = ImageChannel::ChrominanceV;
    else if(str == "Intensity")
      channel = ImageChannel::Intensity;
    else if(str == "Luminance2")
      channel = ImageChannel::Luminance2;
    else if(str == "Hue")
      channel = ImageChannel::Hue;
    else if(str == "Saturation")
      channel = ImageChannel::Saturation;
    else if(str == "Value")
      channel = ImageChannel::Value;
    else if(str == "Lightness")
      channel = ImageChannel::Lightness;
    else if(str == "Count")
      channel = ImageChannel::Count;
    else if(str == "Label")
      channel = ImageChannel::Label;
    else if(str == "Depth")
      channel = ImageChannel::Depth;
    else if(str == "Signal")
      channel = ImageChannel::Signal;
    // else if(str == "Unused") channel = ImageChannel::Unused;
    else
      channel = ImageChannel::Unused;
    return strm;
  }

  template class Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
  template class Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
  template class Pixel<float, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
  template class Pixel<float, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
  template class Pixel<uint8_t, ImageChannel::Blue, ImageChannel::Green, ImageChannel::Red>;
  template class Pixel<uint8_t, ImageChannel::Blue, ImageChannel::Green, ImageChannel::Red, ImageChannel::Alpha>;
  template class Pixel<uint8_t, ImageChannel::Luminance, ImageChannel::ChrominanceU, ImageChannel::ChrominanceV>;
  template class Pixel<float, ImageChannel::Luminance, ImageChannel::ChrominanceU, ImageChannel::ChrominanceV>;


  // Also about arrays based on the pixel types
  template class Array<PixelI8,2>;
  template class Array<PixelY8,2>;
  template class Array<PixelY16,2>;
  template class Array<PixelZ16, 2>;
  template class Array<PixelZ32F, 2>;
  template class Array<PixelRGB8,2>;
  template class Array<PixelRGBA8,2>;
  template class Array<PixelRGB16,2>;
  template class Array<PixelRGBA16,2>;
  template class Array<PixelRGB32F,2>;
  template class Array<PixelRGBA32F,2>;
  template class Array<PixelBGR8,2>;
  template class Array<PixelBGRA8,2>;
  template class Array<PixelYUV8,2>;
  template class Array<PixelYUV32F,2>;

  namespace {
    [[maybe_unused]] bool g_reg1 = registerCerealFormats<Array<PixelRGB8,2>>();
    [[maybe_unused]] bool g_reg2 = registerCerealFormats<Array<PixelRGBA8,2>>();
    [[maybe_unused]] bool g_reg3 = registerCerealFormats<Array<PixelRGB16,2>>();
    [[maybe_unused]] bool g_reg4 = registerCerealFormats<Array<PixelRGBA16,2>>();
    [[maybe_unused]] bool g_reg5 = registerCerealFormats<Array<PixelY8,2>>();
    [[maybe_unused]] bool g_reg6 = registerCerealFormats<Array<PixelY16,2>>();
    [[maybe_unused]] bool g_reg7 = registerCerealFormats<Array<PixelYUV8,2>>();
    [[maybe_unused]] bool g_reg8 = registerCerealFormats<Array<PixelZ32F, 2>>();
    [[maybe_unused]] bool g_reg9 = registerCerealFormats<Array<PixelZ16, 2>>();
    [[maybe_unused]] bool g_reg10 = registerCerealFormats<Array<PixelRGB32F,2>>();
    [[maybe_unused]] bool g_reg11 = registerCerealFormats<Array<PixelRGBA32F,2>>();
    [[maybe_unused]] bool g_reg14 = registerCerealFormats<Array<PixelYUV32F,2>>();
    [[maybe_unused]] bool g_reg15 = registerCerealFormats<Array<PixelY32F,2>>();

    // Make the pixel type names more readable
    [[maybe_unused]] bool g_typeReg1 = registerTypeName(typeid(PixelI8),"Ravl2::PixelI8");
    [[maybe_unused]] bool g_typeReg2 = registerTypeName(typeid(PixelY8),"Ravl2::PixelY8");
    [[maybe_unused]] bool g_typeReg3 = registerTypeName(typeid(PixelY16),"Ravl2::PixelY16");
    [[maybe_unused]] bool g_typeReg4 = registerTypeName(typeid(PixelYUV8),"Ravl2::PixelYUV8");
    [[maybe_unused]] bool g_typeReg5 = registerTypeName(typeid(PixelRGB8),"Ravl2::PixelRGB8");
    [[maybe_unused]] bool g_typeReg6 = registerTypeName(typeid(PixelRGBA8),"Ravl2::PixelRGBA8");
    [[maybe_unused]] bool g_typeReg7 = registerTypeName(typeid(PixelRGB16),"Ravl2::PixelRGB16");
  }

}// namespace Ravl2