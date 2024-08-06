//
// Created by charles galambos on 06/08/2024.
//

#pragma once

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

    static const std::array<int,256> mRGBcYUV_ubLookup;
    static const std::array<int,256> mRGBcYUV_vrLookup;
    static const std::array<int,256 * 256> mRGBcYUV_uvgLookup;

    // Specialization for Luminance conversion
    template<typename CompT, typename PixelType>
     requires std::is_same_v<typename PixelType::value_type, uint8_t>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Red>)
    {
      int iy = pixel.template get<ImageChannel::Luminance>();
      int v = pixel.template get<ImageChannel::ChrominanceV>();
      int tmp = iy + mRGBcYUV_vrLookup[std::size_t(v)];
      return std::clamp(tmp,0,255);
    }

    template<typename CompT, typename PixelType>
    requires std::is_same_v<typename PixelType::value_type, uint8_t>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Green>)
    {
      int iy = pixel.template get<ImageChannel::Luminance>();
      int v = pixel.template get<ImageChannel::ChrominanceV>();
      int u = pixel.template get<ImageChannel::ChrominanceU>();
      int tmp = iy + mRGBcYUV_uvgLookup[std::size_t(u + 256 * v)];
      return std::clamp(tmp,0,255);
    }

    template<typename CompT, typename PixelType>
    requires std::is_same_v<typename PixelType::value_type, uint8_t>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Blue>)
    {
      int iy = pixel.template get<ImageChannel::Luminance>();
      int u = pixel.template get<ImageChannel::ChrominanceU>();
      auto tmp = iy + mRGBcYUV_ubLookup[std::size_t(u)];
      return std::clamp(tmp,0,255);
    }

    // Specialization for Luminance conversion
    template<typename CompT, typename PixelType>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Luminance>)
    {
      return WorkingT(0.299) * WorkingT(pixel.template get<ImageChannel::Red>()) + WorkingT(0.587) * WorkingT(pixel.template get<ImageChannel::Green>()) + WorkingT(0.114) * WorkingT(pixel.template get<ImageChannel::Blue>());
    }

    // Specialization for Intensity conversion
    template<typename CompT, typename PixelType>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Intensity>)
    {
      return 0.2126 * pixel.template get<ImageChannel::Red>() + 0.7152 * pixel.template get<ImageChannel::Green>() + 0.0722 * pixel.template get<ImageChannel::Blue>();
    }

    // Specialization for ChrominanceU conversion
    template<typename CompT, typename PixelType>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::ChrominanceU>)
    {
      return -0.14713 * pixel.template get<ImageChannel::Red>() - 0.28886 * pixel.template get<ImageChannel::Green>() + 0.436 * pixel.template get<ImageChannel::Blue>();
    }

    // Specialization for ChrominanceV conversion
    template<typename CompT, typename PixelType>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::ChrominanceV>)
    {
      return 0.615 * pixel.template get<ImageChannel::Red>() - 0.51499 * pixel.template get<ImageChannel::Green>() - 0.10001 * pixel.template get<ImageChannel::Blue>();
    }

    // Specialization for recovering Red from YUV
    template<typename CompT, typename PixelType>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Red>)
    {
      return 1.0 * pixel.template get<ImageChannel::Luminance>() + 0.0 * pixel.template get<ImageChannel::ChrominanceU>() + 1.13983 * pixel.template get<ImageChannel::ChrominanceV>();
    }

    // Specialization for recovering Green from YUV
    template<typename CompT, typename PixelType>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Green>)
    {
      return 1.0 * pixel.template get<ImageChannel::Luminance>() - 0.39465 * pixel.template get<ImageChannel::ChrominanceU>() - 0.58060 * pixel.template get<ImageChannel::ChrominanceV>();
    }

    // Specialization for recovering Blue from YUV
    template<typename CompT, typename PixelType>
    static constexpr CompT convert(const PixelType &pixel, std::integral_constant<ImageChannel, ImageChannel::Blue>)
    {
      return 1.0 * pixel.template get<ImageChannel::Luminance>() + 2.03211 * pixel.template get<ImageChannel::ChrominanceU>() + 0.0 * pixel.template get<ImageChannel::ChrominanceV>();
    }
  };

  //! Get the value of channel from the pixel, compute if possible
  template<ImageChannel target, typename CompT, typename SourceCompT, ImageChannel... Channels>
  [[nodiscard]] constexpr CompT get(const Pixel<SourceCompT, Channels...> &pixel)
  {
    if constexpr (Pixel<SourceCompT, Channels...>::template hasChannel<target>())
    {
      return CompT(pixel.template get<target>());
    }
    return ColorConversion::convert<CompT>(pixel, std::integral_constant<ImageChannel, target>{});
  }

}