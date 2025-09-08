//
// Created on 08/09/2025.
//

#include "Ravl2/Pixel/PixelPlane.hh"

namespace Ravl2
{

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

}// namespace Ravl2
