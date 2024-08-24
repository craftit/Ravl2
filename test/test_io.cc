
#include <spdlog/spdlog.h>
#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Assert.hh"
#include "Ravl2/IO/TypeConverter.hh"

TEST_CASE("Conversions")
{
  using namespace Ravl2;


  SECTION("Conversion search")
  {
    // This is a one-step conversion from int32_t to int16_t
    auto conv = typeConverterMap().find(typeid(int32_t),typeid(int16_t));
    CHECK(conv.has_value());

    // This is a two-step conversion from int64_t to int16_t
    auto conv2 = typeConverterMap().find(typeid(int64_t),typeid(int16_t));
    CHECK(conv2.has_value());

    //! This conversion does not exist
    auto conv3 = typeConverterMap().find(typeid(int16_t),typeid(std::string));
    CHECK_FALSE(conv3.has_value());

  }

  SECTION("Conversion")
  {
    // This is a one-step conversion from int32_t to int16_t
    {
      auto conv = typeConverterMap().find(typeid(int32_t), typeid(int16_t));
      CHECK(conv.has_value());

      int16_t i16 = 0x1234;
      auto result = conv->convert(std::any(i16));
      REQUIRE(result.has_value());
      auto i32 = std::any_cast<int32_t>(result);
      CHECK(i32 == int32_t(i16));
    }
    {
      auto conv = typeConverterMap().find(typeid(int64_t), typeid(int16_t));
      CHECK(conv.has_value());

      int16_t i16 = 0x1234;
      auto result = conv->convert(std::any(i16));
      REQUIRE(result.has_value());
      auto i64 = std::any_cast<int64_t>(result);
      CHECK(i64 == int64_t(i16));
    }

    {
      int16_t i16 = 0x1234;
      auto i64 = typeConvert<int64_t>(i16);
      CHECK(i64 == int64_t(i16));
    }

  }

}
