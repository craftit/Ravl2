
#include <ranges>
#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Array.hh"
#include "Ravl2/ArrayIterZip.hh"
#include "Ravl2/Geometry/Geometry.hh"

TEST_CASE("Array iterators conform to c++ concepts.", "[Array]")
{
  STATIC_REQUIRE(std::bidirectional_iterator<Ravl2::IndexRangeIterator<1> >);
  STATIC_REQUIRE(std::forward_iterator<Ravl2::IndexRangeIterator<2> >);


  STATIC_REQUIRE(std::ranges::range<Ravl2::IndexRange<1>>);
  STATIC_REQUIRE(std::ranges::range<Ravl2::IndexRange<2>>);

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

  STATIC_REQUIRE(std::input_iterator<Ravl2::ArrayIterZipN<1,int> >);
  STATIC_REQUIRE(std::input_iterator<Ravl2::ArrayIterZipN<1,int,float> >);
  STATIC_REQUIRE(std::input_iterator<Ravl2::ArrayIterZipN<2,int> >);
  STATIC_REQUIRE(std::input_iterator<Ravl2::ArrayIterZipN<2,int,float> >);

  using namespace Ravl2;

  static_assert(WindowedArray<ArrayAccess<int,2> ,int,2>, "ArrayAccess<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<ArrayView<int,2> ,int,2>, "ArrayView<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<Array<int,2> ,int,2>, "Array<int,2> does not satisfy WindowedArray");

  static_assert(WindowedArray<ArrayAccess<int,1> ,int,1>, "ArrayAccess<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<ArrayView<int,1> ,int,1>, "ArrayView<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<Array<int,1> ,int,1>, "Array<int,2> does not satisfy WindowedArray");

  static_assert(sizeof(Vector2f) == 2*sizeof(float),"Vector2f is not packed");
  static_assert(sizeof(Vector2d) == 2*sizeof(double),"Vector3d is not packed");
  static_assert(sizeof(Vector3f) == 3*sizeof(float),"Vector3f is not packed");
  static_assert(sizeof(Vector3d) == 3*sizeof(double),"Vector3d is not packed");
  static_assert(sizeof(Matrix3f) == 9*sizeof(float),"Matrix3f is not packed");
  static_assert(sizeof(Vector<uint8_t,3>) == 3*sizeof(uint8_t),"Vector<uint8_t,3> is not packed");
}


