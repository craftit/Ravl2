#include <ranges>
#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Array.hh"

TEST_CASE("Array iterators conform to c++ concepts.", "[Array]")
{
  STATIC_REQUIRE(std::random_access_iterator<Ravl2::ArrayIter<int,1> >);
  STATIC_REQUIRE(std::forward_iterator<Ravl2::ArrayIter<int,2> >);
  STATIC_REQUIRE(std::forward_iterator<Ravl2::ArrayIter<int,3> >);

  STATIC_REQUIRE(std::ranges::range<Ravl2::ArrayView<int,1>>);
  STATIC_REQUIRE(std::ranges::range<Ravl2::ArrayView<int,2>>);
  STATIC_REQUIRE(std::ranges::range<Ravl2::ArrayView<int,3>>);

  STATIC_REQUIRE(std::ranges::range<Ravl2::Array<int,1>>);
  STATIC_REQUIRE(std::ranges::range<Ravl2::Array<int,2>>);
  STATIC_REQUIRE(std::ranges::range<Ravl2::Array<int,3>>);

  STATIC_REQUIRE(std::ranges::range<Ravl2::ArrayAccess<int,1>>);
  STATIC_REQUIRE(std::ranges::range<Ravl2::ArrayAccess<int,2>>);
  STATIC_REQUIRE(std::ranges::range<Ravl2::ArrayAccess<int,3>>);
}

//TEST_CASE("Factorials are computed with constexpr", "[factorial]")
//{
//  STATIC_REQUIRE(factorial_constexpr(0) == 1);
//  STATIC_REQUIRE(factorial_constexpr(1) == 1);
//  STATIC_REQUIRE(factorial_constexpr(2) == 2);
//  STATIC_REQUIRE(factorial_constexpr(3) == 6);
//  STATIC_REQUIRE(factorial_constexpr(10) == 3628800);
//}

