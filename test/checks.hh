//
// Created by charles galambos on 22/08/2024.
//

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Assert.hh"
#include "Ravl2/Math.hh"

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_FALSE(a) CHECK_FALSE(a)
#define EXPECT_EQ(a, b) CHECK((a) == (b))
#define EXPECT_GT(a, b) CHECK((a) > (b))
#define EXPECT_NE(a, b) CHECK_FALSE((a) == (b))
#define ASSERT_TRUE(a) REQUIRE(a)
#define ASSERT_FALSE(a) REQUIRE_FALSE(a)
#define ASSERT_EQ(a, b) REQUIRE((a) == (b))
#define ASSERT_NE(a, b) REQUIRE_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a, b) REQUIRE(Ravl2::isNearZero((a) -(b),1e-6f))
#define EXPECT_FLOAT_EQ(a, b) CHECK(Ravl2::isNearZero((a) -(b),1e-6f))
#define EXPECT_NEAR(a, b, tol) CHECK(Ravl2::isNearZero((a) -(b),tol))

