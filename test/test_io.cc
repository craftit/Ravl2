
#include <any>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Assert.hh"
#include "Ravl2/IndexRangeSet.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/Cereal.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/InputSequence.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/OutputSequence.hh"

namespace Ravl2
{
  TEST_CASE("Conversions")
  {

    SECTION("Conversion search")
    {
      // This is a one-step conversion from int32_t to int16_t
      auto conv = typeConverterMap().find(typeid(int32_t), typeid(int16_t));
      CHECK(conv.has_value());

      // This is a two-step conversion from int64_t to int16_t
      auto conv2 = typeConverterMap().find(typeid(int64_t), typeid(int16_t));
      CHECK(conv2.has_value());

      //! This conversion does not exist
      auto conv3 = typeConverterMap().find(typeid(int16_t), typeid(std::string));
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

    SECTION("Binary Cereal into a directory that does not exist fails, without throwing")
    {
      // It used to throw cereal's "Failed to write 4 bytes" from inside ioSave.
      IndexRangeSet<2> rng({10, 10});
      nlohmann::json hints;
      hints["verbose"] = false;
      bool saved = true;
      CHECK_NOTHROW(saved = ioSave("no_such_directory_for_ravl2_test/test_cereal.xbs", rng, hints));
      CHECK_FALSE(saved);
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

  TEST_CASE("IO format selection")
  {
    constexpr std::string_view kProtocol = "ravl2-test";
    constexpr std::string_view kExtension = "ravl2test,ravl2alternate";
    auto makeInput = [](std::string name, std::string extension, std::string protocol, int priority, int marker) {
      return std::make_shared<InputFormatCall>(std::move(name), std::move(extension), std::move(protocol), priority,
                                               [marker](const ProbeInputContext &) -> std::optional<StreamInputPlan> {
                                                 auto stream = std::make_shared<StreamInputCall<int>>(
                                                   [marker](std::streampos &pos) -> std::optional<int> {
                                                     if(pos != std::streampos(0))
                                                       return std::nullopt;
                                                     pos += std::streamoff(1);
                                                     return marker;
                                                   });
                                                 return StreamInputPlan {stream, {}, 1.0f};
                                               });
    };
    auto makeOutput = [](std::string name, std::string extension, std::string protocol, int priority, int marker,
                         const std::shared_ptr<int> &selected) {
      return std::make_shared<OutputFormatCall>(std::move(name), std::move(extension), std::move(protocol), priority,
                                                [marker, selected](const ProbeOutputContext &) -> std::optional<StreamOutputPlan> {
                                                  auto stream = std::make_shared<StreamOutputCall<int>>(
                                                    [marker, selected](const int &, std::streampos pos) {
                                                      *selected = marker;
                                                      return pos;
                                                    });
                                                  return StreamOutputPlan {stream, {}, 1.0f};
                                                });
    };

    auto inputLow = makeInput("input-low", std::string(kExtension), std::string(kProtocol), 1, 1);
    auto inputHigh = makeInput("input-high", std::string(kExtension), std::string(kProtocol), 10, 10);
    auto inputWrongProtocol = makeInput("input-other-protocol", std::string(kExtension), "other", 100, 100);
    REQUIRE(inputFormatMap().add(inputLow));
    REQUIRE(inputFormatMap().add(inputHigh));
    REQUIRE(inputFormatMap().add(inputWrongProtocol));
    CHECK(inputHigh->supportsExtension("RAVL2ALTERNATE"));

    auto outputSelected = std::make_shared<int>(0);
    auto outputLow = makeOutput("output-low", std::string(kExtension), std::string(kProtocol), 1, 1, outputSelected);
    auto outputHigh = makeOutput("output-high", std::string(kExtension), std::string(kProtocol), 10, 10, outputSelected);
    auto outputWrongProtocol = makeOutput("output-other-protocol", std::string(kExtension), "other", 100, 100, outputSelected);
    REQUIRE(outputFormatMap().add(outputLow));
    REQUIRE(outputFormatMap().add(outputHigh));
    REQUIRE(outputFormatMap().add(outputWrongProtocol));
    CHECK(outputHigh->supportsExtension("RAVL2ALTERNATE"));

    const std::string uppercaseUrl = "ravl2-test://resource.RAVL2TEST";
    auto inputPlan = openInput(uppercaseUrl, typeid(int), nlohmann::json::object());
    REQUIRE(inputPlan.has_value());
    std::streampos inputPosition = inputPlan->mStream->beginOffset();
    auto inputValue = inputPlan->mStream->anyNext(inputPosition);
    REQUIRE(inputValue.has_value());
    CHECK(std::any_cast<int>(inputValue) == 10);

    auto outputPlan = openOutput(uppercaseUrl, typeid(int), nlohmann::json::object());
    REQUIRE(outputPlan.has_value());
    CHECK(outputPlan->mStream->anyWrite(std::any(42), 0) == std::streampos(0));
    CHECK(*outputSelected == 10);

    auto inputFallback = makeInput("input-fallback", "", std::string(kProtocol), 0, 20);
    REQUIRE(inputFormatMap().add(inputFallback));
    auto fallbackInputPlan = openInput("ravl2-test://resource.unknown", typeid(int), nlohmann::json::object());
    REQUIRE(fallbackInputPlan.has_value());
    inputPosition = fallbackInputPlan->mStream->beginOffset();
    CHECK(std::any_cast<int>(fallbackInputPlan->mStream->anyNext(inputPosition)) == 20);

    auto outputFallback = makeOutput("output-fallback", "", std::string(kProtocol), 0, 20, outputSelected);
    REQUIRE(outputFormatMap().add(outputFallback));
    auto fallbackOutputPlan = openOutput("ravl2-test://resource.unknown", typeid(int), nlohmann::json::object());
    REQUIRE(fallbackOutputPlan.has_value());
    (void)fallbackOutputPlan->mStream->anyWrite(std::any(42), 0);
    CHECK(*outputSelected == 20);
  }

  TEST_CASE("Input stream range iteration handles converted end of stream")
  {
    auto conversion = typeConverterMap().find(typeid(int64_t), typeid(int16_t));
    REQUIRE(conversion.has_value());
    auto format = std::make_shared<InputFormatCall>("converted-range", "ravl2range", "ravl2-test", 0,
                                                    [conversion](const ProbeInputContext &) -> std::optional<StreamInputPlan> {
                                                      auto stream = std::make_shared<StreamInputCall<int16_t>>(
                                                        [](std::streampos &pos) -> std::optional<int16_t> {
                                                          if(pos >= std::streampos(3))
                                                            return std::nullopt;
                                                          const auto value = static_cast<int16_t>(static_cast<std::streamoff>(pos) + 1);
                                                          pos += std::streamoff(1);
                                                          return value;
                                                        },
                                                        0, 3);
                                                      auto convert = [conversion = *conversion](std::any value) { return conversion(std::move(value)); };
                                                      return StreamInputPlan {stream, std::move(convert), conversion->conversionLoss()};
                                                    });
    REQUIRE(inputFormatMap().add(format));

    auto stream = openInputStream<int64_t>("ravl2-test://resource.ravl2range");
    REQUIRE(stream.valid());
    std::vector<int64_t> values;
    for(const auto value : stream)
      values.push_back(value);
    CHECK((values == std::vector<int64_t> {1, 2, 3}));
  }
}// namespace Ravl2