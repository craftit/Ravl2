//
// Created by charles galambos on 06/08/2024.
//

#pragma once

#include <limits>
#include <numeric>
#include <array>
#include <cstdint>
#include <spdlog/spdlog.h>
#include "Ravl2/Types.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! Image channel types.
  //! It would be possible to turn this into set of classes with static constexpr members specifying the behaviour for
  //! a channel.  So far this has not been necessary, and would make it even more complicated to use.
  //!
  //! Following is a table showing the pixel type, and channels that can be used with it, the range of values for each channel
  //! and the offset for the channel.  The offset is used to convert the channel value to the correct range.
  //!
  //! | Pixel Type | Channels                     | Range      | Offset | Default | Rescaled |
  //! |------------|------------------------------|------------|--------|---------|----------|
  //! | uint8_t    | Red,Green,Blue               | [0, 255]   | 0      | NA      | Yes      |
  //! | uint8_t    | Alpha                        | [0, 255]   | 0      | 255     | Yes      |
  //! | uint8_t    | Luminance                    | [0, 255]   | 0      | NA      | Yes      |
  //! | uint8_t    | ChrominanceU, ChrominanceV   | [0, 255]   | 128    | 128     | Yes      |
  //! | uint8_t    | Hue, Saturation, Value       | [0, 255]   | 0      | NA      | Yes      |
  //! | uint8_t    | Lightness                    | [0, 255]   | 0      | NA      | Yes      |
  //! | uint8_t    | Count, Label, Depth, Unused  | [0, 255]   | 0      | NA      | No       |
  //! | uint8_t    | Signal                       | [0, 255]   | 0      | NA      | Yes      |
  //! | uint16_t   | Red,Green,Blue               | [0, 65535] | 0      | NA      | Yes      |
  //! | uint16_t   | Red,Green,Blue               | [0, 65535] | 0      | NA      | Yes      |
  //! | uint16_t   | Alpha                        | [0, 65535] | 0      | 65535   | Yes      |
  //! | uint16_t   | Luminance                    | [0, 65535] | 0      | NA      | Yes      |
  //! | uint16_t   | ChrominanceU, ChrominanceV   | [0, 65535] | 32768  | 32768   | Yes      |
  //! | uint16_t   | Hue, Saturation, Value       | [0, 65535] | 0      | NA      | Yes      |
  //! | uint16_t   | Lightness                    | [0, 65535] | 0      | NA      | Yes      |
  //! | uint16_t   | Count, Label, Depth, Unused  | [0, 65535] | 0      | NA      | No       |
  //! | uint16_t   | Signal                       | [0, 65535] | 0      | NA      | Yes      |
  //! | float      | Red,Green,Blue               | [0, 1]     | 0      | NA      | Yes      |
  //! | float      | Alpha                        | [0, 1]     | 0      | 1       | Yes      |
  //! | float      | Luminance                    | [0, 1]     | 0      | NA      | Yes      |
  //! | float      | ChrominanceU, ChrominanceV   | [-0.5, 0.5]| 0      | 0       | Yes      |
  //! | float      | Hue, Saturation, Value       | [0, 1]     | 0      | NA      | Yes      |
  //! | float      | Lightness                    | [0, 1]     | 0      | NA      | Yes      |
  //! | float      | Count, Label, Depth, Unused  |  Undef     | 0      | NA      | No       |
  //! | float      | Signal                       | [0, 1]     | 0      | NA      | Yes      |
  //! |------------|------------------------------|------------|--------|---------|----------|
  //!
  //! The default value is the value used when the channel is not present in the pixel initialization.

  enum class ImageChannel
  {
    Intensity,
    Red,
    Green,
    Blue,
    Alpha,
    Luminance,
    ChrominanceU,//!< For uint8_t images the value is +128
    ChrominanceV,//!< For uint8_t images the value is +128
    Luminance2,  //!< In YUV 4:2:2, this is the second luma value
    Hue,
    Saturation,
    Value,
    Lightness,
    Count,
    Label,
    Depth,
    Signal,
    Unused
  };

  //! Get name for a channel.
  std::string_view toString(ImageChannel channel);

  std::ostream &operator<<(std::ostream &strm, const ImageChannel &channel);
  std::istream &operator>>(std::istream &strm, ImageChannel &channel);

  //! Setup some traits for the ImageChannel enum types
  template <ImageChannel channel>
  struct PixelChannelTraits {
    static constexpr bool isRGB = (channel == ImageChannel::Red || channel == ImageChannel::Green || channel == ImageChannel::Blue || channel == ImageChannel::Alpha);
    static constexpr bool isYUV = (channel == ImageChannel::Luminance || channel == ImageChannel::ChrominanceU || channel == ImageChannel::ChrominanceV);
    static constexpr bool isHSV = (channel == ImageChannel::Hue || channel == ImageChannel::Saturation || channel == ImageChannel::Value);
    static constexpr bool isHSL = (channel == ImageChannel::Hue || channel == ImageChannel::Saturation || channel == ImageChannel::Lightness);

    //! Check if channel is an absolute value or scaled from 0 to 1. For example in float images the values are usually in the range [0, 1].
    //! When converting the colour values from a byte to a float image the values are scaled to the range [0, 1].
    static constexpr bool isNormalized = !(channel == ImageChannel::Count || channel == ImageChannel::Label || channel == ImageChannel::Depth || channel == ImageChannel::Unused);
  };

  //! Some traits for channels with a pixel type
  template <typename DataT, ImageChannel channel>
  struct PixelTypeTraits {
    using value_type = DataT;
    static constexpr bool isNormalized = PixelChannelTraits<channel>::isNormalized;
    static constexpr DataT min = (std::is_integral_v<DataT> ? 0 : DataT(0));
    static constexpr DataT max = (std::is_integral_v<DataT> ? std::numeric_limits<DataT>::max() : DataT(1));
    static constexpr DataT offset = 0;
    static constexpr DataT defaultValue = 0;
  };

  //! Deal with floating point chroma types
  template <typename DataT, ImageChannel channel>
    requires(std::is_floating_point_v<DataT> && (channel == ImageChannel::ChrominanceU || channel == ImageChannel::ChrominanceV))
  struct PixelTypeTraits<DataT, channel> {
    using value_type = DataT;
    static constexpr bool isNormalized = PixelChannelTraits<channel>::isNormalized;
    static constexpr DataT min = DataT(-0.5);
    static constexpr DataT max = DataT(0.5);
    static constexpr DataT offset = 0;
    static constexpr DataT defaultValue = 0;
  };

  //! Some traits for channels with a pixel type
  template <typename DataT, ImageChannel channel>
    requires std::is_signed_v<DataT> && std::is_integral_v<DataT>
  struct PixelTypeTraits<DataT, channel> {
    using value_type = DataT;
    static constexpr bool isNormalized = PixelChannelTraits<channel>::isNormalized;
    static constexpr DataT min = 0;
    static constexpr DataT max = std::numeric_limits<DataT>::max();
    static constexpr DataT offset = 0;
    static constexpr DataT defaultValue = 0;
  };

  //! Deal with unsigned integer chroma types
  template <typename DataT, ImageChannel channel>
    requires(std::is_integral_v<DataT> && std::is_unsigned_v<DataT> && (channel == ImageChannel::ChrominanceU || channel == ImageChannel::ChrominanceV))
  struct PixelTypeTraits<DataT, channel> {
    using value_type = DataT;
    static constexpr bool isNormalized = PixelChannelTraits<channel>::isNormalized;
    static constexpr DataT min = std::numeric_limits<DataT>::min();
    static constexpr DataT max = std::numeric_limits<DataT>::max();
    static constexpr DataT offset = std::numeric_limits<DataT>::max() / 2;
    static constexpr DataT defaultValue = offset;
  };

  //! Convert a pixel value to a different type follow the channel conversion rules.
  template <ImageChannel channel, typename CompT, typename PixelValueTypeT>
  constexpr CompT get(PixelValueTypeT pixel)
  {
    // If the channel is normalized then we need to scale the value to the correct range.
    if constexpr(PixelTypeTraits<CompT, channel>::isNormalized) {
      //SPDLOG_INFO("Normalized channel {} -> {}  Type:{} <- {} ",toString(channel), (PixelTypeTraits<CompT, channel>::max - PixelTypeTraits<CompT, channel>::min) / (PixelTypeTraits<PixelValueTypeT, channel>::max - PixelTypeTraits<PixelValueTypeT, channel>::min), typeid(CompT).name(), typeid(PixelValueTypeT).name());
      // If the channel is normalized then we need to scale the value to the correct range.
      auto raw = (pixel - PixelTypeTraits<PixelValueTypeT, channel>::offset) * (PixelTypeTraits<CompT, channel>::max - PixelTypeTraits<CompT, channel>::min) / (PixelTypeTraits<PixelValueTypeT, channel>::max - PixelTypeTraits<PixelValueTypeT, channel>::min) + PixelTypeTraits<CompT, channel>::offset;
      // Check if we're converting to a floating point type.
      if constexpr(std::is_floating_point_v<CompT>) {
        return raw;
      }
      if constexpr(std::is_floating_point_v<PixelValueTypeT> && std::is_integral_v<CompT>) {
        // If we're converting to an integer type then we need to clamp the value.
        return intRound< decltype(raw),CompT>(std::clamp( raw, PixelValueTypeT(PixelTypeTraits<CompT, channel>::min), PixelValueTypeT(PixelTypeTraits<CompT, channel>::max)));
      }
      // If we're converting to an integer type then we need to clamp the value.
      return CompT(std::clamp(raw, decltype(raw)(PixelTypeTraits<CompT, channel>::min), decltype(raw)(PixelTypeTraits<CompT, channel>::max)));
    }
    // If the channel is not normalized then we can just return the value.
    return CompT(pixel);
  }

  //! Pixel type, it is formed by a data type and set of channels.
  //! By convention when a channel is floating point the values
  //! are in the range [0, 1], otherwise the values use the full
  //! range of the data type.

  template <typename CompT, ImageChannel... Channels>
  class Pixel : public Vector<CompT, IndexSizeT(sizeof...(Channels))>
  {
  public:
    using value_type = CompT;

    //! Default constructor.
    // Creates an undefined value.
    Pixel() = default;

    //! Unpack from parameter pack.
    template <typename... Args>
    explicit Pixel(Args... args)
        : Vector<CompT, IndexSizeT (sizeof...(Channels))>({CompT(args)...})
    {}

    //! Construct from an initializer list
    Pixel(std::initializer_list<CompT> list)
    {
      if(list.size() != sizeof...(Channels)) {
	SPDLOG_ERROR("Pixel initializer list size mismatch: {} != {}", list.size(), sizeof...(Channels));
	throw std::runtime_error("Pixel initializer list size mismatch");
      }
      std::copy(list.begin(), list.end(), this->begin());
    }

    //! Construct from another pixel, mapping the channels.
    template <typename OCompT, ImageChannel... OChannels>
    explicit Pixel(const Pixel<OCompT, OChannels...> &other)
    {
      assign(*this,other);
    }

  private:
    template <ImageChannel channel, std::size_t... Is>
    static constexpr std::size_t getIndexImpl(std::index_sequence<Is...>)
    {
      return ((Channels == channel ? Is : 0) + ...);
    }

  public:
    //! Get the index of a channel.
    template <ImageChannel channel>
    [[nodiscard]] static constexpr std::size_t channelIndex()
    {
      return getIndexImpl<channel>(std::make_index_sequence<sizeof...(Channels)> {});
    }

    //! Check if a channel is present.
    template <ImageChannel channel>
    [[nodiscard]] static constexpr bool hasChannel()
    {
      return ((Channels == channel) || ...);
    }

    //! Check if a list of channels are present.
    template <ImageChannel... OChannels>
    [[nodiscard]] static constexpr bool hasChannels()
    {
      return ((hasChannel<OChannels>()) && ...);
    }

    //! Set a single channel.
    template <ImageChannel channel>
    constexpr void set(const CompT &value)
    {
      static_assert(hasChannel<channel>(), "Channel not present");
      (*this)[channelIndex<channel>()] = value;
    }

    //! Get a single channel.
    template <ImageChannel channel, typename TargetTypeT = CompT>
    [[nodiscard]] constexpr TargetTypeT get() const
    {
      if constexpr(hasChannel<channel>()) {
        return Ravl2::get<channel, TargetTypeT>((*this)[channelIndex<channel>()]);
      } else if constexpr(channel == ImageChannel::Alpha) {
        // If we don't the alpha channel then return the default value.
        return PixelTypeTraits<TargetTypeT, channel>::max;
      } else if constexpr(channel == ImageChannel::ChrominanceU || channel == ImageChannel::ChrominanceV) {
        // Chroma channels have a default value of grey
        return PixelTypeTraits<TargetTypeT, channel>::defaultValue;
      } else {
        // Report an error if we don't have the channel.
        static_assert(hasChannel<channel>(), "Channel not present");
      }
      return TargetTypeT();
    }

    //! Assign from another pixel, mapping the channels.
    //! This will leave pixels with no matching channels unchanged.
    template <typename OCompT, ImageChannel... OChannels>
    constexpr void assign(const Pixel<OCompT, OChannels...> &other)
    {
      (set<OChannels>(other.template get<OChannels>()), ...);
    }

    //! Serialization support
    //! Just do the same thing as the vector.
//    template <typename ArchiveT>
//    void serialize(ArchiveT &archive)
//    {
//      archive(static_cast<Vector<CompT, sizeof...(Channels)> &>(*this));
//    }
  };

  //! Stream output
  template <class CompT, ImageChannel... Channels>
  inline std::ostream &operator<<(std::ostream &strm, const Pixel<CompT, Channels...> &val)
  {
    for(std::size_t i = 0; i < sizeof...(Channels); ++i) {
      if(i != 0) {
        strm << ' ';
      }
      // If we're working with int8_t or uint8_t make sure they are treated as numeric.
      if constexpr(std::is_same_v<CompT, int8_t> || std::is_same_v<CompT, uint8_t>) {
        strm << int(val[i]);
      } else {
        strm << val[i];
      }
    }
    return strm;
  }

  //! Stream input
  template <class CompT, ImageChannel... Channels>
  inline std::istream &operator>>(std::istream &strm, Pixel<CompT, Channels...> &val)
  {
    for(std::size_t i = 0; i < sizeof...(Channels); ++i) {
      CompT tmp;
      strm >> tmp;
      val[i] = tmp;
    }
    return strm;
  }

  //! Define some common formats to save typing
  using PixelY8 = Pixel<uint8_t, ImageChannel::Luminance>;
  using PixelY16 = Pixel<uint16_t, ImageChannel::Luminance>;
  using PixelY32F = Pixel<uint16_t, ImageChannel::Luminance>;
  using PixelZ16 = Pixel<uint16_t, ImageChannel::Depth>;
  using PixelZ32F = Pixel<float, ImageChannel::Depth>;
  using PixelRGB8 = Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
  using PixelRGBA8 = Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
  using PixelRGB16 = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
  using PixelRGBA16 = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
  using PixelRGB32F = Pixel<float, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
  using PixelRGBA32F = Pixel<float, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
  using PixelBGR8 = Pixel<uint8_t, ImageChannel::Blue, ImageChannel::Green, ImageChannel::Red>;
  using PixelBGRA8 = Pixel<uint8_t, ImageChannel::Blue, ImageChannel::Green, ImageChannel::Red, ImageChannel::Alpha>;
  using PixelYUV8 = Pixel<uint8_t, ImageChannel::Luminance, ImageChannel::ChrominanceU, ImageChannel::ChrominanceV>;
  using PixelYUV32F = Pixel<float, ImageChannel::Luminance, ImageChannel::ChrominanceU, ImageChannel::ChrominanceV>;

  // Let the compiler know there's instantiations of the template
  extern template class Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
  extern template class Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
  extern template class Pixel<float, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
  extern template class Pixel<float, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
  extern template class Pixel<uint8_t, ImageChannel::Blue, ImageChannel::Green, ImageChannel::Red>;
  extern template class Pixel<uint8_t, ImageChannel::Blue, ImageChannel::Green, ImageChannel::Red, ImageChannel::Alpha>;
  extern template class Pixel<uint8_t, ImageChannel::Luminance, ImageChannel::ChrominanceU, ImageChannel::ChrominanceV>;
  extern template class Pixel<float, ImageChannel::Luminance, ImageChannel::ChrominanceU, ImageChannel::ChrominanceV>;

  // Also about arrays based on the pixel types
  extern template class Array<PixelY8,2>;
  extern template class Array<PixelY16,2>;
  extern template class Array<PixelZ16, 2>;
  extern template class Array<PixelZ32F, 2>;
  extern template class Array<PixelRGB8,2>;
  extern template class Array<PixelRGBA8,2>;
  extern template class Array<PixelRGB16,2>;
  extern template class Array<PixelRGBA16,2>;
  extern template class Array<PixelRGB32F,2>;
  extern template class Array<PixelRGBA32F,2>;
  extern template class Array<PixelBGR8,2>;
  extern template class Array<PixelBGRA8,2>;
  extern template class Array<PixelYUV8,2>;
  extern template class Array<PixelYUV32F,2>;



}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename CompT, Ravl2::ImageChannel... Channels>
struct fmt::formatter<Ravl2::Pixel<CompT, Channels...>> : fmt::ostream_formatter {
};
template <>
struct fmt::formatter<Ravl2::ImageChannel> : fmt::ostream_formatter {
};
#endif
