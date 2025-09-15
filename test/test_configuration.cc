//
// Created by Charles Galambos on 22/07/2021.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Configuration.hh"
#include "Ravl2/Logging.hh"

class TestClass
{
public:
  explicit TestClass(Ravl2::Configuration &config)
  {
    m_a = config.getNumber("value", "test 1", 0, 0, 100);
  }

  int m_a = 0;
};

[[maybe_unused]] static bool g_register1 = Ravl2::defaultConfigFactory().registerDirectType<TestClass>("TestClass");

TEST_CASE("Configuration")
{
  SECTION("Values")
  {
    Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( { "a":1,"b":2 } )");

    ASSERT_EQ(config.getNumber("a", "test 1", 0, 0, 100), 1);
    ASSERT_EQ(config.getNumber("b", "test 2", 0, 0, 100), 2);
  }
  
  SECTION("Vector")
  {
    Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( { "a":[1,2,3],"b":[4,5,6] } )");
    
    auto a = config.getNumericVector<float>("a", "test 1", 0.0f, 0.0f, 100.0f,3);
    auto b = config.getNumericVector<float>("b", "test 2", 0.0f, 0.0f, 100.0f,3);
    ASSERT_EQ(a.size(), 3);
    ASSERT_EQ(b.size(), 3);
    ASSERT_EQ(a[0], 1);
    ASSERT_EQ(a[1], 2);
    ASSERT_EQ(a[2], 3);
    ASSERT_EQ(b[0], 4);
    ASSERT_EQ(b[1], 5);
    ASSERT_EQ(b[2], 6);
    
    // Get as a point
    auto pnt = config.getPoint<float,3>("a", "test 1", 0.0f, 0.0f, 100.0f);
    ASSERT_EQ(pnt[0], 1);
    ASSERT_EQ(pnt[1], 2);
    ASSERT_EQ(pnt[2], 3);
  }
  
  SECTION("Matrix")
  {
    Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( { "a":[1,2,3,4,5,6,7,8,9] } )");
    
    auto mat =  config.getMatrix<float,3,3>("a", "test 1", 0.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(mat(0,0),1);
    EXPECT_FLOAT_EQ(mat(0,1),2);
    EXPECT_FLOAT_EQ(mat(0,2),3);
    EXPECT_FLOAT_EQ(mat(1,0),4);
    EXPECT_FLOAT_EQ(mat(1,1),5);
    EXPECT_FLOAT_EQ(mat(1,2),6);
    EXPECT_FLOAT_EQ(mat(2,0),7);
    EXPECT_FLOAT_EQ(mat(2,1),8);
    EXPECT_FLOAT_EQ(mat(2,2),9);
  }
  
  
  SECTION("UseGroup")
  {
    Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( {
	 "group": {
		"a": { "value":1 },
	  "b": { "value":2 }
	 }
} )");

    std::vector<TestClass> values;
    config.useGroup("group", "test 1", values);
    ASSERT_EQ(values.size(), 2);
    ASSERT_EQ(values[0].m_a, 1);
    ASSERT_EQ(values[1].m_a, 2);
  }

  SECTION("AnonChild")
  {
    Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( {
           "group": {
                  "a": { "value":1 },
            "b": { "value":2 }
           }
  } )");
    auto groupConfig = config.child("group", "hello");
    std::shared_ptr<TestClass> testVal1;
    std::shared_ptr<TestClass> testVal2;
    groupConfig.useComponent("a", "test 1", testVal1);
    groupConfig.useComponent("b", "test 1", testVal2);
    EXPECT_EQ(testVal1->m_a, 1);
    EXPECT_EQ(testVal2->m_a, 2);
  }

  SECTION("UnusedField")
  {
    Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( {
                  "a": { "value":1, "extra":2 }
  } )");

    std::shared_ptr<TestClass> testVal1;
    CHECK_THROWS(config.useComponent("a", "test 1", testVal1));
  }

  SECTION("Duration")
  {
    // Test with numeric values (integer and floating point)
    Ravl2::Configuration config1 = Ravl2::Configuration::fromJSONString(R"( {
      "intSeconds": 42,
      "floatSeconds": 10.5
    } )");

    // Integer seconds
    auto seconds = config1.getDuration<int64_t>("intSeconds", "Integer seconds", std::chrono::seconds(0));
    ASSERT_EQ(seconds.count(), 42);

    // Floating point seconds
    auto floatSeconds = config1.getDuration<double>("floatSeconds", "Float seconds", std::chrono::duration<double>(0));
    ASSERT_EQ(floatSeconds.count(), 10.5);

    // Test with string values and different units
    Ravl2::Configuration config2 = Ravl2::Configuration::fromJSONString(R"( {
      "seconds": "30s",
      "milliseconds": "500ms",
      "microseconds": "750us",
      "minutes": "5min",
      "hours": "2h",
      "floatHours": "1.5h",
      "plainNumber": "42"
    } )");

    // String with seconds unit
    auto stringSeconds = config2.getDuration<int64_t>("seconds", "String seconds", std::chrono::seconds(0));
    ASSERT_EQ(stringSeconds.count(), 30);

    // String with milliseconds unit
    auto ms = config2.getDuration<int64_t, std::milli>("milliseconds", "Milliseconds", std::chrono::milliseconds(0));
    ASSERT_EQ(ms.count(), 500);

    // String with microseconds unit
    auto us = config2.getDuration<int64_t, std::micro>("microseconds", "Microseconds", std::chrono::microseconds(0));
    ASSERT_EQ(us.count(), 750);

    // String with minutes unit
    auto minutes = config2.getDuration<int64_t, std::ratio<60>>("minutes", "Minutes", std::chrono::minutes(0));
    ASSERT_EQ(minutes.count(), 5);

    // String with hours unit
    auto hours = config2.getDuration<int64_t, std::ratio<3600>>("hours", "Hours", std::chrono::hours(0));
    ASSERT_EQ(hours.count(), 2);

    // String with floating point hours
    auto floatHours = config2.getDuration<double, std::ratio<3600>>("floatHours", "Float hours", std::chrono::duration<double, std::ratio<3600>>(0));
    ASSERT_EQ(floatHours.count(), 1.5);

    // String with a plain number (should be interpreted as seconds)
    auto plainNumber = config2.getDuration<int64_t>("plainNumber", "Plain number", std::chrono::seconds(0));
    ASSERT_EQ(plainNumber.count(), 42);

    // Test unit conversion (get seconds as milliseconds)
    auto secondsAsMs = config2.getDuration<int64_t, std::milli>("seconds", "Seconds as milliseconds", std::chrono::milliseconds(0));
    ASSERT_EQ(secondsAsMs.count(), 30000); // 30s = 30000ms

    // Test floating point with integer duration
    auto floatAsInt = config2.getDuration<int64_t>("floatHours", "Float hours as int", std::chrono::seconds(0));
    ASSERT_EQ(floatAsInt.count(), 5400); // 1.5h = 5400s
  }

  SECTION("DurationDefaults")
  {
    //Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::initializeLogging();
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( {} )");

    // Test with a default value
    SPDLOG_INFO("Checking default value");
    auto defaultValue = config.getDuration<int64_t>("nonexistent", "Uses default", std::chrono::seconds(60));
    SPDLOG_INFO("Default value: {}", defaultValue.count());
    ASSERT_EQ(defaultValue.count(), 60);

    // Check we throw if we give an invalid default value.
    CHECK_THROWS(config.getDuration<int64_t>("seconds", "With minimum", std::chrono::seconds(30), std::chrono::seconds(60)));
  }

  SECTION("DurationErrors")
  {
    Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
    Ravl2::Configuration config = Ravl2::Configuration::fromJSONString(R"( {
      "invalid": "not a duration",
      "outOfRange": "1000s"
    } )");

    // Test with invalid string format
    CHECK_THROWS(config.getDuration<int64_t>("invalid", "Invalid format", std::chrono::seconds(0)));

    // Test with value out of range
    CHECK_THROWS(config.getDuration<int64_t>("outOfRange", "Out of range", std::chrono::seconds(0), std::chrono::seconds(0), std::chrono::seconds(500)));
  }
}