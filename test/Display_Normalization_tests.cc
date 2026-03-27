#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Ravl2/Display/Normalization.hh"

using Catch::Approx;
using namespace Ravl2::DebugDisplay;

TEST_CASE("Normalization finiteMinMax ignores NaN/Inf and returns sensible defaults") {
  const int W = 4, H = 3;
  float buf[H][W] = {
    { std::nanf(""), 2.f,  3.f,  std::numeric_limits<float>::infinity() },
    { -1.f,           0.f,  1.f,  5.f },
    { -2.f,          -3.f, -4.f, -5.f }
  };
  auto mm = finiteMinMax(&buf[0][0], W, H, W * int(sizeof(float)));
  REQUIRE(mm.first == Approx(-5.f));
  REQUIRE(mm.second == Approx(5.f));

  // All NaN -> default {0,1}
  float nans[W*H];
  std::fill_n(nans, W*H, std::nanf(""));
  auto mm2 = finiteMinMax(nans, W, H, W * int(sizeof(float)));
  REQUIRE(mm2.first == Approx(0.f));
  REQUIRE(mm2.second == Approx(1.f));

  // All equal finite -> default {0,1}
  float eq[W*H];
  std::fill_n(eq, W*H, 7.f);
  auto mm3 = finiteMinMax(eq, W, H, W * int(sizeof(float)));
  REQUIRE(mm3.first == Approx(0.f));
  REQUIRE(mm3.second == Approx(1.f));
}

TEST_CASE("Normalization percentiles basic and edge cases") {
  const int W = 10;
  float data[W] = {0,1,2,3,4,5,6,7,8,9};
  auto p_0_100 = percentiles(data, W, 1, W * int(sizeof(float)), 0.f, 100.f);
  REQUIRE(p_0_100.first == Approx(0.f));
  REQUIRE(p_0_100.second == Approx(9.f));

  auto p_10_90 = percentiles(data, W, 1, W * int(sizeof(float)), 10.f, 90.f);
  // For n=10, indices iLow=floor(0.1*9)=0, iHigh=floor(0.9*9)=8
  REQUIRE(p_10_90.first == Approx(0.f));
  REQUIRE(p_10_90.second == Approx(8.f));

  // low>high should swap internally
  auto p_swap = percentiles(data, W, 1, W * int(sizeof(float)), 90.f, 10.f);
  REQUIRE(p_swap.first == Approx(0.f));
  REQUIRE(p_swap.second == Approx(8.f));

  // Presence of NaN and Infs should be ignored
  float noisy[W] = {0,1,2, std::nanf(""), 4, 5, std::numeric_limits<float>::infinity(), 7, 8, 9};
  auto p_noisy = percentiles(noisy, W, 1, W * int(sizeof(float)), 0.f, 100.f);
  REQUIRE(p_noisy.first == Approx(0.f));
  REQUIRE(p_noisy.second == Approx(9.f));

  // All equal finite values -> vHigh adjusted away from vLow
  float eq[W]; std::fill_n(eq, W, 3.f);
  auto p_eq = percentiles(eq, W, 1, W * int(sizeof(float)), 25.f, 75.f);
  REQUIRE(p_eq.first == Approx(3.f));
  REQUIRE(p_eq.second > p_eq.first);
}

TEST_CASE("normalizeRowToU8 clamps and maps into [0,255]") {
  const int W = 5;
  float row[W] = { -1.f, 0.f, 0.5f, 1.f, std::numeric_limits<float>::infinity() };
  uint8_t out[W]{};
  normalizeRowToU8(row, out, W, 0.f, 1.f);
  // Expected: -1 -> 0, 0 -> 0, 0.5 -> ~128, 1 -> 255, +inf -> 0 after isfinite guard
  REQUIRE(out[0] == 0);
  REQUIRE(out[1] == 0);
  REQUIRE(out[2] == Approx(128).margin(1));
  REQUIRE(out[3] == 255);
  REQUIRE(out[4] == 0);
}
