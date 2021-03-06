/*
 * Range.hh
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#ifndef RAVL2_INDEX_HH_
#define RAVL2_INDEX_HH_

#include <assert.h>
#include <vector>
#include <algorithm>
#include <iostream>

namespace Ravl2
{
  //! N-dimensional index.

  template<unsigned N>
  class Index
  {
  public:
    //! array of indexs
    Index(std::initializer_list<int> val)
    {
      if(val.size() == 1) {
        for(int i = 0;i < N;i++)
          m_index[i] = val.begin()[0];
        return;
      }
      assert(val.size() == N);
      for(int i = 0;i < N;i++)
        m_index[i] = val.begin()[i];
    }

    //! Default constructor
    Index()
    {}

    //! Access location in the i th dimension.
    int index(unsigned i) const noexcept
    {
      assert(i < N);
      return m_index[i];
    }

    //! Access index for dimension n.
    int operator[](unsigned n) const
    {
      assert(n < N);
      return m_index[n];
    }

    //! Access index for dimension n.
    int &operator[](unsigned n)
    {
      assert(n < N);
      return m_index[n];
    }

    //! Add index in place
    Index<N> &operator+=(const Index<N> &ind) noexcept
    {
      for(int i = 0;i < N;i++)
        m_index[i] += ind[i];
      return *this;
    }

    //! Subtract index in place
    Index<N> &operator-=(const Index<N> &ind) noexcept
    {
      for(int i = 0;i < N;i++)
        m_index[i] -= ind[i];
      return *this;
    }

    //! Add two index's together.
    Index<N> operator+(const Index<N> &ind) const
    {
      Index<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_index[i] + ind[i];
      return ret;
    }

    //! Subtract an index from this one.
    Index<N> operator-(const Index<N> &ind) const
    {
      Index<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_index[i] - ind[i];
      return ret;
    }

    //! Equality test.
    bool operator==(const Index<N> &ind) const
    {
      for(unsigned i = 0;i < N;i++)
        if(m_index[i] != ind[i])
          return false;
      return true;
    }

    //! Equality test.
    bool operator!=(const Index<N> &ind) const
    { return !operator==(ind); }

    //! begin
    int *begin()
    { return m_index; }

    //! begin
    const int *end() const
    { return &m_index + N; }

    //! begin
    const int *begin() const
    { return m_index; }

  protected:
    int m_index[N] = {0};
  };

  template<unsigned N>
  std::ostream &operator<<(std::ostream &strm,const Index<N> &ind)
  {
    for(unsigned i = 0;i < N;i++)
      strm << ind[i] << " ";
    return strm;
  }

  template<unsigned N> class IndexRangeIterator;
  template<unsigned N> class IndexRange;


  //! Specialisation for 1 dimensional range.

  template<>
  class IndexRange<1>
  {
  public:
    //! Default constructor
    IndexRange()
       : m_min(0), // gcc-4.7.2 doesn't appear to initialise these correctly.
         m_max(-1)
    {}

    //! Default constructor
    IndexRange(int min,int max)
     : m_min(min),
       m_max(max)
    {}

    //! Default constructor
    IndexRange(std::initializer_list<int> init)
    {
      assert(init.size() > 0);
      if(init.size() == 1) {
        m_min = 0;
        m_max = init.begin()[0]+1;
      }
      if(init.size() > 1) {
        m_min = init.begin()[0];
        m_max = init.begin()[1];
      }
    }

    //! Default constructor
    IndexRange(int size)
     : m_min(0),
       m_max(size-1)
    {}

    //! Make size of range 0.
    void clear() noexcept
    { m_max = m_min-1; }

    //! Test if range is empty
    bool empty() const noexcept
    { return m_min > m_max; }

    //! Get the size of the range in each dimension.
    int size() const noexcept
    { return (m_max - m_min) + 1; }

    //! Get size of given dimension
    int size(unsigned n) const noexcept
    {
      assert(n == 0);
      return size();
    }

    //! Get number of elements covered by range.
    // This only works for positive range sizes.
    size_t elements() const
    { return std::max(static_cast<size_t>(size()),static_cast<size_t>(0)); }

    //! Test if an index is contained within the range.
    bool contains(int ind) const noexcept
    { return (ind >= m_min && ind <= m_max); }

    //! Test if an index is contained within the range.
    bool contains(Index<1> ind) const noexcept
    { return (ind[0] >= m_min && ind[0] <= m_max); }

    //! Test if 'range' is contained with this one
    bool contains(const IndexRange<1> &range) const noexcept
    { return (contains(range.m_min) && contains(range.m_max)); }

    //! Shrink the range in from both ends by amount.
    IndexRange<1> shrink(int amount) const
    { return IndexRange<1>(m_min + amount,m_max - amount); }

    //! Shrink the range by given size,
    //! min is increased by min of amount.min(), and max decreased by amount.max()
    IndexRange<1> shrink(const IndexRange<1> &amount) const
    { return IndexRange<1>(m_min + amount.min(),m_max - amount.max()); }

    //! Shrink the range in from both ends by amount.
    IndexRange<1> expand(int amount) const
    { return IndexRange<1>(m_min - amount,m_max + amount); }

    //! Shift range by given values.
    IndexRange<1> &operator+=(int ind)
    {
      m_min += ind;
      m_max += ind;
      return *this;
    }

    //! Shift this range in dimension n by the given 'amount'
    void shift(unsigned n,int amount)
    {
      assert(n == 0);
      m_min += amount;
      m_max += amount;
    }

    //! Smallest index in all dimensions.
    const int &min() const
    { return m_min; }

    //! largest index in all dimensions.
    const int &max() const
    { return m_max; }

    //! Create a new index clipped so it is within the range.
    int clip(int index) const
    {
      if(index < m_min)
        return m_min;
      if(index > m_max)
        return m_max;
      return index;
    }

    //! Clip index so it is within the range.
    IndexRange<1> clip(const IndexRange<1> &range) const
    { return IndexRange<1>(clip(range.min()),clip(range.max())); }

    //! Access range of given dimension.
    const IndexRange<1> &operator[](unsigned i) const
    {
      assert(i == 0);
      return *this;
    }

    //! Access range of given dimension.
    IndexRange<1> &operator[](unsigned i)
    {
      assert(i == 0);
      return *this;
    }

    //! Start of the range.
    IndexRangeIterator<1> begin() const;

    //! One passed the end of the range.
    IndexRangeIterator<1> end() const;

  protected:
    int m_min {0};
    int m_max {-1};
  };

  //! N-dimensional range of index's
  // The range is inclusive of all values.

  template<unsigned N>
  class IndexRange
  {
  public:
    //! Default constructor
    IndexRange() noexcept
    {}

    //! Build a new range given two parts
    IndexRange(const IndexRange<N-1> &start,const IndexRange<1> &extra) noexcept
    {
      for(unsigned i = 0;i < N-1;i++)
        m_range[i] = start[i];
      m_range[N-1] = extra;
    }

    //! Build a new range given two parts
    IndexRange(const IndexRange<N-1> &start,int extraSize) noexcept
    {
      for(unsigned i = 0;i < N-1;i++)
        m_range[i] = start[i];
      m_range[N-1] = IndexRange<1>(extraSize);
    }

    //! Default constructor
    IndexRange(const Index<N> &min,const Index<N> &max) noexcept
    {
      for(unsigned i = 0;i < N;i++)
        m_range[i] = IndexRange<1>(min[i],max[i]);
    }

    //! Construct from sizes for each dimension.
    //! The ranges will have a zero origin.
    IndexRange(std::initializer_list<int> sizes) noexcept
    {
      assert(sizes.size() == N);
      for(unsigned i = 0;i < N;i++)
        m_range[i] = IndexRange<1>(sizes.begin()[i]);
    }

    //! Construct from ranges for each dimension.

    IndexRange(std::initializer_list<std::initializer_list<int> > sizes) noexcept
    {
      assert(sizes.size() == N);
      for(unsigned i = 0;i < N;i++) {
        m_range[i] = IndexRange<1>(sizes.begin()[i]);
      }
    }

    //! Make size of range 0.
    void clear() noexcept
    { m_range[0].clear(); }

    //! Get the size of the range in each dimension.
    Index<N> size() const noexcept
    {
      Index<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].size();
      return ret;
    }

    //! Get size of given dimension
    int size(unsigned n) const noexcept
    { return m_range[n].size(); }

    //! Get total number of elements covered by range.
    size_t elements() const
    {
      size_t n = 1;
      for(unsigned i = 0;i < N;i++)
        n *= m_range[i].size();
      return n;
    }

    //! Is range empty ?
    bool empty() const noexcept
    {
      for(unsigned i = 0;i < N;i++)
        if(m_range[i].empty())
          return true;
      return false;
    }

    //! Test if an index is contained within the range.
    bool contains(const Index<N> &ind) const noexcept
    {
      for(unsigned i = 0;i < N;i++)
        if(!m_range[i].contains(ind[i]))
          return false;
      return true;
    }

    //! Test if 'range' is contained with this one
    bool contains(const IndexRange<N> &range) const noexcept
    {
      for(unsigned i = 0;i < N;i++)
        if(m_range[i].contains(range.m_range[i]))
          return false;
      return true;
    }

    //! Shrink the range in from both ends by amount.
    IndexRange<N> shrink(int amount) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].shrink(amount);
      return ret;
    }

    //! Shrink the range in from both ends by amount.
    IndexRange<N> shrink(const Index<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].shrink(ind[i]);
      return ret;
    }

    //! Shrink the range by given size,
    //! min is increased by min of amount.min(), and max decreased by amount.max()
    IndexRange<N> shrink(const IndexRange<N> &amount) const
    {
      IndexRange<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].shrink(amount[i]);
      return ret;
    }

    //! Shrink the range in from both ends by amount.
    IndexRange<N> expand(int amount) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].expand(amount);
      return ret;
    }

    //! Shrink the range in from both ends by amount.
    IndexRange<N> expand(const Index<N> &ind) const noexcept
    {
      IndexRange<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].expand(ind[i]);
      return ret;
    }

    //! Shift range by given values.
    IndexRange<N> &operator+=(const Index<N> &ind) noexcept
    {
      for(unsigned i = 0;i < N;i++)
        m_range[i] += ind[i];
      return *this;
    }

    //! Shift this range in dimension n by the given 'amount'
    void shift(unsigned n,int amount)
    { m_range[n] += amount; }

    //! Smallest index in all dimensions.
    Index<N> min() const
    {
      Index<N> min;
      for(unsigned i = 0;i < N;i++)
        min[i] = m_range[i].min();
      return min;
    }

    //! largest index in all dimensions.
    Index<N> max() const
    {
      Index<N> max;
      for(unsigned i = 0;i < N;i++)
        max[i] = m_range[i].max();
      return max;
    }

    //! Create a new index clipped so it is within the range.
    Index<N> clip(const Index<N> &index) const
    {
      Index<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].clip(index[i]);
      return ret;
    }

    //! Access range of given dimension.
    const IndexRange<1> &operator[](unsigned i) const
    {
      assert(i >= 0 && i < N);
      return m_range[i];
    }

    //! Access range of given dimension.
    IndexRange<1> &operator[](unsigned i)
    {
      assert(i >= 0 && i < N);
      return m_range[i];
    }

    //! Clip index so it is within the range.
    IndexRange<N> clip(const IndexRange<N> &range) const
    {
      IndexRange<N> ret;
      for(unsigned i = 0;i < N;i++)
        ret[i] = m_range[i].clip(range[i]);
      return ret;
    }

    //! Start of the range.
    IndexRangeIterator<N> begin() const;

    //! One passed the end of the range.
    IndexRangeIterator<N> end() const;
  protected:
    IndexRange<1> m_range[N];
  };

  template<unsigned N>
  std::ostream &operator<<(std::ostream &strm,const IndexRange<N> &rng)
  {
    for(unsigned i = 0;i < N;i++) {
      strm << "(" << rng[i].min() << "," << rng[i].max() << ")";
    }

    return strm;
  }

  //! Iterator through a 1 dimensional range.

  template<>
  class IndexRangeIterator<1>
  {
  public:
    //! Constructor iterator
    IndexRangeIterator(int at)
     : m_at(at)
    {}

    //! Access index.
    int operator*() const
    { return m_at; }

    //! Increment
    IndexRangeIterator<1> &operator++(int)
    {
      m_at++;
      return *this;
    }

    //! Increment
    IndexRangeIterator<1> operator++()
    {
      int v = ++m_at;
      return IndexRangeIterator<1>(v);
    }

    //! Decrement
    IndexRangeIterator<1> &operator--(int)
    {
      m_at++;
      return *this;
    }

    bool operator!=(const IndexRangeIterator<1> &other) const
    { return other.m_at != m_at; }

    bool operator==(const IndexRangeIterator<1> &other) const
    { return other.m_at == m_at; }

  protected:
    int m_at;
  };

  //! Start of the range.
  IndexRangeIterator<1> IndexRange<1>::begin() const
  { return m_min; }

  //! One passed the end of the range.
  IndexRangeIterator<1> IndexRange<1>::end() const
  { return m_max+1; }

  //! Iterate through an N dimensional range.

  template<unsigned N>
  class IndexRangeIterator
  {
  public:
    IndexRangeIterator(const IndexRange<N> &range,const Index<N> &at)
     : m_range(&range),
       m_at(at)
    {}

    //! Access current index.
    const Index<N> &operator*() const
    { return m_at; }

    //! Increment position.
    void operator++()
    {
      for(unsigned i = N-1;i > 0;--i) {
        ++m_at[i];
        if(m_at[i] <= m_range->max()[i])
          return ;
        m_at[i] = m_range->min()[i];
      }
      ++m_at[0];
    }

    //! Equality test.
    bool operator==(const IndexRangeIterator<N> &other) const
    { return m_at == other.m_at; }

    //! Equality test.
    bool operator!=(const IndexRangeIterator<N> &other) const
    { return m_at != other.m_at; }
  protected:
    const IndexRange<N> *m_range;
    Index<N> m_at;
  };

  //! Start of the range.
  template<unsigned N>
  inline IndexRangeIterator<N> IndexRange<N>::begin() const
  { return IndexRangeIterator<N>(*this,min()); }

  //! One passed the end of the range.
  template<unsigned N>
  inline IndexRangeIterator<N> IndexRange<N>::end() const
  {
    Index<N> end = min();
    end[0] = m_range[0].max()+1;
    return IndexRangeIterator<N>(*this,end);
  }

}



#endif /* RANGE_HH_ */
