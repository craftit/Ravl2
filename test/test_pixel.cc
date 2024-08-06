//
// Created by charles galambos on 06/08/2024.
//

#include <catch2/catch_test_macros.hpp>

#include "Ravl2/Pixel/Pixel.hh"

TEST_CASE("Pixels")
{
  using namespace Ravl2;

  SECTION("PixelRGB")
  {
    Pixel<uint8_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue> pixel(1, 2, 3);
    CHECK(pixel.template get<ImageChannel::Red>() == 1);
    CHECK(pixel.template get<ImageChannel::Green>() == 2);
    CHECK(pixel.template get<ImageChannel::Blue>() == 3);
  }

  SECTION("PixelRGBA")
  {
    PixelRGBA8 pixel(1, 2, 3, 4);
  }
}

