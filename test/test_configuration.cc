//
// Created by Charles Galambos on 22/07/2021.
//

#include "Ravl2/Catch2checks.hh"
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
}