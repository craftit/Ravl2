//
// Created on 08/09/2025.
//

#include "Ravl2/Pixel/PixelPlane.hh"
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  // Call to ensure that the conversion functions are registered
  void initPlaneConversion()
  {}

  // Explicit instantiations for common plane types
  // 2D planes with various scaling factors - Luminance planes
  template class PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::Luminance, 2, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::Luminance, 2, 2>;
  template class PixelPlane<uint16_t, 2, ImageChannel::Luminance, 1, 1>;
  template class PixelPlane<uint16_t, 2, ImageChannel::Luminance, 2, 1>;
  template class PixelPlane<uint16_t, 2, ImageChannel::Luminance, 2, 2>;
  template class PixelPlane<float, 2, ImageChannel::Luminance, 1, 1>;
  template class PixelPlane<float, 2, ImageChannel::Luminance, 2, 1>;
  template class PixelPlane<float, 2, ImageChannel::Luminance, 2, 2>;

  // Chrominance planes
  template class PixelPlane<uint8_t, 2, ImageChannel::ChrominanceU, 1, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::ChrominanceU, 2, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::ChrominanceU, 2, 2>;
  template class PixelPlane<uint8_t, 2, ImageChannel::ChrominanceV, 1, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::ChrominanceV, 2, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::ChrominanceV, 2, 2>;

  // RGB planes
  template class PixelPlane<uint8_t, 2, ImageChannel::Red, 1, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::Green, 1, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::Blue, 1, 1>;
  template class PixelPlane<uint8_t, 2, ImageChannel::Alpha, 1, 1>;

  // 3D volumes
  template class PixelPlane<uint8_t, 3, ImageChannel::Red, 1, 1, 1>;
  template class PixelPlane<uint8_t, 3, ImageChannel::Green, 1, 1, 1>;
  template class PixelPlane<uint8_t, 3, ImageChannel::Blue, 1, 1, 1>;
  template class PixelPlane<uint16_t, 3, ImageChannel::Luminance, 1, 1, 1>;
  template class PixelPlane<float, 3, ImageChannel::Luminance, 1, 1, 1>;

  // Explicit instantiations for common planar image types
  // YUV444 format (4:4:4)
  template class PlanarImage<2,
    PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::ChrominanceU, 1, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::ChrominanceV, 1, 1>
  >;

  // YUV422 format (4:2:2)
  template class PlanarImage<2,
    PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::ChrominanceU, 2, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::ChrominanceV, 2, 1>
  >;

  // YUV420 format (4:2:0)
  template class PlanarImage<2,
    PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::ChrominanceU, 2, 2>,
    PixelPlane<uint8_t, 2, ImageChannel::ChrominanceV, 2, 2>
  >;

  // RGB format
  template class PlanarImage<2,
    PixelPlane<uint8_t, 2, ImageChannel::Red, 1, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::Green, 1, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::Blue, 1, 1>
  >;

  // 3D volume example
  template class PlanarImage<3,
    PixelPlane<uint8_t, 3, ImageChannel::Red, 1, 1, 1>,
    PixelPlane<uint8_t, 3, ImageChannel::Green, 1, 1, 1>,
    PixelPlane<uint8_t, 3, ImageChannel::Blue, 1, 1, 1>
  >;

  // Instantiate scaling helpers
  template struct PlaneScale<2, 1, 1>;
  template struct PlaneScale<2, 2, 1>;
  template struct PlaneScale<2, 2, 2>;
  template struct PlaneScale<3, 1, 1, 1>;

  // Explicit instantiations of conversion functions for common types

  // Explicitly instantiate the generic conversion function for common types
  // RGB conversions
  template auto convertToPlanar<2, PixelRGB8>(const Array<PixelRGB8, 2>& packedArray);
  template auto convertToPlanar<2, PixelRGBA8>(const Array<PixelRGBA8, 2>& packedArray);
  template auto convertToPlanar<2, PixelRGB16>(const Array<PixelRGB16, 2>& packedArray);
  template auto convertToPlanar<2, PixelRGBA16>(const Array<PixelRGBA16, 2>& packedArray);
  template auto convertToPlanar<2, PixelRGB32F>(const Array<PixelRGB32F, 2>& packedArray);
  template auto convertToPlanar<2, PixelRGBA32F>(const Array<PixelRGBA32F, 2>& packedArray);

  // BGR conversions
  template auto convertToPlanar<2, PixelBGR8>(const Array<PixelBGR8, 2>& packedArray);
  template auto convertToPlanar<2, PixelBGRA8>(const Array<PixelBGRA8, 2>& packedArray);

  // YUV conversions
  template auto convertToPlanar<2, PixelYUV8>(const Array<PixelYUV8, 2>& packedArray);
  template auto convertToPlanar<2, PixelYUVA8>(const Array<PixelYUVA8, 2>& packedArray);
  template auto convertToPlanar<2, PixelYUV32F>(const Array<PixelYUV32F, 2>& packedArray);

  // Luminance conversions
  template auto convertToPlanar<2, PixelY8>(const Array<PixelY8, 2>& packedArray);
  template auto convertToPlanar<2, PixelYA8>(const Array<PixelYA8, 2>& packedArray);
  template auto convertToPlanar<2, PixelY16>(const Array<PixelY16, 2>& packedArray);
  template auto convertToPlanar<2, PixelY32F>(const Array<PixelY32F, 2>& packedArray);

  [[maybe_unused]] bool g_reg1 = registerConversion(convertToPlanar<2, PixelRGB8>, 1.00f);
  [[maybe_unused]] bool g_reg2 = registerConversion(convertToPlanar<2, PixelRGBA8>, 1.00f);
  [[maybe_unused]] bool g_reg3 = registerConversion(convertToPlanar<2, PixelRGB16>, 1.00f);
  [[maybe_unused]] bool g_reg4 = registerConversion(convertToPlanar<2, PixelRGBA16>, 1.00f);
  [[maybe_unused]] bool g_reg5 = registerConversion(convertToPlanar<2, PixelRGB32F>, 1.00f);
  [[maybe_unused]] bool g_reg6 = registerConversion(convertToPlanar<2, PixelRGBA32F>, 1.00f);
  [[maybe_unused]] bool g_reg7 = registerConversion(convertToPlanar<2, PixelBGR8>, 1.00f);
  [[maybe_unused]] bool g_reg8 = registerConversion(convertToPlanar<2, PixelBGRA8>, 1.00f);
  [[maybe_unused]] bool g_reg9 = registerConversion(convertToPlanar<2, PixelYUV8>, 1.00f);
  [[maybe_unused]] bool g_reg10 = registerConversion(convertToPlanar<2, PixelYUVA8>, 1.00f);
  [[maybe_unused]] bool g_reg11 = registerConversion(convertToPlanar<2, PixelYUV32F>, 1.00f);
  [[maybe_unused]] bool g_reg12 = registerConversion(convertToPlanar<2, PixelY8>, 1.00f);
  [[maybe_unused]] bool g_reg13 = registerConversion(convertToPlanar<2, PixelYA8>, 1.00f);
  [[maybe_unused]] bool g_reg14 = registerConversion(convertToPlanar<2, PixelY16>, 1.00f);
  [[maybe_unused]] bool g_reg15 = registerConversion(convertToPlanar<2, PixelY32F>, 1.00f);

  // Helper to expand the channel pack at compile time
  namespace detail {
    template<typename PixelT, typename PlanarImageT, std::size_t... Is>
    auto convertToPackedPixelAtIndexImpl(const PlanarImageT& image, const Index<2>& idx, std::index_sequence<Is...>) {
      // Use Pixel directly instead of trying to access a non-existent pixel_type
      return image.template createPackedPixel<Pixel,
                                            typename PixelT::value_type,
                                            PixelT::template getChannelAtIndex<Is>()...>(idx);
    }

    template<typename PixelT, typename PlanarImageT>
    auto convertToPackedPixelAtIndex(const PlanarImageT& image, const Index<2>& idx) {
      return convertToPackedPixelAtIndexImpl<PixelT>(image, idx,
                                                  std::make_index_sequence<PixelT::channel_count>{});
    }
  }

  template<typename PixelT, typename PlanarImageT>
  Array<PixelT, 2> helperConvertToPacked(const PlanarImageT &image)
  {
    // Get the master range from the planar image
    auto masterRange = image.range();

    // Create the packed array with the same master range
    Array<PixelT, 2> result(masterRange);

    // Iterate through all coordinates in the master range
    for (auto it = result.begin(); it != result.end(); ++it) {
      const auto& idx = it.index();

      // Create a packed pixel from the planes at this position
      // Using the createPackedPixel method from PlanarImage with the proper channel expansion
      *it = detail::convertToPackedPixelAtIndex<PixelT>(image, idx);
    }

    return result;
  }

  // Explicitly instantiate the helperConvertToPacked function for common types
  // RGB conversions
  template Array<PixelRGB8, 2> helperConvertToPacked<PixelRGB8>(const RGBPlanarImage<uint8_t>& image);
  template Array<PixelRGBA8, 2> helperConvertToPacked<PixelRGBA8>(const RGBAPlanarImage<uint8_t>& image);
  template Array<PixelRGB16, 2> helperConvertToPacked<PixelRGB16>(const RGBPlanarImage<uint16_t>& image);
  template Array<PixelRGBA16, 2> helperConvertToPacked<PixelRGBA16>(const RGBAPlanarImage<uint16_t>& image);
  template Array<PixelRGB32F, 2> helperConvertToPacked<PixelRGB32F>(const RGBPlanarImage<float>& image);
  template Array<PixelRGBA32F, 2> helperConvertToPacked<PixelRGBA32F>(const RGBAPlanarImage<float>& image);

  // BGR conversions
  template Array<PixelBGR8, 2> helperConvertToPacked<PixelBGR8>(const RGBPlanarImage<uint8_t>& image);
  template Array<PixelBGRA8, 2> helperConvertToPacked<PixelBGRA8>(const RGBAPlanarImage<uint8_t>& image);

  // YUV conversions
  template Array<PixelYUV8, 2> helperConvertToPacked<PixelYUV8>(const YUV444Image<uint8_t>& image);
  template Array<PixelYUVA8, 2> helperConvertToPacked<PixelYUVA8>(const YUV444Image<uint8_t>& image);
  template Array<PixelYUV32F, 2> helperConvertToPacked<PixelYUV32F>(const YUV444Image<float>& image);

  // Luminance conversions
  template Array<PixelY8, 2> helperConvertToPacked<PixelY8>(const PlanarImage2D<PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>>& image);
  template Array<PixelYA8, 2> helperConvertToPacked<PixelYA8>(const PlanarImage2D<
    PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>,
    PixelPlane<uint8_t, 2, ImageChannel::Alpha, 1, 1>>& image);
  template Array<PixelY16, 2> helperConvertToPacked<PixelY16>(const PlanarImage2D<PixelPlane<uint16_t, 2, ImageChannel::Luminance, 1, 1>>& image);
  template Array<PixelY32F, 2> helperConvertToPacked<PixelY32F>(const PlanarImage2D<PixelPlane<float, 2, ImageChannel::Luminance, 1, 1>>& image);

  namespace  {

    // Register the helperConvertToPacked function for common types
    [[maybe_unused]] bool g_reg16 = registerConversion(helperConvertToPacked<PixelRGB8, RGBPlanarImage<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg17 = registerConversion(helperConvertToPacked<PixelRGBA8, RGBAPlanarImage<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg18 = registerConversion(helperConvertToPacked<PixelRGB16, RGBPlanarImage<uint16_t>>, 1.00f);
    [[maybe_unused]] bool g_reg19 = registerConversion(helperConvertToPacked<PixelRGBA16, RGBAPlanarImage<uint16_t>>, 1.00f);
    [[maybe_unused]] bool g_reg20 = registerConversion(helperConvertToPacked<PixelRGB32F, RGBPlanarImage<float>>, 1.00f);
    [[maybe_unused]] bool g_reg21 = registerConversion(helperConvertToPacked<PixelRGBA32F, RGBAPlanarImage<float>>, 1.00f);
    [[maybe_unused]] bool g_reg22 = registerConversion(helperConvertToPacked<PixelBGR8, RGBPlanarImage<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg23 = registerConversion(helperConvertToPacked<PixelBGRA8, RGBAPlanarImage<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg24 = registerConversion(helperConvertToPacked<PixelYUV8, YUV444Image<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg24a = registerConversion(helperConvertToPacked<PixelYUV8, YUV422Image<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg24b = registerConversion(helperConvertToPacked<PixelYUV8, YUV420Image<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg25 = registerConversion(helperConvertToPacked<PixelYUVA8, YUV444Image<uint8_t>>, 1.00f);
    [[maybe_unused]] bool g_reg26 = registerConversion(helperConvertToPacked<PixelYUV32F, YUV444Image<float>>, 1.00f);
    [[maybe_unused]] bool g_reg27 = registerConversion(helperConvertToPacked<PixelY8, PlanarImage2D<PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>>>, 1.00f);
    [[maybe_unused]] bool g_reg28 = registerConversion(helperConvertToPacked<PixelYA8, PlanarImage2D<
      PixelPlane<uint8_t, 2, ImageChannel::Luminance, 1, 1>,
      PixelPlane<uint8_t, 2, ImageChannel::Alpha, 1, 1>>>, 1.00f);
    [[maybe_unused]] bool g_reg29 = registerConversion(helperConvertToPacked<PixelY16, PlanarImage2D<PixelPlane<uint16_t, 2, ImageChannel::Luminance, 1, 1>>>, 1.00f);
    [[maybe_unused]] bool g_reg30 = registerConversion(helperConvertToPacked<PixelY32F, PlanarImage2D<PixelPlane<float, 2, ImageChannel::Luminance, 1, 1>>>, 1.00f);
  }




}
