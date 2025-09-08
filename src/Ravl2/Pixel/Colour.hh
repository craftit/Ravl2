//
// Created by charles galambos on 06/08/2024.
//

#pragma once

#include <array>
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/ArrayIterZip.hh"

namespace Ravl2
{
  // Call to ensure that the conversion functions are registered
  extern void initColourConversion();

  //! Define a struct to handle color conversions
  //! This is here to work around the fact that we can't partially specialize functions in a namespace

  namespace ColorConversion
  {
    using WorkingT = float;

    //! Matrix to convert YUV values to RGB.
    extern const Matrix<float, 3, 3> mImageYUVtoRGBMatrix;

    //! Matrix to convert YUV values to RGB.
    extern const Matrix<float, 3, 3> mImageRGBtoYUVMatrixStd;

    //! Matrix to convert YUV values to RGB.
    extern const Matrix<float, 3, 3> mImageRGBtoYUVMatrix;

    //! Byte conversion tables for YUV to RGB
    extern const std::array<int, 256> mRGBcYUV_ubLookup;
    extern const std::array<int, 256> mRGBcYUV_vrLookup;
    extern const std::array<int, 256 * 256> mRGBcYUV_uvgLookup;

    // Specialization for Luminance conversion
    template <typename CompT, typename PixelType>
      requires std::is_same_v<typename PixelType::value_type, uint8_t> && std::is_same_v<CompT, uint8_t> && (PixelType::template hasChannels<ImageChannel::Luminance>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Red>)
    {
      int iy = pixel.template get<ImageChannel::Luminance, uint8_t>();
      int v = pixel.template get<ImageChannel::ChrominanceV, uint8_t>();
      // Since the chroma V value is offset by 128, we need to adjust it before looking up
      int tmp = iy + mRGBcYUV_vrLookup[std::size_t(v)];
      return get<ImageChannel::Red, CompT>(uint8_t(std::clamp(tmp, 0, 255)));
    }

    template <typename CompT, typename PixelType>
      requires std::is_same_v<typename PixelType::value_type, uint8_t> && std::is_same_v<CompT, uint8_t> && (PixelType::template hasChannels<ImageChannel::Luminance>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Green>)
    {
      int iy = pixel.template get<ImageChannel::Luminance, uint8_t>();
      int v = pixel.template get<ImageChannel::ChrominanceV, uint8_t>();
      int u = pixel.template get<ImageChannel::ChrominanceU, uint8_t>();
      // The UV values are offset by 128, which is accounted for in the lookup table
      int tmp = iy + mRGBcYUV_uvgLookup[std::size_t(u + 256 * v)];
      return get<ImageChannel::Green, CompT>(uint8_t(std::clamp(tmp, 0, 255)));
    }

    template <typename CompT, typename PixelType>
      requires std::is_same_v<typename PixelType::value_type, uint8_t> && std::is_same_v<CompT, uint8_t> && (PixelType::template hasChannels<ImageChannel::Luminance>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Blue>)
    {
      int iy = pixel.template get<ImageChannel::Luminance, uint8_t>();
      int u = pixel.template get<ImageChannel::ChrominanceU, uint8_t>();
      // Since the chroma U value is offset by 128, we need to adjust it before looking up
      auto tmp = iy + mRGBcYUV_ubLookup[std::size_t(u)];
      return get<ImageChannel::Blue, CompT>(uint8_t(std::clamp(tmp, 0, 255)));
    }

    // Specialization for Luminance conversion
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannels<ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Luminance>)
    {
      return get<ImageChannel::Luminance, CompT>(0.299f * pixel.template get<ImageChannel::Red, float>() + 0.587f * WorkingT(pixel.template get<ImageChannel::Green, float>()) + 0.114f * pixel.template get<ImageChannel::Blue, float>());
    }

    // Specialization for Intensity conversion
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannels<ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Intensity>)
    {
      return get<ImageChannel::Intensity, CompT>(0.2126f * pixel.template get<ImageChannel::Red, float>() + 0.7152f * pixel.template get<ImageChannel::Green, float>() + 0.0722f * pixel.template get<ImageChannel::Blue, float>());
    }

    // Specialization for ChrominanceU conversion
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannels<ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::ChrominanceU>)
    {
      return get<ImageChannel::ChrominanceU, CompT>(-0.14713f * pixel.template get<ImageChannel::Red, float>() - 0.28886f * pixel.template get<ImageChannel::Green, float>() + 0.436f * pixel.template get<ImageChannel::Blue, float>());
    }

    // Specialization for ChrominanceV conversion
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannels<ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::ChrominanceV>)
    {
      return get<ImageChannel::ChrominanceV, CompT>(0.615f * pixel.template get<ImageChannel::Red, float>() - 0.51499f * pixel.template get<ImageChannel::Green, float>() - 0.10001f * pixel.template get<ImageChannel::Blue, float>());
    }

    // Specialization for recovering Red from YUV
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannels<ImageChannel::Luminance>()) && (!std::is_same_v<typename PixelType::value_type, uint8_t>)
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Red>)
    {
      return get<ImageChannel::Red, CompT>(1.0f * pixel.template get<ImageChannel::Luminance, float>() + 1.13983f * pixel.template get<ImageChannel::ChrominanceV, float>());
    }

    // Specialization for recovering Green from YUV
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannels<ImageChannel::Luminance>()) && (!std::is_same_v<typename PixelType::value_type, uint8_t>)
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Green>)
    {
      return get<ImageChannel::Green, CompT>(1.0f * pixel.template get<ImageChannel::Luminance, float>() - 0.39465f * pixel.template get<ImageChannel::ChrominanceU, float>() - 0.58060f * pixel.template get<ImageChannel::ChrominanceV, float>());
    }

    // Specialization for recovering Blue from YUV
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannels<ImageChannel::Luminance>()) && (!std::is_same_v<typename PixelType::value_type, uint8_t>)
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Blue>)
    {
      return get<ImageChannel::Blue, CompT>(1.0f * pixel.template get<ImageChannel::Luminance, float>() + 2.03211f * pixel.template get<ImageChannel::ChrominanceU, float>());
    }

    // If we have Luminance but no ChrominanceU, return default value
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannel<ImageChannel::Luminance>()) && (!PixelType::template hasChannel<ImageChannel::ChrominanceU>())
    constexpr CompT convert(const PixelType &, std::integral_constant<ImageChannel, ImageChannel::ChrominanceU>)
    {
      return PixelTypeTraits<CompT, ImageChannel::ChrominanceU>::defaultValue;
    }

    // If we have Luminance but no ChrominanceV, return default value
    template <typename CompT, typename PixelType>
      requires(PixelType::template hasChannel<ImageChannel::Luminance>()) && (!PixelType::template hasChannel<ImageChannel::ChrominanceV>())
    constexpr CompT convert(const PixelType &, std::integral_constant<ImageChannel, ImageChannel::ChrominanceV>)
    {
      return PixelTypeTraits<CompT, ImageChannel::ChrominanceV>::defaultValue;
    }

    // Just change datatype as needed
    template <typename CompT, typename PixelType, ImageChannel channel>
      requires(PixelType::template hasChannel<channel>())
    constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, channel>)
    {
      return pixel.template get<channel, CompT>();
    }
  };

  //! Get the value of channel from the pixel, compute if possible
  template <ImageChannel target, typename CompT, typename SourceCompT, ImageChannel... Channels>
  [[nodiscard]] constexpr CompT get(const Pixel<SourceCompT, Channels...> &pixel)
  {
    return ColorConversion::convert<CompT>(pixel, std::integral_constant<ImageChannel, target> {});
  }

  //! Assign converted pixel value to another.
  template <typename TargetCompT, ImageChannel... TargetChannels,
            typename SourceCompT, ImageChannel... SourceChannels>
  constexpr void assign(Pixel<TargetCompT, TargetChannels...> &target,
                        const Pixel<SourceCompT, SourceChannels...> &source)
  {
    // Go through channel by channel converting as needed
    (target.template set<TargetChannels>(get<TargetChannels, TargetCompT>(source)), ...);
  }

  //! Assign a pixel value to a scalar, extract Luminance by default
  template<typename TargetTypeT, typename SourceCompT, ImageChannel... SourceChannels>
   requires std::is_integral_v<TargetTypeT> || std::is_floating_point_v<TargetTypeT>
  inline constexpr void assign(TargetTypeT &target,const Pixel<SourceCompT, SourceChannels...> &source)
  {
    target = ColorConversion::convert<TargetTypeT>(source, std::integral_constant<ImageChannel, ImageChannel::Luminance> {});
  }

  //! Copy data from a source array to destination array
  template <typename Array1T, typename Array2T, typename Data1T = typename Array1T::value_type, unsigned N = Array1T::dimensions>
    requires(WindowedArray<Array1T, typename Array1T::value_type, N> && WindowedArray<Array2T, typename Array2T::value_type, N>) && (N >= 2)
  constexpr void convert(Array1T &dest, const Array2T &src)
  {
    // Check if we can reallocate 'dest', that if it is an Array, and the size is different
    if constexpr(std::is_same_v<Array1T, Array<Data1T, N>>) {
      if(!dest.range().contains(src.range())) {
        dest = Array1T(src.range());
      }
    }
    auto iter = zip(dest, src);
    while(iter.valid()) {
      do {
        assign(iter.template data<0>(), iter.template data<1>());
      } while(iter.next());
    }
  }

  // Convert a return a new array
  template <typename DestT, typename SourceT>
  DestT convert(const SourceT &src)
  {
    DestT dest;
    convert(dest, src);
    return dest;
  }

  // Declare some common conversions
  extern template void convert(Array<PixelY8, 2> &dest, const Array<PixelYUV8, 2> &src);
  extern template void convert(Array<PixelY8, 2> &dest, const Array<PixelRGB8, 2> &src);
  extern template void convert(Array<PixelYUV8, 2> &dest, const Array<PixelY8, 2> &src);
  extern template void convert(Array<PixelYUV8, 2> &dest, const Array<PixelRGB8, 2> &src);
  extern template void convert(Array<PixelRGB8, 2> &dest, const Array<PixelYUV8, 2> &src);
  extern template void convert(Array<PixelRGB8, 2> &dest, const Array<PixelY8, 2> &src);

  extern template void convert(Array<PixelRGB8, 2> &dest, const Array<PixelBGR8, 2> &src);
  extern template void convert(Array<PixelBGR8, 2> &dest, const Array<PixelRGB8, 2> &src);
  extern template void convert(Array<PixelBGR8, 2> &dest, const Array<PixelRGBA8, 2> &src);
  extern template void convert(Array<PixelBGRA8, 2> &dest, const Array<PixelRGBA8, 2> &src);
  extern template void convert(Array<PixelRGBA8, 2> &dest, const Array<PixelBGRA8, 2> &src);

  extern template void convert(Array<PixelZ32F, 2> &dest, const Array<PixelZ16, 2> &src);

}// namespace Ravl2