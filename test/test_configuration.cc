//
// Created by Charles Galambos on 22/07/2021.
//

#include "checks.hh"
#include "Ravl2/Configuration.hh"

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
}