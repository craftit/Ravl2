//
// Created by charles galambos on 11/08/2024.
//

#pragma once

#include "Ravl2/IndexRange1.hh"

namespace Ravl2
{

  //! N-dimensional range of index's
  //! The range is inclusive of all values.

  template <unsigned N>
  class IndexRange
  {
  public:
    using value_type = Index<N>;
    constexpr static unsigned dimensions = N;

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
    
    //! Get the center of the range.
    [[nodiscard]] constexpr auto center() const noexcept
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].center();
      return ret;
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

    //! Returns the range shrunk by removing of the
    //! last 'n' items on both limits of this range in each dimension.
    [[nodiscard]] constexpr IndexRange<N> shrinkMax(int amount) const
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].shrinkMax(amount);
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

    //! Create a new range that has the same size size but
    //! with the min shifted at the given index.
    [[nodiscard]] constexpr IndexRange<N> shiftedMin(const Index<N> &newMinIndex) const
    {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_range[i].shiftedMin(newMinIndex[i]);
      return ret;
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

    //! Clip 'range' so it is within the this range.
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
  inline std::ostream &operator<<(std::ostream &strm, const IndexRange<N> &rng)
  {
    for(unsigned i = 0; i < N; i++) {
      strm  << rng[i].min() << " " << rng[i].max() << " ";
    }
    return strm;
  }
  
  template <unsigned N>
  inline std::istream &operator>>(std::istream &strm, IndexRange<N> &rng)
  {
    for(unsigned i = 0; i < N; i++) {
      strm >> rng[i].min() >> rng[i].max();
    }
    return strm;
  }
  
  //! Iterate through an N dimensional range.

  template <unsigned N>
  class IndexRangeIterator
  {
  public:
    using value_type = Index<N>;
    constexpr static unsigned dimensions = N;
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

    //! Access current index.
    [[nodiscard]] constexpr const Index<N> &index() const
    {
      return m_at;
    }

    //! @brief Goto next row of the range.
    //! Return true if we're still in the range.
    constexpr bool nextRow()
    {
      m_at[N - 1] = m_range->min()[N - 1];
      for(unsigned i = N - 2; i > 0; --i) {
        ++m_at[i];
        if(m_at[i] <= m_range->max()[i])
          return true;
        m_at[i] = m_range->min()[i];
      }
      ++m_at[0];
      return m_at[0] <= m_range->max()[0];
    }

    //! Goto next element, returns true if we're on the same row, false if we've changed rows.
    [[nodiscard]]
    constexpr inline bool next()
    {
      ++m_at[N - 1];
      if(m_at[N - 1] <= m_range->max()[N - 1])
        return true;
      nextRow();
      return false;
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
  
  //! Clip a range by another range.
  template <unsigned N>
  [[nodiscard]] constexpr IndexRange<N> clip(const IndexRange<N> &range, const IndexRange<N> &clipRange)
  {
    if constexpr(N == 1) {
      return range.clip(clipRange);
    } else {
      IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = range[i].clip(clipRange[i]);
      return ret;
    }
  }

  extern template class IndexRange<2>;
  extern template class IndexRange<3>;
  extern template class IndexRangeIterator<2>;
  extern template class IndexRangeIterator<3>;

}// namespace Ravl2