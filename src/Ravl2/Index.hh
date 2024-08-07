/*
 * Range.hh
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/archives/json.hpp>

#include "Ravl2/Sentinel.hh"

namespace Ravl2
{
  //! N-dimensional index.

  template <unsigned N>
  class Index
  {
  public:
    //! array of indexs
    constexpr Index(std::initializer_list<int> val)
    {
      if(val.size() == 1) {
        for(unsigned i = 0; i < N; i++)
          m_index[i] = val.begin()[0];
        return;
      }
      assert(val.size() == N);
      for(unsigned i = 0; i < N; i++)
        m_index[i] = val.begin()[i];
    }

    //! Unpack using a template
    template <typename... Args>
    constexpr explicit Index(Args... args)
    {
      static_assert(sizeof...(args) == N, "Incorrect number of arguments");
      std::array<int, N> vals {args...};
      for(unsigned i = 0; i < N; i++)
        m_index[i] = vals[i];
    }

    //! Default constructor
    constexpr Index() = default;

    //! Access location in the i th dimension.
    [[nodiscard]] constexpr int index(unsigned i) const noexcept
    {
      assert(i < N);
      return m_index[i];
    }

    //! Access index for dimension n.
    [[nodiscard]] constexpr int operator[](unsigned n) const
    {
      assert(n < N);
      return m_index[n];
    }

    //! Access index for dimension n.
    [[nodiscard]] constexpr int &operator[](unsigned n)
    {
      assert(n < N);
      return m_index[n];
    }

    //! Add index in place
    constexpr Index<N> &operator+=(const Index<N> &ind) noexcept
    {
      for(unsigned i = 0; i < N; i++)
        m_index[i] += ind[i];
      return *this;
    }

    //! Subtract index in place
    constexpr Index<N> &operator-=(const Index<N> &ind) noexcept
    {
      for(unsigned i = 0; i < N; i++)
        m_index[i] -= ind[i];
      return *this;
    }

    //! Add two index's together.
    [[nodiscard]] constexpr Index<N> operator+(const Index<N> &ind) const
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_index[i] + ind[i];
      return ret;
    }

    //! Subtract an index from this one.
    [[nodiscard]] constexpr Index<N> operator-(const Index<N> &ind) const
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_index[i] - ind[i];
      return ret;
    }

    //! Equality test.
    [[nodiscard]] constexpr bool operator==(const Index<N> &ind) const
    {
      for(unsigned i = 0; i < N; i++)
        if(m_index[i] != ind[i])
          return false;
      return true;
    }

    //! Equality test.
    [[nodiscard]] constexpr bool operator!=(const Index<N> &ind) const
    {
      return !operator==(ind);
    }

    //! begin
    [[nodiscard]] constexpr int *begin()
    {
      return m_index.data();
    }

    //! end
    [[nodiscard]] constexpr int *end()
    {
      return &(m_index.data()[N]);
    }

    //! begin
    [[nodiscard]] constexpr const int *begin() const
    {
      return m_index.data();
    }

    //! end
    [[nodiscard]] constexpr const int *end() const
    {
      return &(m_index.data()[N]);
    }

    //! Access as const ptr.
    [[nodiscard]] constexpr const int *data() const
    {
      return m_index.data();
    }

    //! Access as ptr.
    [[nodiscard]] constexpr int *data()
    {
      return m_index.data();
    }

    //! Access size
    [[nodiscard]] constexpr size_t size() const
    {
      return size_t(N);
    }

  protected:
    std::array<int, N> m_index = {0};
  };

  template <unsigned N>
  std::ostream &operator<<(std::ostream &strm, const Index<N> &ind)
  {
    strm << ind[0];
    for(unsigned i = 1; i < N; i++)
      strm << " " << ind[i];
    return strm;
  }

  //! Convert a parameter list of RealT to a point
  template <typename... DataT, unsigned N = sizeof...(DataT)>
  constexpr inline Index<N> toIndex(DataT... data)
  {
    return Index<N>({int(data)...});
  }

  template <unsigned N>
  class IndexRangeIterator;
  template <unsigned N>
  class IndexRange;

  //! Specialisation for 1 dimensional range.

  template <>
  class IndexRange<1>
  {
  public:
    //! Default constructor
    constexpr IndexRange() = default;

    //! Constructor from a min and max value, inclusive.
    constexpr IndexRange(int min, int max)
        : m_min(min),
          m_max(max)
    {}

    //! Construct from a single value.
    constexpr explicit IndexRange(int at)
        : m_min(at),
          m_max(at)
    {}

    //! Create a range from a size.
    //! The range will be from 0 to size-1.
    static constexpr IndexRange<1> fromSize(int size)
    {
      return {0, size - 1};
    }

    //! Create the range which creates the most negative area.
    // This is useful if you want guarantee the first point involved in
    // the rectangle is always covered
    static constexpr auto mostEmpty()
    {
      return IndexRange<1>(std::numeric_limits<int>::max(), std::numeric_limits<int>::min());
    }

    //! Default constructor
    constexpr IndexRange(std::initializer_list<int> init)
    {
      assert(init.size() > 0);
      if(init.size() == 1) {
        m_min = 0;
        m_max = init.begin()[0] + 1;
      }
      if(init.size() > 1) {
        m_min = init.begin()[0];
        m_max = init.begin()[1];
      }
    }

    //! Make size of range 0.
    constexpr void clear() noexcept
    {
      m_max = m_min - 1;
    }

    //! Test if range is empty
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      return m_min > m_max;
    }

    //! Get the size of the range in each dimension.
    [[nodiscard]] constexpr int size() const noexcept
    {
      return (m_max - m_min) + 1;
    }

    //! Get the area of the range.
    [[nodiscard]] constexpr int area() const noexcept
    {
      return size();
    }

    //! Get size of given dimension
    [[nodiscard]] constexpr int size([[maybe_unused]] unsigned n) const noexcept
    {
      assert(n == 0);
      return size();
    }

    //! Get number of elements covered by range.
    // This only works for positive range sizes.
    [[nodiscard]] constexpr size_t elements() const
    {
      const auto theSize = size();
      if(theSize < 0) return 0;
      return static_cast<size_t>(theSize);
    }

    //! Test if an index is contained within the range.
    [[nodiscard]] constexpr bool contains(int ind) const noexcept
    {
      return (ind >= m_min && ind <= m_max);
    }

    //! Test if an index is contained within the range.
    [[nodiscard]] constexpr bool contains(Index<1> ind) const noexcept
    {
      return (ind[0] >= m_min && ind[0] <= m_max);
    }

    //! Test if 'range' is contained with this one
    [[nodiscard]] constexpr bool contains(const IndexRange<1> &range) const noexcept
    {
      return (contains(range.m_min) && contains(range.m_max));
    }

    //! Test if 'range' overlaps with this one
    [[nodiscard]] constexpr bool overlaps(const IndexRange<1> &range) const noexcept
    {
      return (range.m_min <= m_max && range.m_max >= m_min);
    }

    //! Shrink the range in from both ends by amount.
    [[nodiscard]] constexpr IndexRange<1> shrink(int amount) const
    {
      return {m_min + amount, m_max - amount};
    }

    //! Shrink the range by given size,
    //! min is increased by min of amount.min(), and max decreased by amount.max()
    [[nodiscard]] constexpr IndexRange<1> shrink(const IndexRange<1> &amount) const
    {
      return {m_min - amount.min(), m_max - amount.max()};
    }

    //! Shrink the range in from both ends by amount.
    [[nodiscard]] constexpr IndexRange<1> expand(int amount) const
    {
      return {m_min - amount, m_max + amount};
    }

    //! Shift range by given values.
    constexpr IndexRange<1> &operator+=(int ind)
    {
      m_min += ind;
      m_max += ind;
      return *this;
    }

    //! Shift this range in dimension n by the given 'amount'
    constexpr void shift([[maybe_unused]] unsigned n, int amount)
    {
      assert(n == 0);
      m_min += amount;
      m_max += amount;
    }

    [[nodiscard]] constexpr auto &range([[maybe_unused]] unsigned i)
    {
      assert(i == 0);
      return *this;
    }

    [[nodiscard]] constexpr const auto &range([[maybe_unused]] unsigned i) const
    {
      assert(i == 0);
      return *this;
    }

    //! Smallest index in all dimensions.
    [[nodiscard]] constexpr int min() const
    {
      return m_min;
    }

    //! largest index in all dimensions.
    [[nodiscard]] constexpr int max() const
    {
      return m_max;
    }

    //! Smallest index in all dimensions.
    [[nodiscard]] constexpr int &min()
    {
      return m_min;
    }

    //! largest index in all dimensions.
    [[nodiscard]] constexpr int &max()
    {
      return m_max;
    }

    [[nodiscard]] constexpr int &min([[maybe_unused]] unsigned i)
    {
      assert(i == 0);
      return min();
    }

    [[nodiscard]] constexpr int min([[maybe_unused]] unsigned i) const
    {
      assert(i == 0);
      return min();
    }

    [[nodiscard]] constexpr int &max([[maybe_unused]] unsigned i)
    {
      assert(i == 0);
      return max();
    }

    [[nodiscard]] constexpr int max([[maybe_unused]] unsigned i) const
    {
      assert(i == 0);
      return max();
    }

    //! Create a new index clipped so it is within the range.
    [[nodiscard]] constexpr int clip(int index) const
    {
      if(index < m_min)
        return m_min;
      if(index > m_max)
        return m_max;
      return index;
    }

    //! Clip index so it is within the range.
    [[nodiscard]] constexpr IndexRange<1> clip(const IndexRange<1> &range) const
    {
      return {clip(range.min()), clip(range.max())};
    }

    //! Clip range in place
    //! Returns true if the range is still valid.
    constexpr bool clipBy(const IndexRange<1> &range)
    {
      m_min = std::max(m_min, range.min());
      m_max = std::min(m_max, range.max());
      return m_min <= m_max;
    }

    //! Add offset to range
    [[nodiscard]] constexpr IndexRange<1> operator+(int ind) const
    {
      return {m_min + ind, m_max + ind};
    }

    //! Add offset to range
    [[nodiscard]] constexpr IndexRange<1> operator-(int ind) const
    {
      return {m_min - ind, m_max - ind};
    }

    //! Add ranges
    [[nodiscard]] constexpr IndexRange<1> operator+(IndexRange<1> ind) const
    {
      return {m_min + ind.m_min, m_max + ind.m_max};
    }

    //! Subtract ranges
    [[nodiscard]] constexpr IndexRange<1> operator-(IndexRange<1> ind) const
    {
      return {m_min - ind.m_min, m_max - ind.m_max};
    }

    //! Are two ranges equal.
    constexpr bool operator==(const IndexRange<1> &ind) const noexcept
    {
      return m_min == ind.m_min && m_max == ind.m_max;
    }

    //! Are two ranges not equal.
    constexpr bool operator!=(const IndexRange<1> &ind) const noexcept
    {
      return m_min != ind.m_min || m_max != ind.m_max;
    }

    //! Access range of given dimension.
    [[nodiscard]] constexpr const IndexRange<1> &operator[]([[maybe_unused]] unsigned i) const
    {
      assert(i == 0);
      return *this;
    }

    //! Access range of given dimension.
    [[nodiscard]] constexpr IndexRange<1> &operator[]([[maybe_unused]] unsigned i)
    {
      assert(i == 0);
      return *this;
    }

    //! Modify range to include given index.
    //! Returns true if ind is within the range.
    inline constexpr bool involve(int ind)
    {
      bool inside = true;
      if(ind > m_max) {
        m_max = ind;
        inside = false;
      }
      if(ind < m_min) {
        m_min = ind;
        inside = false;
      }
      return inside;
    }

    //! Modify range to include given index.
    //! Returns true if rng is within this range.
    inline constexpr bool involve(const IndexRange<1> &rng)
    {
      bool inside = true;
      if(rng.m_max > m_max) {
        m_max = rng.m_max;
        inside = false;
      }
      if(rng.m_min < m_min) {
        m_min = rng.m_min;
        inside = false;
      }
      return inside;
    }

    //! Start of the range.
    [[nodiscard]] constexpr IndexRangeIterator<1> begin() const;

    //! One passed the end of the range.
    [[nodiscard]] constexpr IndexRangeIterator<1> end() const;

    //! Access as ptr to array of 1d ranges
    [[nodiscard]] constexpr IndexRange<1> *range_data()
    {
      return this;
    }

    //! Access as ptr to array of 1d ranges
    [[nodiscard]] constexpr const IndexRange<1> *range_data() const
    {
      return this;
    }

  protected:
    int m_min {0};
    int m_max {-1};
  };

  //! N-dimensional range of index's
  // The range is inclusive of all values.

  template <unsigned N>
  class IndexRange
  {
  public:
    //! Default constructor
    constexpr IndexRange() noexcept = default;

    //! Build a new range given two parts
    constexpr IndexRange(const IndexRange<N - 1> &start, const IndexRange<1> &extra) noexcept
    {
      for(unsigned i = 0; i < N - 1; i++)
        m_range[i] = start[i];
      m_range[N - 1] = extra;
    }

    //! Build a new range given two parts
    constexpr IndexRange(const IndexRange<N - 1> &start, int extraSize) noexcept
    {
      for(unsigned i = 0; i < N - 1; i++)
        m_range[i] = start[i];
      m_range[N - 1] = IndexRange<1>(extraSize);
    }

    //! Default constructor
    constexpr IndexRange(const Index<N> &min, const Index<N> &max) noexcept
    {
      for(unsigned i = 0; i < N; i++)
        m_range[i] = IndexRange<1>(min[i], max[i]);
    }

    //! Construct from sizes for each dimension.
    //! The ranges will have a zero origin.
    constexpr IndexRange(std::initializer_list<size_t> sizes) noexcept
    {
      assert(sizes.size() == N);
      for(unsigned i = 0; i < N; i++)
        m_range[i] = IndexRange<1>(0, int(sizes.begin()[i]) - 1);
    }

    //! Construct from ranges for each dimension.

    constexpr IndexRange(std::initializer_list<std::initializer_list<int>> init) noexcept
    {
      assert(init.size() == N);
      for(unsigned i = 0; i < N; i++) {
        m_range[i] = IndexRange<1>(init.begin()[i]);
      }
    }

    constexpr IndexRange(std::initializer_list<IndexRange<1>> ranges) noexcept
    {
      assert(ranges.size() == N);
      for(unsigned i = 0; i < N; i++) {
        m_range[i] = ranges.begin()[i];
      }
    }

    //! Create a range from a sizes.
    //! The range[i] will be from 0 to size[i]-1.
    template <typename... IndexT>
    [[nodiscard]] static IndexRange<N> fromSize(IndexT... sizes)
    {
      return IndexRange<N>({size_t(sizes)...});
    }

    //! Create the range which creates the most negative area.
    // This is useful if you want guarantee the first point involved in
    // the rectangle is always covered
    static constexpr IndexRange<N> mostEmpty()
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = IndexRange<1>::mostEmpty();
      return ret;
    }

    //! Make size of range 0.
    constexpr void clear() noexcept
    {
      m_range[0].clear();
    }

    //! Get the size of the range in each dimension.
    [[nodiscard]] constexpr Index<N> size() const noexcept
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].size();
      return ret;
    }

    //! Get size of given dimension
    [[nodiscard]] constexpr int size(unsigned n) const noexcept
    {
      return m_range[n].size();
    }

    //! Get the area of the range.
    [[nodiscard]] constexpr int area() const noexcept
    {
      int a = 1;
      for(unsigned i = 0; i < N; i++)
        a *= m_range[i].size();
      return a;
    }

    //! Get total number of elements that should be allocated
    //! for an array.  This will always be positive or zero.
    [[nodiscard]] constexpr size_t elements() const
    {
      int n = 1;
      for(unsigned i = 0; i < N; i++)
        n *= m_range[i].size();
      if(n < 0)
        n = 0;
      return size_t(n);
    }

    //! Is range empty ?
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      for(unsigned i = 0; i < N; i++)
        if(m_range[i].empty())
          return true;
      return false;
    }

    //! Test if an index is contained within the range.
    [[nodiscard]] constexpr bool contains(const Index<N> &ind) const noexcept
    {
      for(unsigned i = 0; i < N; i++)
        if(!m_range[i].contains(ind[i]))
          return false;
      return true;
    }

    //! Test if 'range' is contained with this one
    [[nodiscard]] constexpr bool contains(const IndexRange<N> &range) const noexcept
    {
      for(unsigned i = 0; i < N; i++)
        if(!m_range[i].contains(range.m_range[i]))
          return false;
      return true;
    }

    //! Test if 'range' overlaps with this one
    [[nodiscard]] constexpr bool overlaps(const IndexRange<N> &range) const noexcept
    {
      for(unsigned i = 0; i < N; i++)
        if(!m_range[i].overlaps(range.m_range[i]))
          return false;
      return true;
    }

    //! Shrink the range in from both ends by amount.
    [[nodiscard]] constexpr IndexRange<N> shrink(int amount) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].shrink(amount);
      return ret;
    }

    //! Shrink the range in from both ends by amount.
    [[nodiscard]] constexpr IndexRange<N> shrink(const Index<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].shrink(ind[i]);
      return ret;
    }

    //! Shrink the range by given size,
    //! min is increased by min of amount.min(), and max decreased by amount.max()
    [[nodiscard]] constexpr IndexRange<N> shrink(const IndexRange<N> &amount) const
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].shrink(amount[i]);
      return ret;
    }

    //! Shrink the range in from both ends by amount.
    [[nodiscard]] constexpr IndexRange<N> expand(int amount) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].expand(amount);
      return ret;
    }

    //! Shrink the range in from both ends by amount.
    [[nodiscard]] constexpr IndexRange<N> expand(const Index<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].expand(ind[i]);
      return ret;
    }

    //! Shift range by given values.
    constexpr IndexRange<N> &operator+=(const Index<N> &ind) noexcept
    {
      for(unsigned i = 0; i < N; i++)
        m_range[i] += ind[i];
      return *this;
    }

    //! Shift range by given values.
    constexpr IndexRange<N> operator+(const Index<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret.m_range[i] = m_range[i] + ind[i];
      return ret;
    }

    //! Shift range by given values.
    constexpr IndexRange<N> operator-(const Index<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret.m_range[i] = m_range[i] - ind[i];
      return ret;
    }

    //! Add one range to another
    [[nodiscard]] constexpr IndexRange<N> operator+(const IndexRange<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret.m_range[i] = m_range[i] + ind[i];
      return ret;
    }

    //! Add one range to another
    [[nodiscard]] constexpr IndexRange<N> operator-(const IndexRange<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret.m_range[i] = m_range[i] - ind[i];
      return ret;
    }

    //! Are two ranges equal.
    [[nodiscard]] constexpr bool operator==(const IndexRange<N> &ind) const noexcept
    {
      for(unsigned i = 0; i < N; i++) {
        if(m_range[i] != ind.m_range[i])
          return false;
      }
      return true;
    }

    //! Are two ranges not equal.
    [[nodiscard]]
    constexpr bool operator!=(const IndexRange<N> &ind) const noexcept
    {
      for(unsigned i = 0; i < N; i++) {
        if(m_range[i] != ind.m_range[i])
          return true;
      }
      return false;
    }

    //! Shift this range in dimension n by the given 'amount'
    constexpr void shift(unsigned n, int amount)
    {
      m_range[n] += amount;
    }

    //! Smallest index in all dimensions.
    [[nodiscard]] constexpr Index<N> min() const
    {
      Index<N> min;
      for(unsigned i = 0; i < N; i++)
        min[i] = m_range[i].min();
      return min;
    }

    //! largest index in all dimensions.
    [[nodiscard]] constexpr Index<N> max() const
    {
      Index<N> max;
      for(unsigned i = 0; i < N; i++)
        max[i] = m_range[i].max();
      return max;
    }

    //! Create a new index clipped so it is within the range.
    [[nodiscard]] constexpr Index<N> clip(const Index<N> &index) const
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].clip(index[i]);
      return ret;
    }

    //! Clip range in place
    //! Returns true if the range is still valid.
    constexpr bool clipBy(const IndexRange<N> &range)
    {
      bool valid = true;
      for(unsigned i = 0; i < N; i++)
        valid &= m_range[i].clipBy(range.range(i));
      return valid;
    }

    //! Access range of given dimension.
    [[nodiscard]] constexpr const IndexRange<1> &operator[](unsigned i) const
    {
      assert(i < N);
      return m_range[i];
    }

    //! Access range of given dimension.
    [[nodiscard]] constexpr IndexRange<1> &operator[](unsigned i)
    {
      assert(i < N);
      return m_range[i];
    }

    //! Clip index so it is within the range.
    [[nodiscard]] constexpr IndexRange<N> clip(const IndexRange<N> &range) const
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].clip(range[i]);
      return ret;
    }

    //! Involve an index in the range.
    //! Returns true if ind is within the range.
    constexpr bool involve(const Index<N> &ind)
    {
      bool ret = true;
      for(unsigned i = 0; i < N; i++) {
        if(!m_range[i].involve(ind[i]))
          ret = false;
      }
      return ret;
    }

    //! Involve another index range in this range.
    //! Returns true if ind is within the range.
    constexpr bool involve(const IndexRange<N> &rng)
    {
      bool ret = true;
      for(unsigned i = 0; i < N; i++) {
        if(!m_range[i].involve(rng[i]))
          ret = false;
      }
      return ret;
    }

    //! Start of the range.
    [[nodiscard]] constexpr IndexRangeIterator<N> begin() const;

    //! One passed the end of the range.
    [[nodiscard]] constexpr Sentinel end() const
    {
      return {};
    }

    //! Access as an array of ranges
    [[nodiscard]] constexpr std::array<IndexRange<1>, N> &ranges()
    {
      return m_range;
    }

    //! Access as an array of ranges
    [[nodiscard]] const std::array<IndexRange<1>, N> &ranges() const
    {
      return m_range;
    }

    [[nodiscard]] constexpr IndexRange<1> &range(unsigned i)
    {
      assert(i < N);
      return m_range[i];
    }

    [[nodiscard]] constexpr const IndexRange<1> &range(unsigned i) const
    {
      assert(i < N);
      return m_range[i];
    }

    [[nodiscard]] constexpr int &min(unsigned i)
    {
      assert(i < N);
      return m_range[i].min();
    }

    [[nodiscard]] constexpr int min(unsigned i) const
    {
      assert(i < N);
      return m_range[i].min();
    }

    [[nodiscard]] constexpr int &max(unsigned i)
    {
      assert(i < N);
      return m_range[i].max();
    }

    [[nodiscard]] constexpr int max(unsigned i) const
    {
      assert(i < N);
      return m_range[i].max();
    }

    [[nodiscard]] constexpr IndexRange<1> *range_data()
    {
      return m_range.data();
    }

    [[nodiscard]] constexpr const IndexRange<1> *range_data() const
    {
      return m_range.data();
    }

  protected:
    std::array<IndexRange<1>, N> m_range;
  };

  template <unsigned N>
  std::ostream &operator<<(std::ostream &strm, const IndexRange<N> &rng)
  {
    for(unsigned i = 0; i < N; i++) {
      strm << "(" << rng[i].min() << "," << rng[i].max() << ")";
    }
    return strm;
  }

  //! Iterator through a 1 dimensional range.

  template <>
  class IndexRangeIterator<1>
  {
  public:
    using value_type = int;
    using difference_type = int;

    constexpr IndexRangeIterator() = default;

    //! Constructor iterator
    explicit constexpr IndexRangeIterator(int at)
        : m_at(at)
    {}

    //! Access index.
    constexpr int operator*() const
    {
      return m_at;
    }

    //! Increment
    constexpr IndexRangeIterator<1> operator++(int)
    {
      auto ret = *this;
      ++m_at;
      return ret;
    }

    //! Increment
    constexpr IndexRangeIterator<1> &operator++()
    {
      ++m_at;
      return *this;
    }

    //! Decrement
    constexpr IndexRangeIterator<1> &operator--()
    {
      m_at--;
      return *this;
    }

    //! Decrement
    constexpr IndexRangeIterator<1> operator--(int)
    {
      auto ret = *this;
      m_at--;
      return ret;
    }

    [[nodiscard]] constexpr bool operator!=(const IndexRangeIterator<1> &other) const
    {
      return other.m_at != m_at;
    }

    [[nodiscard]] constexpr bool operator==(const IndexRangeIterator<1> &other) const
    {
      return other.m_at == m_at;
    }

  protected:
    int m_at {};
  };

  //! Start of the range.
  inline constexpr IndexRangeIterator<1> IndexRange<1>::begin() const
  {
    return IndexRangeIterator<1> {m_min};
  }

  //! One passed the end of the range.
  inline constexpr IndexRangeIterator<1> IndexRange<1>::end() const
  {
    return IndexRangeIterator<1> {m_max + 1};
  }

  //! Iterate through an N dimensional range.

  template <unsigned N>
  class IndexRangeIterator
  {
  public:
    using value_type = Index<N>;
    using difference_type = int;

    //! Default constructor, creates an invalid iterator.
    constexpr IndexRangeIterator() = default;

    //! Constructor iterator from a range and a position.
    //! The position must be within the range.
    // cppcheck-suppress functionStatic
    constexpr IndexRangeIterator(const IndexRange<N> &range, const Index<N> &at)
        : m_range(&range),
          m_at(at)
    {
      assert(range.contains(at));
    }

    //! Constructor iterator from a range and a position.
    // cppcheck-suppress functionStatic
    constexpr IndexRangeIterator(const IndexRange<N> &range, const Index<N> &at, [[maybe_unused]] bool noRangeCheck)
        : m_range(&range),
          m_at(at)
    {
      assert(noRangeCheck || range.contains(at));
    }

    //! Access current index.
    [[nodiscard]] constexpr const Index<N> &operator*() const
    {
      return m_at;
    }

    //! Increment position.
    constexpr IndexRangeIterator<N> &operator++()
    {
      for(unsigned i = N - 1; i > 0; --i) {
        ++m_at[i];
        if(m_at[i] <= m_range->max()[i])
          return *this;
        m_at[i] = m_range->min()[i];
      }
      ++m_at[0];
      return *this;
    }

    //! Increment position.
    constexpr IndexRangeIterator<N> operator++(int)
    {
      IndexRangeIterator<N> ret = *this;
      operator++();
      return ret;
    }

    //! Are we at the end of the range?
    //! In the case of this iterator we have all we need to know internally.
    [[nodiscard]] constexpr bool done() const
    {
      return m_at[0] > m_range->max()[0];
    }

    //! Are we at the end of the range?
    [[nodiscard]] constexpr bool valid() const
    {
      return m_at[0] <= m_range->max()[0];
    }

    //! Equality test.
    [[nodiscard]] constexpr bool operator==(const IndexRangeIterator<N> &other) const
    {
      return m_at == other.m_at;
    }

    //! Equality test.
    [[nodiscard]] constexpr bool operator!=(const IndexRangeIterator<N> &other) const
    {
      return m_at != other.m_at;
    }

    //! Compute the distance between two iterators in terms of number of increments.
    [[nodiscard]] constexpr difference_type operator-(const IndexRangeIterator<N> &other) const
    {
      assert(m_range == other.m_range);
      assert(m_range != nullptr);
      difference_type diff = (m_at[N - 1] - other.m_at[N - 1]);
      for(unsigned i = 0; i < N - 1; i++)
        diff += (m_at[i] - other.m_at[i]) * m_range->size(i);
      return diff;
    }

  protected:
    const IndexRange<N> *m_range = nullptr;
    Index<N> m_at {};
  };

  //! Start of the range.
  template <unsigned N>
  inline constexpr IndexRangeIterator<N> IndexRange<N>::begin() const
  {
    return IndexRangeIterator<N>(*this, min());
  }

  //! Serialization support
  template <class Archive, unsigned N>
  constexpr void serialize(Archive &archive, Index<N> &ind)
  {
    cereal::size_type s = N;
    archive(cereal::make_size_tag(s));
    if(s != N) {
      throw std::runtime_error("array has incorrect length");
    }
    for(auto &i : ind) {
      archive(i);
    }
  }

  //! Serialization support
  template <class Archive>
  constexpr void serialize(Archive &archive, IndexRange<1> &range)
  {
    cereal::size_type s = 2;
    archive(cereal::make_size_tag(s));
    if(s != 2) {
      throw std::runtime_error("index range has incorrect length");
    }
    archive(range.min(), range.max());
  }

  //! Serialization support
  template <class Archive, unsigned N>
    requires(N > 1)
  constexpr void serialize(Archive &archive, IndexRange<N> &range)
  {
    cereal::size_type s = N;
    archive(cereal::make_size_tag(s));
    if(s != N) {
      throw std::runtime_error("index range has incorrect length");
    }
    for(auto &r : range.ranges()) {
      archive(r);
    }
  }

  // Let everyone know there's an implementation already generated for common cases
  extern template class Index<1>;
  extern template class Index<2>;
  extern template class Index<3>;
  extern template class IndexRange<1>;
  extern template class IndexRange<2>;
  extern template class IndexRange<3>;
  extern template class IndexRangeIterator<1>;
  extern template class IndexRangeIterator<2>;
  extern template class IndexRangeIterator<3>;
}// namespace Ravl2

namespace fmt
{
  template <unsigned N>
  struct formatter<Ravl2::Index<N>> : ostream_formatter {
  };
  template <unsigned N>
  struct formatter<Ravl2::IndexRange<N>> : ostream_formatter {
  };
}// namespace fmt
