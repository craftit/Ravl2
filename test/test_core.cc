/*
 * testRavl2Core.cc
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <cereal/archives/json.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/Math.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/ArrayIterZip.hh"
#include "Ravl2/ScanWindow.hh"

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))

TEST_CASE("Math", "[int_floor]")
{
  // Check the int_floor
  CHECK_EQ(Ravl2::int_floor(1.0),1);
  CHECK_EQ(Ravl2::int_floor(0.1),0);
  CHECK_EQ(Ravl2::int_floor(0.9),0);
  CHECK_EQ(Ravl2::int_floor(-0.1),-1);
  CHECK_EQ(Ravl2::int_floor(0.0),0);
  CHECK_EQ(Ravl2::int_floor(-0.0),0);
  CHECK_EQ(Ravl2::int_floor(-1.0),-1);
  CHECK_EQ(Ravl2::int_floor(-2.0),-2);

  // Check the int_round
  CHECK_EQ(Ravl2::int_round(0.1),0);
  CHECK_EQ(Ravl2::int_round(0.9),1);
  CHECK_EQ(Ravl2::int_round(-0.1),0);
  CHECK_EQ(Ravl2::int_round(0.0),0);
  CHECK_EQ(Ravl2::int_round(-0.0),0);
  CHECK_EQ(Ravl2::int_round(-0.9),-1);

  // Check the sign
  CHECK_EQ(Ravl2::sign(1),1);
  CHECK_EQ(Ravl2::sign(0),0);
  CHECK_EQ(Ravl2::sign(-1),-1);
  CHECK_EQ(Ravl2::sign(0.1),1);
  CHECK_EQ(Ravl2::sign(-0.1),-1);
  CHECK_EQ(Ravl2::sign(0.0),0);
}


TEST_CASE("Array")
{
  SECTION("Array 1d")
  {
    // Test creation of 1 dimensional array.

    Ravl2::IndexRange<1> aRange(0,9);
    Ravl2::Array<int,1> val(aRange);

    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    c = 0;
    for(auto a : aRange) {
      CHECK_EQ(val[a],c++);
    }

    c = 0;
    for(int i = aRange.min();i <= aRange.max();i++) {
      CHECK_EQ(val[i],c++);
    }

    CHECK(c == 10);
  }

  SECTION("Array<int,1> Cereal. ")
  {
    Ravl2::Array<int,1> array1(Ravl2::IndexRange<1>(0,4));
    for(auto a : array1.range()) {
      array1[a] = a;
    }

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(array1);
    }
    //SPDLOG_INFO("Array<int,1>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Ravl2::Array<int,1> array2;
      iarchive(array2);
      CHECK(array1.range() == array2.range());
    }
  }


  SECTION("Array 2d")
  {
    Ravl2::IndexRange<2> aRange {10,11};

    // Create a 2-dimensional array of objects.
    Ravl2::Array<int,2> val(aRange);

    // Write some data
    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    // Check what we wrote is still there.
    c = 0;
    for(auto a : aRange) {
      CHECK_EQ(val[a],c++);
    }

    // Index dimensions individually
    c = 0;
    for(auto i : aRange[0]) {
      for(auto j : aRange[1]) {
        CHECK_EQ(val[i][j],c++);
      }
    }

    // Using plain c style indexing.
    c = 0;
    for(int i = aRange[0].min();i <= aRange[0].max();i++) {
      for(int j = aRange[1].min();j <= aRange[1].max();j++) {
        CHECK_EQ(val[i][j],c++);
      }
    }
    CHECK_EQ(c,110);
  }

  SECTION("Array 3d")
  {
    // Test creation of 3-dimensional array.

    Ravl2::IndexRange<3> aRange {10,11,12};
    Ravl2::Array<int,3> val(aRange);

    // Write some data
    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    // Check what we wrote is still there.
    c = 0;
    for(auto a : aRange) {
      CHECK_EQ(val[a],c++);
    }

    // Index dimensions individually
    c = 0;
    for(auto i : aRange[0]) {
      for(auto j : aRange[1]) {
        for(auto k : aRange[2]) {
          CHECK_EQ(val[i][j][k],c++);
        }
      }
    }

    // Using plain c style indexing.
    c = 0;
    for(int i = aRange[0].min();i <= aRange[0].max();i++) {
      for(int j = aRange[1].min();j <= aRange[1].max();j++) {
        for(int k = aRange[2].min();k <= aRange[2].max();k++) {
          CHECK_EQ(val[i][j][k],c++);
        }
      }
    }

    CHECK_EQ(c,1320);
  }

  SECTION("Array 3d Partial index")
  {
    Ravl2::IndexRange<3> aRange {10,11,12};
    Ravl2::Array<int,3> val(aRange);

    int i = 0;
    for(auto a : aRange) {
      val[a] = i++;
    }

    Ravl2::Index<2> idx = {0,0};
    CHECK(val[idx][0] == 0);
    CHECK(val[0][idx] == 0);
    CHECK(val[idx][1] == 1);
    CHECK(val[idx][2] == 2);
    CHECK(val[idx].range().size() == 12);
    CHECK(val[Ravl2::toIndex(1,2)][3] == val[1][2][3]);
    CHECK(val[1][Ravl2::toIndex(2,3)] == val[1][2][3]);

    // Check const access

    const Ravl2::Array<int,3> &cval = val;
    CHECK(cval[idx][0] == 0);
    CHECK(cval[0][idx] == 0);
    Ravl2::Index<2> idx2 = {0,1};
    CHECK(cval[0][idx2] == 1);
    CHECK(cval[0][Ravl2::toIndex(0,3)] == 3);
    CHECK(cval[Ravl2::toIndex(1,2)][3] == cval[1][2][3]);
    CHECK(cval[1][Ravl2::toIndex(2,3)] == cval[1][2][3]);
  }
  SECTION("Array Access Assignment")
  {
    Ravl2::IndexRange<3> aRange {10,11,3};
    Ravl2::Array<int,3> val(aRange);
    int i = 0;
    for(auto a : aRange) {
      val[a] = i++;
    }

    Ravl2::Array<int,3> val2(aRange, 0);
    auto dest = val[0][0];
    std::copy(dest.begin(),dest.end(),val2[0][1].begin());
    //! Copy data from another array

    CHECK(val2[0][1][0] == val[0][0][0]);
    CHECK(val2[0][1][1] == val[0][0][1]);
    CHECK(val2[0][1][2] == val[0][0][2]);
  }


  SECTION("Conversion from a vector")
  {
    // Test conversion from a c++ vector

    std::vector<int> acvec {1,2,3};
    Ravl2::Array<int,1> anAccess(acvec);

    CHECK(anAccess.range().min() == 0);
    CHECK(anAccess.range().max() == 2);
    CHECK(anAccess.range().size() == 3);

    CHECK_EQ(anAccess[0],1);
    CHECK_EQ(anAccess[1],2);
    CHECK_EQ(anAccess[2],3);
  }

  SECTION("Move from a vector")
  {
    // Test move from a c++ vector

    std::vector<int> acvec {1,2,3};
    Ravl2::Array<int,1> anAccess(std::move(acvec));

    CHECK(anAccess.range().min() == 0);
    CHECK(anAccess.range().max() == 2);
    CHECK_EQ(anAccess.range().size(),3);
    CHECK_EQ(anAccess[0],1);
    CHECK_EQ(anAccess[1],2);
    CHECK_EQ(anAccess[2],3);
  }

  SECTION("Array<int,2> Cereal. ")
  {
    Ravl2::Array<int,2> array1(Ravl2::IndexRange<2>({{0,4},{1,6}}));
    int count = 0;
    for(auto &a : array1) {
      a = count++;
    }

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(array1);
    }
    //SPDLOG_INFO("Array<int,2>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Ravl2::Array<int,2> array2;
      iarchive(array2);
      CHECK(array1.range() == array2.range());
    }
  }
}

TEST_CASE("ArrayIter1", "[ArrayIter<1>]")
{
  Ravl2::Array<int,1> val(Ravl2::IndexRange<1>(0,9));
  int sum = 0;
  for(auto a : val.range()) {
    val[a] = a;
    sum += val[a];
  }
  CHECK_EQ(sum,45);
  sum = 0;
  for(auto a : val) {
    sum += a;
  }
  CHECK_EQ(sum,45);
}

TEST_CASE("ArrayAccessIter1", "[ArrayIter<1>]")
{
  Ravl2::Array<int,1> val(Ravl2::IndexRange<1>(0,9));
  Ravl2::ArrayAccess<int,1> view(val);
  int sum = 0;
  for(auto a : view.range()) {
    val[a] = a;
    sum += val[a];
  }
  CHECK_EQ(sum,45);
  sum = 0;
  for(auto a : view) {
    sum += a;
  }
  CHECK_EQ(sum,45);
}

TEST_CASE("ArrayIter2", "[ArrayIter<N>]")
{
  // Check 1x2 case
  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({1, 2}));
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    ASSERT_EQ(iter, iter);
    ASSERT_NE(iter, end);
    CHECK_EQ(*iter, 0);
    CHECK(iter.valid());
    CHECK_EQ(iter.index(), Ravl2::Index<2>({0, 0}));
    ++iter;
    ASSERT_EQ(iter, iter);
    ASSERT_NE(iter, end);
    CHECK_EQ(*iter, 1);
    CHECK(iter.valid());
    CHECK_EQ(iter.index(), Ravl2::Index<2>({0, 1}));
    ++iter;
    CHECK_EQ(iter, end);
    CHECK_FALSE(iter.valid());
  }

  // Check 2x1 case
  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({2, 1}));
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    CHECK_EQ(iter, iter);
    ASSERT_NE(iter, end);
    CHECK_EQ(*iter, 0);
    CHECK(iter.valid());
    CHECK_EQ(iter.index(), Ravl2::Index<2>({0, 0}));
    ++iter;
    CHECK_EQ(iter, iter);
    ASSERT_NE(iter, end);
    CHECK_EQ(*iter, 1);
    CHECK(iter.valid());
    CHECK_EQ(iter.index(), Ravl2::Index<2>({1, 0}));
    ++iter;
    CHECK_EQ(iter, iter);
    CHECK_EQ(iter, end);
    CHECK_FALSE(iter.valid());
  }

  // Check 1x1 case
  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({1, 1}));
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    CHECK_EQ(iter, iter);
    ASSERT_NE(iter, end);
    CHECK_EQ(*iter, 0);
    CHECK(iter.valid());
    CHECK_EQ(iter.index(), Ravl2::Index<2>({0, 0}));
    ++iter;
    CHECK_EQ(iter, end);
    CHECK_FALSE(iter.valid());
  }
  // Check 2x2 case
  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({2, 2}));
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    for(int i = 0;i < 4;i++)
    {
      CHECK_EQ(iter, iter);
      ASSERT_NE(iter, end);
      CHECK_EQ(*iter, i);
      CHECK(iter.valid());
      ++iter;
    }
    CHECK_EQ(iter, end);
    CHECK_FALSE(iter.valid());
    CHECK(iter.done());
  }

  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({10, 20}));
    int sum = 0;
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
      sum += val[a];
    }

    auto iter = val.begin();
    CHECK_EQ(*iter, 0);
    CHECK_EQ(iter, iter);

    auto iterEnd = val.end();
    ASSERT_NE(iter, iterEnd);

    ++iter;
    CHECK_EQ(*iter, 1);
    ASSERT_NE(iter, iterEnd);
    CHECK_EQ(sum, 19900);
    sum = 0;
    for (auto a: val) {
      sum += a;
    }
    CHECK_EQ(sum, 19900);
  }

}

TEST_CASE("ArrayIter2Offset", "[ArrayIter<N>]")
{
  Ravl2::Array<int, 2> kernel(Ravl2::IndexRange<2>(Ravl2::IndexRange<1>(1,2),Ravl2::IndexRange<1>(3,5)));
  int at = 0;
  for(auto a: kernel.range())
  {
    //SPDLOG_INFO("Buffer: At {} = {}  @ {} ", a, at, (void *) &kernel[a]);
    kernel[a] = at++;
  }

  //SPDLOG_INFO("Strides: {} {} ", kernel.strides()[0], kernel.strides()[1]);

  at = 0;
  for(int i = 0;i < 6;i++,at++)
  {
    auto x = kernel.buffer().get()[i];
    //SPDLOG_INFO("Buffer: {} ", x);
    CHECK_EQ(x,at);
  }

  auto it = kernel.begin();
  auto end = kernel.end();
  for(int i = 0;i < 6;++i)
  {
    //SPDLOG_INFO("Data: {} {} @ {} ", i, *it, (void *) &(*it));
    CHECK_EQ(*it,i);
    ASSERT_NE(it,end);
    CHECK(it.valid());
    CHECK_FALSE(it.done());
    ++it;
  }
  CHECK_FALSE(it.valid());
  CHECK(it.done());
  CHECK_EQ(it,end);
}

TEST_CASE("ArrayIter3",  "[ArrayIter<N>]")
{
  // Check 2x3x4 case
  {
    Ravl2::Array<int, 3> val(Ravl2::IndexRange<3>({2, 3, 4}));
    int at = 0;
    for(auto a : val.range())
    {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    int count = val.range().area();
    CHECK_EQ(count, 2 * 3 * 4);
    for(int i = 0; i < count; i++)
    {
      CHECK_EQ(iter, iter);
      ASSERT_NE(iter, end);
      CHECK_EQ(*iter, i);
      CHECK(iter.valid());
      ++iter;
    }
    CHECK_EQ(iter, end);
    CHECK_FALSE(iter.valid());
  }
}


TEST_CASE("ArrayIter2View", "[ArrayIter<N>]")
{
  Ravl2::Array<int, 2> matrix(Ravl2::IndexRange<2>({4, 4}));
  int at = 0;
  for(auto a : matrix.range())
  {
    //SPDLOG_INFO("Matrix: At {} = {}  @ {} ", a, at, (void *) &matrix[a]);
    matrix[a] = at++;
  }
  //SPDLOG_INFO("Strides: {} {} ", matrix.strides()[0], matrix.strides()[1]);

  Ravl2::IndexRange<2> win({2, 2});
  auto view = matrix.access(win);
  //auto view = matrix.view(win);

  int targetSum = 0;
  for(auto a : view.range())
  {
    targetSum += matrix[a];
  }

  auto it = view.begin();
  auto end = view.end();
  //SPDLOG_INFO("Window range: {}  Win:{} ", view.range(),win);
  CHECK_EQ(view.range(), win);
  int area = win.area();
  for(int i = 0;i < area;++i)
  {
    CHECK(it.valid());
    CHECK_FALSE(it.done());
    ASSERT_NE(it,end);
    //SPDLOG_INFO("Data: {} {} @ {} ", i, *it, (void *) &(*it));
    ++it;
  }
  CHECK_FALSE(it.valid());
  CHECK(it.done());
  CHECK_EQ(it,end);

  int sum = 0;
  for(auto a : view)
  {
    sum += a;
  }
  CHECK_EQ(sum, targetSum);
}

TEST_CASE("ShiftView", "Array<N>")
{
  Ravl2::Array<int, 2> matrix(Ravl2::IndexRange<2>({5, 5}));
  int at = 0;
  for(auto a : matrix.range())
  {
    matrix[a] = at++;
  }
  Ravl2::Array<int, 2> kernel(Ravl2::IndexRange<2>({3, 3}));
  at = 0;
  int sumKernel = 0;
  for(auto a : kernel.range())
  {
    sumKernel += at;
    kernel[a] = at++;
  }

  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  for(auto si : scanRange)
  {
    Ravl2::IndexRange<2> rng = kernel.range() + si;
    auto view = matrix.access(rng);
    //auto view = matrix.view(rng);

    int sum1 = 0;
    int sum2 = 0;
    for(auto it = Ravl2::ArrayIterZipN<2, int, int>(kernel, view); !it.done(); ++it)
    {
      //SPDLOG_INFO("Data1: {} Data2: {}", it.data1(), it.data2());
      sum1 += it.data<0>();
      sum2 += it.data<1>();
    }
    CHECK_EQ(sum1,sumKernel);
    int sum3 = 0;
    for(auto a : view)
    {
      sum3 += a;
    }
    CHECK_EQ(sum2,sum3);
    //SPDLOG_INFO("\n");
  }
}

TEST_CASE("ZipN", "[ZipIterN]")
{
  using namespace Ravl2;
  Array<int, 2> a({4, 4});
  Array<unsigned , 2> b({4, 4});
  int at = 0;
  for(auto ai : a.range())
  {
    a[ai] = at;
    b[ai] = unsigned (at++);
  }
  auto it = begin(a, b);
  int count = 0;
  CHECK_FALSE(it.done());
  CHECK(it.valid());
  CHECK(it.index<0>() == Index<2>({0, 0}));

  for(;it.valid();++it)
  {
    //SPDLOG_INFO("Data: {} {}  @ {} ", it.data<0>(), it.data<1>(),it.index<0>());
    CHECK(a.range().contains(it.index<0>()));
    CHECK_FALSE(it.done());
    CHECK_EQ(it.data<0>(), int(it.data<1>()));
    CHECK(count < 16);
    count++;
  }
  CHECK_EQ(count, 16);
}


TEST_CASE("ScanWindow2", "[ScanWindow]")
{
  Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({11, 10}));
  int at = 0;
  for (auto a: val.range()) {
    val[a] = at++;
  }

  Ravl2::IndexRange<2> windowRange({2, 2});
  Ravl2::ScanWindow<int, 2> scan(val, windowRange);

  CHECK_EQ(scan.scanArea().area(), 9 * 10);

  int count = 0;
  auto win = scan.window();
  CHECK_EQ(win.range().area(), 2 * 2);
  CHECK_EQ(win.range().size(0), 2);
  CHECK_EQ(win.range().size(1), 2);
  CHECK_EQ(win[0][0], 0);
  CHECK_EQ(win[0][1], 1);
  CHECK_EQ(win[1][0], 10);
  CHECK_EQ(win[1][1], 11);

  {
    int sum = 0;
    for (auto x: win)
      sum += x;
    CHECK_EQ(sum, (0 + 1 + 10 + 11));
  }

  count += win.range().area();
  ++scan;

  win = scan.window();
  CHECK_EQ(win.range().area(), 2 * 2);

  CHECK_EQ(win[0][0], 1);
  CHECK_EQ(win[0][1], 2);
  CHECK_EQ(win[1][0], 11);
  CHECK_EQ(win[1][1], 12);
  count += win.range().area();
  ++scan;

  // Keep going until the next row
  for(int i = 0;i < 7;i++)
  {
    win = scan.window();
    CHECK_EQ(win.range().area(), 2 * 2);
    count += win.range().area();
    ++scan;
  }

  // Check the window is on the next row
  win = scan.window();
  CHECK_EQ(win.range().area(), 2 * 2);
  CHECK_EQ(win[0][0], 10);
  CHECK_EQ(win[0][1], 11);
  CHECK_EQ(win[1][0], 20);
  CHECK_EQ(win[1][1], 21);

  {
    int sum = 0;
    for (auto x: win)
      sum += x;
    CHECK_EQ(sum, (10 + 11 + 20 + 21));
  }

  count += win.range().area();
  ++scan;

  while (!scan.done()) {
    auto window = scan.window();
    for (auto a: window.range()) {
      //std::cout << " " << window[a] << "\n";
      CHECK(window.range().contains(a));
      count++;
    }
    ++scan;
  }
  CHECK_EQ(count, 9 * 10 * 4);
}

TEST_CASE("ScanWindow1", "[ScanWindow1]")
{
  Ravl2::Array<int, 1> val(Ravl2::IndexRange<1>(0,9));
  int at = 0;

  for (auto a: val.range()) {
    val[a] = at++;
  }

  Ravl2::IndexRange<1> windowRange(0,1);
  //SPDLOG_INFO("Window range: {} -> {} ", windowRange.min(), windowRange.max());
  CHECK_EQ(windowRange.area(), 2);
  Ravl2::ScanWindow<int, 1> scan(val, windowRange);

  CHECK_EQ(scan.scanArea().area(), 9);

  int count = 0;
  auto win = scan.window();

  CHECK_EQ(win.range().area(), 2);
  CHECK_EQ(win[0], 0);
  CHECK_EQ(win[1], 1);
  count += win.range().area();
  CHECK(count > 0);
}

TEST_CASE("AnotherIterTest", "[ArrayIter]")
{
  Ravl2::Array<int, 2> narray({4,4},1);
  Ravl2::IndexRange<2> nrng({{narray.range(0).min() + 1, narray.range(0).max()},
		      {narray.range(1).min() + 1, narray.range(1).max()}
		     }
		    );
  assert(!nrng.empty());
  auto subArray = clip(narray, nrng);
  auto cit = subArray.begin();
  CHECK(cit.valid());
  int area = nrng.area();
  for(int i = 0;i < area;i++)
  {
    CHECK(cit.valid());
    ++cit;
  }
  CHECK_FALSE(cit.valid());
}
