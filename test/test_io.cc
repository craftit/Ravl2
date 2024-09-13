
#include <spdlog/spdlog.h>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "Ravl2/Assert.hh"
#include "Ravl2/IndexRangeSet.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/Cereal.hh"

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

TEST_CASE("File")
{
  using namespace Ravl2;
  initCerealIO();

  SECTION("Binary Cereal")
  {
    // Generate a temporary file name
    std::string filename = "test_cereal.xbs";
    IndexRangeSet<2> rng({10, 10});
    nlohmann::json hints;
    hints["verbose"] = false;
    CHECK(ioSave(filename, rng, hints));

    IndexRangeSet<2> rng2;
    CHECK(ioLoad(rng2, filename, hints));
    CHECK(rng == rng2);
    // Remove the file
    CHECK(std::remove(filename.c_str()) == 0);
  }

  SECTION("JSON Cereal")
  {
#if 0
    // Generate a temporary file name
    std::string filename = "test_cereal.json";
    IndexRangeSet<2> rng({10, 10});
    nlohmann::json hints;
    hints["verbose"] = true;
    CHECK(save(filename, rng,hints));

    IndexRangeSet<2> rng2;
    CHECK(load(rng2,filename,hints));
    CHECK(rng == rng2);
    // Remove the file
//    CHECK(std::remove(filename.c_str()) == 0);
#endif
  }
}

