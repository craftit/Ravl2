//
// Created by charles galambos on 06/08/2024.
//

#include <catch2/catch_test_macros.hpp>

#include <spdlog/spdlog.h>
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"

TEST_CASE("Pixels")
{
  using namespace Ravl2;

  SECTION("PixelRGB")
  {
    Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue> pixel(1, 2, 3);

    STATIC_REQUIRE(pixel.hasChannel<ImageChannel::Red>());
    STATIC_REQUIRE(pixel.hasChannel<ImageChannel::Green>());
    STATIC_REQUIRE(pixel.hasChannel<ImageChannel::Blue>());
    STATIC_REQUIRE(!pixel.hasChannel<ImageChannel::Alpha>());
    STATIC_REQUIRE(!pixel.hasChannel<ImageChannel::Luminance>());
    STATIC_REQUIRE(pixel.hasChannels<ImageChannel::Red,ImageChannel::Green,ImageChannel::Blue>());
    STATIC_REQUIRE(!pixel.hasChannels<ImageChannel::Intensity,ImageChannel::ChrominanceU,ImageChannel::ChrominanceV>());

    CHECK(pixel.template get<ImageChannel::Red>() == 1);
    CHECK(pixel.template get<ImageChannel::Green>() == 2);
    CHECK(pixel.template get<ImageChannel::Blue>() == 3);
    CHECK(get<ImageChannel::Red,uint8_t>(pixel) == 1);
    SPDLOG_INFO("Red: {} -> float {} ", get<ImageChannel::Red,uint8_t>(pixel), get<ImageChannel::Red,float>(pixel));
    CHECK(isNearZero(get<ImageChannel::Red,float>(pixel) - 1.0f/255.0f));
    CHECK(isNearZero(get<ImageChannel::Green,float>(pixel) - 2.0f/255.0f));
    CHECK(isNearZero(get<ImageChannel::Blue,float>(pixel) - 3.0f/255.0f));
    SPDLOG_INFO("Intensity {}", get<ImageChannel::Intensity,float>(pixel));
    SPDLOG_INFO("V {}", get<ImageChannel::ChrominanceV,float>(pixel));
    SPDLOG_INFO("U {}", get<ImageChannel::ChrominanceU,float>(pixel));
  }
  SECTION("PixelRGB32F")
  {
    PixelRGB32F pixel(1.0, 1.0, 0.0);
    CHECK(isNearZero(pixel.template get<ImageChannel::Red>() - 1.0f));
    CHECK(isNearZero(pixel.template get<ImageChannel::Green>() - 1.0f));
    CHECK(isNearZero(pixel.template get<ImageChannel::Blue>() - 0.0f));

    PixelYUV32F yuv;
    assign(yuv, pixel);
    SPDLOG_INFO(" RGB {} -> YUV {}  ", pixel, yuv);

    pixel = PixelRGB32F(0.0, 1.0, 1.0);
    assign(yuv, pixel);
    SPDLOG_INFO(" RGB {} -> YUV {}  ", pixel, yuv);

    pixel = PixelRGB32F(0.0, 1.0, 0.0);
    assign(yuv, pixel);
    SPDLOG_INFO(" RGB {} -> YUV {}  ", pixel, yuv);

    pixel = PixelRGB32F(0.0, 0.0, 1.0);
    assign(yuv, pixel);
    SPDLOG_INFO(" RGB {} -> YUV {}  ", pixel, yuv);

  }

  SECTION("PixelYUV32F")
  {
    const PixelYUV32F pixel(1.0, 0.0, 0.0);
    CHECK(isNearZero(pixel.template get<ImageChannel::Luminance>() - 1.0f));
    CHECK(isNearZero(pixel.template get<ImageChannel::ChrominanceU>() - 0.0f));
    CHECK(isNearZero(pixel.template get<ImageChannel::ChrominanceV>() - 0.0f));

    PixelRGB32F rgb;
    assign(rgb, pixel);

    //SPDLOG_INFO(" {} -> {}  ", pixel, rgb);

    CHECK(isNearZero(rgb.template get<ImageChannel::Red>() - 1.0f));
    CHECK(isNearZero(rgb.template get<ImageChannel::Green>() - 1.0f));
    CHECK(isNearZero(rgb.template get<ImageChannel::Blue>() - 1.0f));

    const PixelYUV32F pixel2(0.1, 0.2, 0.3);

    assign(rgb, pixel2);

    // SPDLOG_INFO(" {} -> {}  ", pixel2, rgb);

    PixelYUV32F pixel3;

    assign(pixel3, rgb);

    // SPDLOG_INFO(" {} -> {}  ", rgb, pixel3);

    CHECK(isNearZero(pixel3.template get<ImageChannel::Luminance>() - pixel2.template get<ImageChannel::Luminance>(),1e-5f));
    CHECK(isNearZero(pixel3.template get<ImageChannel::ChrominanceU>() - pixel2.template get<ImageChannel::ChrominanceU>(),1e-5f));
    CHECK(isNearZero(pixel3.template get<ImageChannel::ChrominanceV>() - pixel2.template get<ImageChannel::ChrominanceV>(),1e-5f));
  }

  SECTION("PixelYUV8")
  {
    const PixelYUV8 pixel(255, 128, 128);
    CHECK(pixel.template get<ImageChannel::Luminance>() == 255);

    PixelRGB8 rgb;
    assign(rgb, pixel);

    //SPDLOG_INFO(" {} -> {}  ", pixel, rgb);
    CHECK(rgb.template get<ImageChannel::Red>() == 255);
    CHECK(rgb.template get<ImageChannel::Green>() == 255);
    CHECK(rgb.template get<ImageChannel::Blue>() == 255);

    const PixelYUV8 pixel2(152, 90, 101);

    assign(rgb, pixel2);
    //SPDLOG_INFO(" {} -> {}  ", pixel2, rgb);

    PixelYUV8 pixel3;
    assign(pixel3, rgb);
    //SPDLOG_INFO(" {} -> {}  ", rgb, pixel3);

    // Check their close
    CHECK(std::abs(pixel3.template get<ImageChannel::Luminance>() - pixel2.template get<ImageChannel::Luminance>()) < 2);
    CHECK(std::abs(pixel3.template get<ImageChannel::ChrominanceU>() - pixel2.template get<ImageChannel::ChrominanceU>()) < 2);
    CHECK(std::abs(pixel3.template get<ImageChannel::ChrominanceV>() - pixel2.template get<ImageChannel::ChrominanceV>()) < 2);
  }

  SECTION("PixelRGBA")
  {
    PixelRGBA8 pixel(1, 2, 3, 4);
  }
}

