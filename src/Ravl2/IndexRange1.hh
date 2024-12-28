//
// Created by charles galambos on 11/08/2024.
//

#pragma once

#include "Ravl2/Index.hh"

namespace Ravl2
{
  template <unsigned N>
  class IndexRangeIterator;

  template <unsigned N>
  class IndexRange;

  //! Specialisation for 1 dimensional range.

  template <>
  class IndexRange<1>
  {
  public:
    using value_type = int;
    constexpr static unsigned dimensions = 1;

    //! Default constructor, constructs an empty range.
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

    //! @brief Create the range which creates the largest negative area.
    //! This is useful if you want guarantee the first point involved in
    //! the rectangle is always covered
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
        m_max = init.begin()[0] - 1;
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
    
    //! Get the center of the range.
    [[nodiscard]] constexpr int center() const noexcept
    {
      return (m_min + m_max) / 2;
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

    //! Returns the range shrunk by removing of the
    //! last 'n' items on the max side.
    [[nodiscard]] constexpr inline IndexRange<1> shrinkMax(int amount) const
    {
      return {m_min, m_max - amount};
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
      if(range.max() < m_min || range.min() > m_max)
        return {};
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

    //! Create a new range that has the same size size but
    //! with the min shifted at the given index.
    [[nodiscard]] constexpr IndexRange<1> shiftedMin(int ind) const
    {
      return {ind, ind + (m_max - m_min)};
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

  //! Iterator through a 1 dimensional range.

  template <>
  class IndexRangeIterator<1>
  {
  public:
    using value_type = int;
    constexpr static unsigned dimensions = 1;
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

  extern template class IndexRange<1>;
  extern template class IndexRangeIterator<1>;
}// namespace Ravl2

namespace fmt
{
  template <unsigned N>
  struct formatter<Ravl2::IndexRange<N>> : ostream_formatter {
  };
}// namespace fmt
