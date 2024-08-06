//
// Created by charles galambos on 06/08/2024.
//

#pragma once

#include <array>
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! Define a struct to handle color conversions

  struct ColorConversion
  {
    using WorkingT = float;

    //! Matrix to convert YUV values to RGB.
    static const Matrix<float, 3, 3> mImageYUVtoRGBMatrix;

    //! Matrix to convert YUV values to RGB.
    static const Matrix<float, 3, 3> mImageRGBtoYUVMatrixStd;

    //! Matrix to convert YUV values to RGB.
    static const Matrix<float, 3, 3> mImageRGBtoYUVMatrix;

    //! Byte conversion tables for YUV to RGB
    static const std::array<int,256> mRGBcYUV_ubLookup;
    static const std::array<int,256> mRGBcYUV_vrLookup;
    static const std::array<int,256 * 256> mRGBcYUV_uvgLookup;

    // Specialization for Luminance conversion
    template<typename CompT, typename PixelType>
     requires std::is_same_v<typename PixelType::value_type, uint8_t> && std::is_same_v<CompT, uint8_t> && (PixelType::template hasChannels<ImageChannel::Luminance,ImageChannel::ChrominanceV>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Red>)
    {
      int iy = pixel.template get<ImageChannel::Luminance,uint8_t>();
      int v = pixel.template get<ImageChannel::ChrominanceV,uint8_t>();
      int tmp = iy + mRGBcYUV_vrLookup[std::size_t(v)];
      return get<ImageChannel::Red,CompT>(uint8_t(std::clamp(tmp,0,255)));
    }

    template<typename CompT, typename PixelType>
    requires std::is_same_v<typename PixelType::value_type, uint8_t>  && std::is_same_v<CompT, uint8_t> && (PixelType::template hasChannels<ImageChannel::Luminance,ImageChannel::ChrominanceV,ImageChannel::ChrominanceU>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Green>)
    {
      int iy = pixel.template get<ImageChannel::Luminance,uint8_t>();
      int v = pixel.template get<ImageChannel::ChrominanceV,uint8_t>();
      int u = pixel.template get<ImageChannel::ChrominanceU,uint8_t>();
      int tmp = iy + mRGBcYUV_uvgLookup[std::size_t(u + 256 * v)];
      return get<ImageChannel::Green,CompT>(uint8_t(std::clamp(tmp,0,255)));
    }

    template<typename CompT, typename PixelType>
    requires std::is_same_v<typename PixelType::value_type, uint8_t> && std::is_same_v<CompT, uint8_t> && (PixelType::template hasChannels<ImageChannel::Luminance,ImageChannel::ChrominanceU>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Blue>)
    {
      int iy = pixel.template get<ImageChannel::Luminance,uint8_t>();
      int u = pixel.template get<ImageChannel::ChrominanceU,uint8_t>();
      auto tmp = iy + mRGBcYUV_ubLookup[std::size_t(u)];
      return get<ImageChannel::Blue,CompT>(uint8_t(std::clamp(tmp,0,255)));
    }

    // Specialization for Luminance conversion
    template<typename CompT, typename PixelType>
      requires (PixelType::template hasChannels<ImageChannel::Red,ImageChannel::Green,ImageChannel::Blue>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Luminance>)
    {
      return get<ImageChannel::Luminance,CompT>(0.299f * pixel.template get<ImageChannel::Red,float>() + 0.587f * WorkingT(pixel.template get<ImageChannel::Green,float>()) + 0.114f * pixel.template get<ImageChannel::Blue,float>());
    }

    // Specialization for Intensity conversion
    template<typename CompT, typename PixelType>
      requires (PixelType::template hasChannels<ImageChannel::Red,ImageChannel::Green,ImageChannel::Blue>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Intensity>)
    {
      return get<ImageChannel::Intensity,CompT>(0.2126f * pixel.template get<ImageChannel::Red,float>() + 0.7152f * pixel.template get<ImageChannel::Green,float>() + 0.0722f * pixel.template get<ImageChannel::Blue,float>());
    }

    // Specialization for ChrominanceU conversion
    template<typename CompT, typename PixelType>
      requires (PixelType::template hasChannels<ImageChannel::Red,ImageChannel::Green,ImageChannel::Blue>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::ChrominanceU>)
    {
      return get<ImageChannel::ChrominanceU,CompT>(-0.14713f * pixel.template get<ImageChannel::Red,float>() - 0.28886f * pixel.template get<ImageChannel::Green,float>() + 0.436f * pixel.template get<ImageChannel::Blue,float>());
    }

    // Specialization for ChrominanceV conversion
    template<typename CompT, typename PixelType>
      requires (PixelType::template hasChannels<ImageChannel::Red,ImageChannel::Green,ImageChannel::Blue>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::ChrominanceV>)
    {
      return get<ImageChannel::ChrominanceV,CompT>(0.615f * pixel.template get<ImageChannel::Red,float>() - 0.51499f * pixel.template get<ImageChannel::Green,float>() - 0.10001f * pixel.template get<ImageChannel::Blue,float>());
    }

    // Specialization for recovering Red from YUV
    template<typename CompT, typename PixelType>
      requires (PixelType::template hasChannels<ImageChannel::Luminance,ImageChannel::ChrominanceV>()) && (!std::is_same_v<typename PixelType::value_type, uint8_t>)
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Red>)
    {
      return get<ImageChannel::Red,CompT>(1.0f * pixel.template get<ImageChannel::Luminance,float>() + 1.13983f * pixel.template get<ImageChannel::ChrominanceV,float>());
    }

    // Specialization for recovering Green from YUV
    template<typename CompT, typename PixelType>
      requires (PixelType::template hasChannels<ImageChannel::Luminance,ImageChannel::ChrominanceU,ImageChannel::ChrominanceV>()) && (!std::is_same_v<typename PixelType::value_type, uint8_t>)
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Green>)
    {
      return get<ImageChannel::Green,CompT>(1.0f * pixel.template get<ImageChannel::Luminance,float>() - 0.39465f * pixel.template get<ImageChannel::ChrominanceU,float>() - 0.58060f * pixel.template get<ImageChannel::ChrominanceV,float>());
    }

    // Specialization for recovering Blue from YUV
    template<typename CompT, typename PixelType>
      requires (PixelType::template hasChannels<ImageChannel::Luminance,ImageChannel::ChrominanceU>()) && (!std::is_same_v<typename PixelType::value_type, uint8_t>)
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Blue>)
    {
      return get<ImageChannel::Blue,CompT>(1.0f * pixel.template get<ImageChannel::Luminance,float>() + 2.03211f * pixel.template get<ImageChannel::ChrominanceU,float>());
    }

    // Just change datatype as needed
    template<typename CompT, typename PixelType, ImageChannel channel>
      requires (PixelType::template hasChannel<channel>())
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, channel>)
    {
      return pixel.template get<channel,CompT>();
    }

  };

  //! Get the value of channel from the pixel, compute if possible
  template<ImageChannel target, typename CompT, typename SourceCompT, ImageChannel... Channels>
  [[nodiscard]] constexpr CompT get(const Pixel<SourceCompT, Channels...> &pixel)
  {
    return ColorConversion::convert<CompT>(pixel, std::integral_constant<ImageChannel, target>{});
  }


  //! Assign converted pixel value to another.
  template<typename TargetCompT, ImageChannel... TargetChannels,
            typename SourceCompT, ImageChannel... SourceChannels>
  constexpr void assign(Pixel<TargetCompT, TargetChannels...> &target,
                        const Pixel<SourceCompT, SourceChannels...> &source)
  {
    // Go through channel by channel converting as needed
    (target.template set<TargetChannels>(get<TargetChannels,TargetCompT>(source)), ...);
  }


}