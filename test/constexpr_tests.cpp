
#include <ranges>
#include <catch2/catch_test_macros.hpp>
#include "Ravl2/Array.hh"
#include "Ravl2/ArrayIterZip.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Isometry3.hh"
#include "Ravl2/ScanWindow.hh"
#include "Ravl2/FileIO.hh"
#include "Ravl2/SequenceIO.hh"

TEST_CASE("Array iterators conform to c++ concepts.", "[Array]")
{
  using namespace Ravl2;

  STATIC_REQUIRE(std::bidirectional_iterator<Ravl2::IndexRangeIterator<1> >);
  STATIC_REQUIRE(std::forward_iterator<Ravl2::IndexRangeIterator<2> >);
  static_assert(WindowedIterator<IndexRangeIterator<2> >, "ScanWindow<int,2> does not satisfy WindowedIterator");
  static_assert(WindowedIterator<IndexRangeIterator<3> >, "ScanWindow<int,3> does not satisfy WindowedIterator");

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

  STATIC_REQUIRE(std::indirectly_readable<Ravl2::InputStreamIterator<int> >);
  STATIC_REQUIRE(std::input_iterator<Ravl2::InputStreamIterator<int> >);
  // STATIC_REQUIRE(std::indirectly_writable<Ravl2::OutputStreamIterator<int>, int>);
//  STATIC_REQUIRE(std::output_iterator<Ravl2::OutputStreamIterator<int>, int>);

  static_assert(WindowedIterator<Ravl2::ArrayIterZipN<1,int> > , "ArrayIterZipN<1,int> does not satisfy WindowedIterator");
  static_assert(WindowedIterator<Ravl2::ArrayIterZipN<2,int> > , "ArrayIterZipN<2,int> does not satisfy WindowedIterator");

  static_assert(WindowedIterator<Ravl2::ArrayIterZipN<1,int,float> > , "ArrayIterZipN<1,int> does not satisfy WindowedIterator");
  static_assert(WindowedIterator<Ravl2::ArrayIterZipN<2,int,float> > , "ArrayIterZipN<2,int,float> does not satisfy WindowedIterator");

  static_assert(WindowedArray<ArrayAccess<int,2> ,int,2>, "ArrayAccess<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<ArrayView<int,2> ,int,2>, "ArrayView<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<Array<int,2> ,int,2>, "Array<int,2> does not satisfy WindowedArray");

  static_assert(WindowedArray<ArrayAccess<int,1> ,int,1>, "ArrayAccess<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<ArrayView<int,1> ,int,1>, "ArrayView<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<Array<int,1> ,int,1>, "Array<int,2> does not satisfy WindowedArray");

  static_assert(WindowedIterator<ArrayIter<int,2> >, "ArrayIter<int,2> does not satisfy WindowedIterator");
  static_assert(WindowedRowIterator<ArrayIter<int,2> >, "ArrayIter<int,2> does not satisfy WindowedRowIterator");

  static_assert(WindowedIterator<ScanWindow<int,1> >, "ScanWindow<int,1> does not satisfy WindowedIterator");
  static_assert(WindowedIterator<ScanWindow<int,2> >, "ScanWindow<int,2> does not satisfy WindowedIterator");

  static_assert(sizeof(Vector2f) == 2*sizeof(float),"Vector2f is not packed");
  static_assert(sizeof(Vector2d) == 2*sizeof(double),"Vector3d is not packed");
  static_assert(sizeof(Vector3f) == 3*sizeof(float),"Vector3f is not packed");
  static_assert(sizeof(Vector3d) == 3*sizeof(double),"Vector3d is not packed");
  static_assert(sizeof(Matrix3f) == 9*sizeof(float),"Matrix3f is not packed");
  static_assert(sizeof(Vector<uint8_t,3>) == 3*sizeof(uint8_t),"Vector<uint8_t,3> is not packed");
}


