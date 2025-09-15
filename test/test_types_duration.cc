//
// Created on 15/09/2025.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Types.hh"
#include <chrono>

TEST_CASE("fromStringToDuration")
{
  SECTION("Basic parsing - seconds")
  {
    // Default interpretation as seconds for plain numbers
    auto seconds1 = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("42");
    REQUIRE(seconds1.has_value());
    ASSERT_EQ(seconds1.value().count(), 42);

    // Explicit seconds
    auto seconds2 = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("42s");
    REQUIRE(seconds2.has_value());
    ASSERT_EQ(seconds2.value().count(), 42);

    // Floating point seconds
    auto seconds3 = Ravl2::fromStringToDuration<double, std::ratio<1>>("42.5");
    REQUIRE(seconds3.has_value());
    ASSERT_EQ(seconds3.value().count(), 42.5);

    auto seconds4 = Ravl2::fromStringToDuration<double, std::ratio<1>>("42.5s");
    REQUIRE(seconds4.has_value());
    ASSERT_EQ(seconds4.value().count(), 42.5);
  }

  SECTION("Different time units - integer representation")
  {
    // Hours
    auto hours = Ravl2::fromStringToDuration<int64_t, std::ratio<3600>>("2h");
    REQUIRE(hours.has_value());
    ASSERT_EQ(hours.value().count(), 2);

    // Minutes
    auto minutes = Ravl2::fromStringToDuration<int64_t, std::ratio<60>>("30min");
    REQUIRE(minutes.has_value());
    ASSERT_EQ(minutes.value().count(), 30);

    // Milliseconds
    auto ms = Ravl2::fromStringToDuration<int64_t, std::milli>("500ms");
    REQUIRE(ms.has_value());
    ASSERT_EQ(ms.value().count(), 500);

    // Microseconds
    auto us = Ravl2::fromStringToDuration<int64_t, std::micro>("100us");
    REQUIRE(us.has_value());
    ASSERT_EQ(us.value().count(), 100);

    // Nanoseconds
    auto ns = Ravl2::fromStringToDuration<int64_t, std::nano>("750ns");
    REQUIRE(ns.has_value());
    ASSERT_EQ(ns.value().count(), 750);
  }

  SECTION("Different time units - floating point representation")
  {
    // Hours with floating point
    auto hours = Ravl2::fromStringToDuration<double, std::ratio<3600>>("1.5h");
    REQUIRE(hours.has_value());
    ASSERT_EQ(hours.value().count(), 1.5);

    // Minutes with floating point
    auto minutes = Ravl2::fromStringToDuration<float, std::ratio<60>>("2.5min");
    REQUIRE(minutes.has_value());
    ASSERT_EQ(minutes.value().count(), 2.5f);

    // Milliseconds with floating point
    auto ms = Ravl2::fromStringToDuration<double, std::milli>("10.75ms");
    REQUIRE(ms.has_value());
    ASSERT_EQ(ms.value().count(), 10.75);
  }

  SECTION("Unit conversion")
  {
    // Convert hours to seconds
    auto hours_as_seconds = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("2h");
    REQUIRE(hours_as_seconds.has_value());
    ASSERT_EQ(hours_as_seconds.value().count(), 7200); // 2 hours = 7200 seconds

    // Convert minutes to milliseconds
    auto minutes_as_ms = Ravl2::fromStringToDuration<int64_t, std::milli>("3min");
    REQUIRE(minutes_as_ms.has_value());
    ASSERT_EQ(minutes_as_ms.value().count(), 180000); // 3 minutes = 180000 milliseconds

    // Convert seconds to microseconds
    auto seconds_as_us = Ravl2::fromStringToDuration<int64_t, std::micro>("1.5s");
    REQUIRE(seconds_as_us.has_value());
    ASSERT_EQ(seconds_as_us.value().count(), 1500000); // 1.5 seconds = 1500000 microseconds
  }

  SECTION("Error handling")
  {
    // Empty string
    auto empty = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("");
    REQUIRE_FALSE(empty.has_value());

    // Non-numeric input
    auto non_numeric = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("abc");
    REQUIRE_FALSE(non_numeric.has_value());

    // Invalid unit
    auto invalid_unit = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("10xyz");
    REQUIRE_FALSE(invalid_unit.has_value());

    // No number part
    auto no_number = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("s");
    REQUIRE_FALSE(no_number.has_value());
  }

  SECTION("Whitespace handling")
  {
    // Space between number and unit
    auto with_space = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("42 s");
    REQUIRE(with_space.has_value());
    ASSERT_EQ(with_space.value().count(), 42);

    // Multiple spaces
    auto multiple_spaces = Ravl2::fromStringToDuration<int64_t, std::ratio<1>>("42   s");
    REQUIRE(multiple_spaces.has_value());
    ASSERT_EQ(multiple_spaces.value().count(), 42);
  }
}
