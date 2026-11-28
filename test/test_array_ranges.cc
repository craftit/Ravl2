//
// Regression tests for array iteration and index range handling: empty ranges,
// non-zero origins, copies into larger arrays and iterator arithmetic.
//

#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "Ravl2/Array.hh"
#include "Ravl2/ScanWindow.hh"

namespace Ravl2
{
  TEST_CASE("1-D arrays with a non-zero origin iterate their own elements")
  {
    Array<int, 1> a(IndexRange<1>(5, 9));
    for(auto i : a.range())
      a[i] = i;
    std::vector<int> seen;
    for(auto v : a)
      seen.push_back(v);
    CHECK((seen == std::vector<int> {5, 6, 7, 8, 9}));

    fill(a, 3);
    CHECK(a[5] == 3);
    CHECK(a[9] == 3);

    // Iterator offsets go the right way.
    auto it = a.begin() + 3;
    CHECK(*(it - 2) == 3);
    CHECK(it.data() - 2 == &a[6]);
  }

  TEST_CASE("Empty ranges iterate nothing")
  {
    CHECK(IndexRange<1>(6, 3).size() == 0);
    CHECK(IndexRange<1>::mostEmpty().size() == 0);
    int count = 0;
    for([[maybe_unused]] auto i : IndexRange<1>(6, 3))
      ++count;
    CHECK(count == 0);

    // Empty in the second dimension only.
    IndexRange<2> rowsNoCols({{0, 4}, {0, -1}});
    CHECK(rowsNoCols.elements() == 0);
    CHECK(rowsNoCols.area() == 0);
    count = 0;
    for([[maybe_unused]] auto i : rowsNoCols)
      ++count;
    CHECK(count == 0);

    // Both dimensions negative used to give elements() == 1.
    CHECK(IndexRange<2>({{5, 3}, {5, 3}}).elements() == 0);

    Array<int, 2> noCols(rowsNoCols);
    count = 0;
    for([[maybe_unused]] auto v : noCols)
      ++count;
    CHECK(count == 0);

    Array<int, 2> defaultArray;
    count = 0;
    for([[maybe_unused]] auto v : defaultArray)
      ++count;
    CHECK(count == 0);
  }

  TEST_CASE("copy into a larger array keeps indices")
  {
    Array<int, 2> dest({10, 10}, 0);
    Array<int, 2> src(IndexRange<2>({{2, 6}, {3, 7}}), 7);
    copy(dest, src);
    CHECK(dest[2][3] == 7);
    CHECK(dest[6][7] == 7);
    CHECK(dest[1][3] == 0);
    CHECK(dest[2][2] == 0);
    CHECK(dest[7][7] == 0);
    CHECK(dest[6][8] == 0);
  }

  TEST_CASE("Iterator distance counts elements")
  {
    Array<int, 2> a({2, 5});
    auto begin = a.begin();
    auto it = a.begin();
    for(int i = 0; i < 7; ++i)
      ++it;
    CHECK(it - begin == 7);

    Array<int, 3> b({2, 3, 4});
    auto b0 = b.begin();
    auto bt = b.begin();
    for(int i = 0; i < 17; ++i)
      ++bt;
    CHECK(bt - b0 == 17);

    IndexRange<2> rng({2, 5});
    auto r0 = rng.begin();
    auto rt = rng.begin();
    for(int i = 0; i < 7; ++i)
      ++rt;
    CHECK(rt - r0 == 7);
  }

  TEST_CASE("Iterator window owns its range")
  {
    Array<int, 2> a({4, 5});
    int n = 0;
    for(auto &v : a)
      v = n++;
    auto it = a.begin();
    for(int i = 0; i < 6; ++i)
      ++it;
    auto w = it.window();
    // The range used to point at a destroyed temporary.
    CHECK(w.range() == a.range() - it.index());
    CHECK(w[{0, 0}] == *it);
    CHECK(w[{1, 1}] == a[2][2]);
  }

  TEST_CASE("ArrayAccess call operator returns a reference")
  {
    Array<int, 2> a({3, 3}, 0);
    ArrayAccess<int, 2> acc(a);
    acc(1, 1) = 7;
    CHECK(a[1][1] == 7);
  }

  TEST_CASE("ScanWindow 1-D index follows the window position")
  {
    Array<int, 1> a(IndexRange<1>(0, 9));
    ScanWindow<int, 1> scan(a, IndexRange<1>(0, 2));
    int expected = 0;
    for(; scan.valid(); ++scan) {
      CHECK(scan.index()[0] == expected);
      ++expected;
    }
    CHECK(expected == 8);
  }
}// namespace Ravl2
