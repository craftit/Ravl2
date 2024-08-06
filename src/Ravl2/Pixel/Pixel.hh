//
// Created by charles galambos on 06/08/2024.
//

#pragma once

#include <array>
#include "Ravl2/Types.hh"

namespace Ravl2
{

  //! Image channel types.
  // It would be possible to turn this into set of classes with static constexpr members specifying the behaviour for
  // a channel.  So far this has not been necessary, and would make it even more complicated to use.

  enum class ImageChannel
  {
    Intensity,
    Red,
    Green,
    Blue,
    Alpha,
    Luminance,
    ChrominanceU, //!< For uint8_t images the value is +128
    ChrominanceV, //!< For uint8_t images the value is +128
    Luminance2, //!< In YUV 4:2:2, this is the second luma value
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

  //! Setup some traits for the ImageChannel enum types
  template<ImageChannel channel>
  struct ImageChannelTraits
  {
    static constexpr bool isRGB = (channel == ImageChannel::Red || channel == ImageChannel::Green || channel == ImageChannel::Blue || channel == ImageChannel::Alpha);
    static constexpr bool isYUV = (channel == ImageChannel::Luminance || channel == ImageChannel::ChrominanceU || channel == ImageChannel::ChrominanceV);
    static constexpr bool isHSV = (channel == ImageChannel::Hue || channel == ImageChannel::Saturation || channel == ImageChannel::Value);
    static constexpr bool isHSL = (channel == ImageChannel::Hue || channel == ImageChannel::Saturation || channel == ImageChannel::Lightness);

    //! Check if channel is an absolute value or scaled from 0 to 1. For example in float images the values are usually in the range [0, 1].
    //! When converting the colour values from a byte to a float image the values are scaled to the range [0, 1].
    static constexpr bool isNormalized = !(channel == ImageChannel::Count || channel == ImageChannel::Label || channel == ImageChannel::Depth
      || channel == ImageChannel::Unused);
  };

  //! Some traits for channels with a pixel type
  template<typename DataT, ImageChannel channel>
  struct ImageTypeTraits
  {
    static constexpr bool isNormalized = ImageChannelTraits<channel>::isNormalized;
    static constexpr DataT min = (std::is_integral_v<DataT> ? std::numeric_limits<DataT>::min() : DataT(0));
    static constexpr DataT max = (std::is_integral_v<DataT> ? std::numeric_limits<DataT>::max() : DataT(1));
    static constexpr DataT offset = 0;
  };

  //! Some traits for channels with a pixel type
  template<ImageChannel channel>
  struct ImageTypeTraits<uint8_t, channel>
  {
    using DataT = uint8_t;
    static constexpr bool isNormalized = ImageChannelTraits<channel>::isNormalized;
    static constexpr DataT minValue = std::numeric_limits<DataT>::min();
    static constexpr DataT maxValue = std::numeric_limits<DataT>::max();
    static constexpr DataT offset = (channel == ImageChannel::ChrominanceU || channel == ImageChannel::ChrominanceV ? DataT(128) : DataT(0));
  };

  //! Some traits for channels with a pixel type
  template<ImageChannel channel>
  struct ImageTypeTraits<uint16_t, channel>
  {
    using DataT = uint8_t;
    static constexpr bool isNormalized = ImageChannelTraits<channel>::isNormalized;
    static constexpr DataT minValue = std::numeric_limits<DataT>::min();
    static constexpr DataT maxValue = std::numeric_limits<DataT>::max();
    static constexpr DataT offset = (channel == ImageChannel::ChrominanceU || channel == ImageChannel::ChrominanceV ? DataT(128*256) : DataT(0));
  };


  //! Pixel type, it is formed by a data type and set of channels.
  //! By convention when a channel is floating point the values
  //! are in the range [0, 1], otherwise the values use the full
  //! range of the data type.

  template<typename CompT, ImageChannel... Channels>
  class Pixel
    : public Vector<CompT, sizeof...(Channels)>
  {
  public:
    using value_type = CompT;

    //! Default constructor.
    // Creates an undefined value.
    Pixel() = default;

    //! Unpack from parameter pack.
    template<typename... Args>
    explicit Pixel(Args... args)
      : Vector<CompT, sizeof...(Channels)>({CompT(args)...})
    {}

    //! Construct from another pixel, mapping the channels.
    template<typename OCompT, ImageChannel... OChannels>
    explicit Pixel(const Pixel<OCompT, OChannels...> &other)
    {
      assign(other);
    }
  private:
    template<ImageChannel channel, std::size_t... Is>
    static constexpr std::size_t getIndexImpl(std::index_sequence<Is...>)
    {
      return ((Channels == channel ? Is : 0) + ...);
    }

  public:
    //! Get the index of a channel.
    template<ImageChannel channel>
    [[nodiscard]] static constexpr std::size_t channelIndex()
    {
      return getIndexImpl<channel>(std::make_index_sequence<sizeof...(Channels)>{});
    }

    //! Check if a channel is present.
    template<ImageChannel channel>
    [[nodiscard]] static constexpr bool hasChannel()
    {
      return ((Channels == channel) || ...);
    }

    //! Check if a list of channels are present.
    template<ImageChannel... OChannels>
    [[nodiscard]] static constexpr bool hasChannels()
    {
      return ((hasChannel<OChannels>()) && ...);
    }

    //! Set a single channel.
    template<ImageChannel channel>
    constexpr void set(const CompT &value)
    {
      static_assert(hasChannel<channel>(), "Channel not present");
      (*this)[channelIndex<channel>()] = value;
    }

    //! Get a single channel.
    template<ImageChannel channel>
    [[nodiscard]] constexpr CompT get() const
    {
      static_assert(hasChannel<channel>(), "Channel not present");
      return (*this)[channelIndex<channel>()];
    }

    //! Assign from another pixel, mapping the channels.
    //! This will leave pixels with no matching channels unchanged.
    template<typename OCompT, ImageChannel... OChannels>
    constexpr void assign(const Pixel<OCompT, OChannels...> &other)
    {
      (set<OChannels>(other.template get<OChannels>()), ...);
    }
  };

  //! Assign converted pixel value to another.
  template<typename TargetCompT, ImageChannel... TargetChannels,
           typename SourceCompT, ImageChannel... SourceChannels>
  constexpr void assign(Pixel<TargetCompT, TargetChannels...> &target,
	    const Pixel<SourceCompT, SourceChannels...> &source)
  {
    // Go through channel by channel converting as needed
    (target.template set<TargetChannels>(get<TargetChannels>(source)), ...);
  }

  //! Stream output
  template <class CompT, ImageChannel... Channels>
  inline std::ostream &operator<<(std::ostream &strm, const Pixel<CompT, Channels...> &val)
  {
    for (std::size_t i = 0; i < sizeof...(Channels); ++i)
    {
      if(i != 0) {
	strm << ' ';
      }
      // If we're working with int8_t or uint8_t make sure they are treated as numeric.
      if constexpr (std::is_same_v<CompT, int8_t> || std::is_same_v<CompT, uint8_t>)
      {
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
    for (std::size_t i = 0; i < sizeof...(Channels); ++i)
    {
      CompT tmp;
      strm >> tmp;
      val[i] = tmp;
    }
    return strm;
  }

  //! Define some common formats
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


}
