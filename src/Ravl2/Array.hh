/*
 * Array.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */

#pragma once

#include <array>
#include "Ravl2/Index.hh"

namespace Ravl2
{
  //! Global stride for 1 dimensional arrays. Always has a value of 1.
  extern const int gStride1;

  template<typename DataT,unsigned N>
  class Array;

  template<typename DataT,unsigned N>
  class ArrayAccess;

  template<typename DataT,unsigned N>
  class ArrayView;

  template<typename DataT,unsigned N>
  class ArrayIter;

  // Declaration of the concept “WindowedArray”, which is satisfied by any type “T”

  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimension>
  concept WindowedArray = requires(ArrayT a, Index<N> ind, IndexRange<N> rng)
  {
    { ArrayT::dimensions } -> std::convertible_to<unsigned>;
    { a[ind] } -> std::convertible_to<DataT>;
    { a.range() } -> std::convertible_to<IndexRange<N> >;
    { a.origin_address() } -> std::convertible_to<DataT *>;
    { a.strides() } -> std::convertible_to<const int *>;
  };

  //! Iterator for 1 dimensional array.

  template<typename DataT>
  class ArrayIter<DataT,1>
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = DataT;
    constexpr static unsigned dimensions = 1;

    constexpr explicit ArrayIter() = default;

    constexpr explicit ArrayIter(DataT *array)
      : mPtr(array)
    {}

    constexpr explicit ArrayIter([[maybe_unused]] IndexRange<1> &rng, DataT *array)
      : mPtr(array)
    {}

    //! Increment iterator
    constexpr ArrayIter<DataT,1> &operator++()
    {
      mPtr++;
      return *this;
    }

    //! Post Increment iterator
    constexpr ArrayIter<DataT,1> operator++(int)
    {
      ArrayIter<DataT,1> iter = *this;
      mPtr++;
      return iter;
    }

    //! Decrement iterator
    constexpr ArrayIter<DataT,1> &operator--()
    {
      mPtr--;
      return *this;
    }

    //! Decrement iterator
    constexpr ArrayIter<DataT,1> operator--(int)
    {
      ArrayIter<DataT,1> iter = *this;
      mPtr--;
      return iter;
    }

    //! Add an offset to the iterator
    template<typename IntegerT>
    requires std::is_integral_v<IntegerT>
    constexpr ArrayIter<DataT,1> &operator+=(IntegerT offset)
    {
      mPtr += offset;
      return *this;
    }

    //! Add an offset to the iterator
    template<typename IntegerT>
    requires std::is_integral_v<IntegerT>
    constexpr ArrayIter<DataT,1> &operator-=(IntegerT offset)
    {
      mPtr -= offset;
      return *this;
    }

    //! Spaceship operator
    [[nodiscard]] constexpr auto operator<=>(const ArrayIter<DataT,1> &other) const
    { return mPtr <=> other.mPtr; }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator==(const ArrayIter<DataT,1> &other) const
    { return mPtr == other.mPtr; }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator!=(const ArrayIter<DataT,1> &other) const
    { return mPtr != other.mPtr; }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator==(const DataT *other) const
    { return mPtr == other; }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator!=(const DataT *other) const
    { return mPtr != other; }

    //! Access element
    [[nodiscard]] constexpr DataT &operator*() const
    { return *mPtr; }

    //! Index access
    [[nodiscard]] constexpr DataT &operator[](int i) const
    { return mPtr[i]; }

  private:
    DataT *mPtr = nullptr;
  };

  //! Add to the iterator
  template<typename DataT,typename IntegerT>
  requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT,1> operator+(const ArrayIter<DataT,1> &it, IntegerT offset)
  {  return ArrayIter<DataT,1>(it.data() + offset); }

  //! Subtract from the iterator
  template<typename DataT,typename IntegerT>
  requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT,1> operator-(const ArrayIter<DataT,1> &it, IntegerT offset)
  { return ArrayIter<DataT,1>(it.data() + offset); }

  //! Add to the iterator
  template<typename DataT,typename IntegerT>
  requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT,1> operator+( IntegerT offset, const ArrayIter<DataT,1> &it)
  {  return ArrayIter<DataT,1>(offset + it.data()); }

  //! Subtract from the iterator
  template<typename DataT,typename IntegerT>
  requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT,1> operator-( IntegerT offset,const ArrayIter<DataT,1> &it)
  { return ArrayIter<DataT,1>(offset + it.data()); }

  //! Add an offset to the iterator
  template<typename DataT>
  [[nodiscard]] constexpr typename ArrayIter<DataT,1>::difference_type operator-(ArrayIter<DataT,1> o1,ArrayIter<DataT,1> o2)
  { return o1.data() - o2.data(); }


  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N>
  class ArrayAccess
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimensions = N;

    ArrayAccess()
    = default;

    constexpr ArrayAccess(const IndexRange<N> &rng, DataT *data, const int *strides)
      : m_ranges(rng.range_data()),
	m_data(data),
	m_strides(strides)
    {}

    constexpr ArrayAccess(const IndexRange<1> *rng, DataT *data, const int *strides)
      : m_ranges(rng),
	m_data(data),
	m_strides(strides)
    {}

    template<typename ArrayT>
      requires WindowedArray<ArrayT,DataT,N>
      constexpr explicit ArrayAccess(ArrayT &array)
      : m_ranges(array.range().range_data()),
	m_data(array.origin_address()),
	m_strides(array.strides())
    {}

    template<typename ArrayT>
    requires WindowedArray<ArrayT,DataT,N>
    constexpr explicit ArrayAccess(ArrayT &array, IndexRange<N> &range)
      : m_ranges(range.range_data()),
	m_data(array.origin_address()),
	m_strides(array.strides())
    {
      assert(array.range().contains(range));
    }

    //! Access origin address
    [[nodiscard]] constexpr DataT *origin_address() const
    { return m_data; }

    //! Drop index one level.
    [[nodiscard]] constexpr ArrayAccess<DataT, N - 1> operator[](int i)
    {
      assert(m_ranges[0].contains(i));
      return ArrayAccess<DataT, N - 1>(m_ranges + 1, m_data + (m_strides[0] * i), m_strides + 1);
    }

    //! Drop index one level.
    [[nodiscard]] constexpr DataT &operator[](const Index<N> &ind)
    {
      DataT *mPtr = m_data;
      for(unsigned i = 0;i < N;i++)
      {
	assert(m_ranges[i].contains(ind[i]));
	mPtr += m_strides[i] * ind[i];
      }
      return *mPtr;
    }

    //! Partial index
    template<unsigned M>
    requires (M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind)
    {
      DataT *mPtr = m_data;
      for(unsigned i = 0;i < M;i++)
      {
	assert(m_ranges[i].contains(ind[i]));
	mPtr += m_strides[i] * ind[i];
      }
      return ArrayAccess<DataT, N - M>(m_ranges + M, mPtr, m_strides + M);
    }

    //! Partial index
    template<unsigned M>
    requires (M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind) const
    {
      const DataT *mPtr = m_data;
      for(unsigned i = 0;i < M;i++)
      {
	assert(m_ranges[i].contains(ind[i]));
	mPtr += m_strides[i] * ind[i];
      }
      return ArrayAccess<const DataT, N - M>(m_ranges + M, mPtr, m_strides + M);
    }

    //! Index an element
    [[nodiscard]] constexpr const DataT &operator[](const Index<N> &ind) const
    {
      const DataT *mPtr = m_data;
      for(unsigned i = 0;i < N;i++)
      {
	assert(m_ranges[i].contains(ind[i]));
	mPtr += m_strides[i] * ind[i];
      }
      return *mPtr;
    }

    //! Range of index's for dim
    [[nodiscard]] constexpr const IndexRange<1> &range(unsigned i) const
    {
      assert(i < N);
      return m_ranges[i];
    }

    //! Access range data
    [[nodiscard]] constexpr const IndexRange<1> *range_data() const
    { return m_ranges; }

    //! Range of index's for row
    [[nodiscard]] constexpr IndexRange<N> range() const
    {
      IndexRange<N> rng;
      for(unsigned i = 0;i < N;i++)
	rng[i] = m_ranges[i];
      return rng;
    }

    //! Is array empty ?
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      if(m_ranges == nullptr) return true;
      for(unsigned i = 0;i < N;i++)
	if(m_ranges[i].empty())
	  return true;
      return false;
    }

    //! Begin iterator
    [[nodiscard]] constexpr ArrayIter<DataT,N> begin() const;

    //! End iterator
    [[nodiscard]] constexpr ArrayIter<DataT,N> end() const;

    //! Begin iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,N> cbegin() const;

    //! End iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,N> cend() const;

    //! Get stride for dimension
    [[nodiscard]] constexpr int stride(unsigned dim) const
    {
      assert(dim < N);
      return m_strides[dim];
    }

    //! Access strides
    [[nodiscard]] constexpr const int *strides() const
    { return m_strides; }

    //! Fill the array with a value
    constexpr void fill(const DataT &value)
    {
      for(auto & it : *this)
        it = value;
    }

  protected:
    const IndexRange<1> *m_ranges {nullptr};
    DataT *m_data {nullptr};
    const int *m_strides {nullptr}; //! Strides of each dimension of the array.
  };

  //! Iterator for N dimensional array.

  template<typename DataT,unsigned N>
  class ArrayIter
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = DataT;
    constexpr static unsigned dimensions = N;

    constexpr ArrayIter()
    = default;

    constexpr ArrayIter(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
     : m_access(rng,data,strides)
    {
      assert(data != nullptr);
      mPtr = data;
      for(unsigned i = 0;i < N;i++)
      {
	mPtr += rng[i].min() * strides[i];
	mIndex[i] = rng[i].min();
      }
      mEnd = mPtr + rng[N-1].size() * strides[N-1];
    }

    constexpr ArrayIter(const IndexRange<N> &rng, DataT *data, const int *strides) noexcept
     : ArrayIter(rng.ranges().data(),data,strides)
    {}

    constexpr ArrayIter(const IndexRange<N> &rng, DataT *data, const std::array<int,N> &strides) noexcept
     : ArrayIter(rng.ranges().data(),data,strides.data())
    {}

    //! Construct end iterator
    static constexpr ArrayIter<DataT,N> end(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
    {
      ArrayIter<DataT,N> iter(rng,data,strides);
      iter.mPtr += strides[0] * rng[0].size();
      iter.mEnd = iter.mPtr;
      return iter;
    }

    //! Construct end iterator
    static constexpr ArrayIter<DataT,N> end(const IndexRange<N> &rng, DataT *data, const std::array<int,N> &strides) noexcept
    {
      return end(rng.ranges().data(),data,strides.data());
    }

  private:
    constexpr inline void nextIndex()
    {
      for(unsigned i = N-2;i > 0;--i) {
        ++mIndex[i];
        if(mIndex[i] <= m_access.range(i).max())
          return;
        mIndex[i] = m_access.range(i).min();
      }
      // On the last index we don't need to update
      ++mIndex[0];
    }

  public:
    constexpr void nextRow()
    {
      nextIndex();
      mPtr = m_access.origin_address();
      for(unsigned i = 0;i < N;i++)
      {
	mPtr += m_access.stride(i) * mIndex[i];
      }
      mEnd = mPtr + m_access.strides()[N-1] * m_access.range(N-1).size();
    }

    //! Increment iterator
    constexpr inline ArrayIter<DataT,N> &operator++()
    {
      mPtr++;
      if(mPtr == mEnd) {
        nextRow();
      }
      return *this;
    }

    //! Post increment iterator
    constexpr inline ArrayIter<DataT,N> operator++(int)
    {
      ArrayIter<DataT,N> iter = *this;
      ++(*this);
      return iter;
    }

    //! Increment iterator
    //! Returns true while we're on the same final dimension (row).
    constexpr inline bool next()
    {
      mPtr++;
      if(mPtr == mEnd) {
	nextRow();
        return false;
      }
      return true;
    }

    //! Test if the iterator is valid.
    [[nodiscard]] constexpr bool valid() const noexcept
    { return mIndex[0] <= m_access.range(0).max();  }

    //! Test if the iterator is finished.
    [[nodiscard]] constexpr bool done() const noexcept
    { return mIndex[0] > m_access.range(0).max();  }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator==(const ArrayIter<DataT,N> &other) const
    { return (mPtr == other.mPtr); }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator!=(const ArrayIter<DataT,N> &other) const
    { return !this->operator==(other); }

    //! Access element
    [[nodiscard]] constexpr DataT &operator*() const noexcept
    { return *mPtr; }

    //! Access element point
    [[nodiscard]] constexpr DataT *data() const noexcept
    { return mPtr; }

    //! Access strides
    [[nodiscard]] constexpr const int *strides() const noexcept
    { return m_access.strides(); }

    //! Access Index
    [[nodiscard]] constexpr Index<N> index() const noexcept
    {
      Index<N> ind = mIndex;
      ind[N-1] = int(mPtr - mEnd + m_access.range(N-1).max() + 1);
      return ind;
    }

    //! Iterator difference
    //! This is the number of elements between the two iterators in the first iterators range.
    [[nodiscard]] constexpr difference_type operator-(const ArrayIter<DataT,N> &other) const noexcept
    {
      auto at1 = index();
      auto at2 = other.index();
      difference_type diff = (at1[1] - at2[1]) + (at1[0] - at2[0]) * m_access.range(0).size();
      return diff;
    }

  protected:
     DataT * mPtr {};
     const DataT * mEnd {};
     Index<N> mIndex {}; // Index of the beginning of the last dimension.
     ArrayAccess<DataT,N> m_access;
  };

  //! 1 dimensional access

  template<typename DataT>
  class ArrayAccess<DataT,1>
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimensions = 1;

    constexpr ArrayAccess(const IndexRange<1> *rng, DataT *data, [[maybe_unused]] const int *strides)
     : m_ranges(rng),
       m_data(data)
    {
      assert(*strides == 1);
    }

    constexpr ArrayAccess(const IndexRange<1> &rng, DataT *data, [[maybe_unused]] const int *strides)
      : m_ranges(&rng),
	m_data(data)
    {
      assert(*strides == 1);
    }

    constexpr ArrayAccess(const IndexRange<1> *rng, DataT *data)
      : m_ranges(rng),
	m_data(data)
    {}

    template<typename ArrayT>
      requires WindowedArray<ArrayT,DataT,1>
    explicit constexpr ArrayAccess(ArrayT &array)
      : m_ranges(array.range().range_data()),
	m_data(array.origin_address())
    {}

    //! Access origin address
    [[nodiscard]] constexpr DataT *origin_address() const
    { return m_data; }

    //! Access indexed element
    [[nodiscard]] inline constexpr DataT &operator[](int i) noexcept
    {
      assert((*m_ranges).contains(i));
      return m_data[i];
    }

    //! Access indexed element
    [[nodiscard]] inline constexpr DataT &operator[](Index<1> i) noexcept
    {
      assert((*m_ranges).contains(i));
      return m_data[i.index(0)];
    }

    //! Access indexed element.
    [[nodiscard]] inline constexpr const DataT &operator[](int i) const noexcept
    {
      assert(m_ranges->contains(i));
      return m_data[i];
    }

    //! Access indexed element.
    [[nodiscard]] inline constexpr const DataT &operator[](Index<1> i) const noexcept
    {
      assert(m_ranges->contains(i));
      return m_data[i.index(0)];
    }

    //! Range of index's for row
    [[nodiscard]] constexpr const IndexRange<1> &range() const
    { return *m_ranges; }

    //! Range of index's for dim
    [[nodiscard]] constexpr const IndexRange<1> &range([[maybe_unused]] unsigned i) const
    {
      assert(i == 0);
      return *m_ranges;
    }

    //! Access range data
    [[nodiscard]] constexpr const IndexRange<1> *range_data() const
    { return m_ranges; }

    //! Is array empty ?
    [[nodiscard]] constexpr bool empty() const noexcept
    { return m_ranges->empty(); }

    //! Begin iterator
    [[nodiscard]] constexpr ArrayIter<DataT,1> begin() const;

    //! End iterator
    [[nodiscard]] constexpr ArrayIter<DataT,1> end() const;

    //! Begin iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,1> cbegin() const;

    //! End iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,1> cend() const;

    //! Get stride for dimension
    [[nodiscard]] constexpr static int stride([[maybe_unused]] unsigned dim)
    { return 1; }

    //! Access strides
    [[nodiscard]] constexpr static const int *strides()
    { return &gStride1; }

    //! Fill the array with a value
    constexpr void fill(const DataT &value)
    {
      for(auto & it : *this)
        it = value;
    }

  protected:
    const IndexRange<1> *m_ranges;
    DataT *m_data;
  };

  template<typename DataT>
  constexpr ArrayIter<DataT, 1> ArrayAccess<DataT, 1>::begin() const
  { return ArrayIter<DataT,1>(&m_data[m_ranges->min()]); }

  template<typename DataT, unsigned int N>
  constexpr ArrayIter<DataT, N> ArrayAccess<DataT, N>::end() const
  { return ArrayIter<DataT,N>::end(m_ranges, m_data, m_strides); }

  template<typename DataT>
  constexpr ArrayIter<const DataT, 1> ArrayAccess<DataT, 1>::cbegin() const
  { return ArrayIter<const DataT,1>(&m_data[m_ranges->min()]); }

  template<typename DataT, unsigned int N>
  constexpr ArrayIter<const DataT, N> ArrayAccess<DataT, N>::cend() const
  { return ArrayIter<const DataT,N>::end(m_ranges, m_data, m_strides); }

  template<typename DataT, unsigned int N>
  constexpr ArrayIter<DataT, N> ArrayAccess<DataT, N>::begin() const
  { return ArrayIter<DataT,N>(m_ranges, m_data, m_strides); }

  template<typename DataT>
  constexpr ArrayIter<DataT, 1> ArrayAccess<DataT, 1>::end() const
  { return ArrayIter<DataT,1>(&m_data[m_ranges->max()+1]); }

  template<typename DataT, unsigned int N>
  constexpr ArrayIter<const DataT, N> ArrayAccess<DataT, N>::cbegin() const
  { return ArrayIter<const DataT,N>(m_ranges, m_data, m_strides); }

  template<typename DataT>
  constexpr ArrayIter<const DataT, 1> ArrayAccess<DataT, 1>::cend() const
  { return ArrayIter<const DataT,1>(&m_data[m_ranges->max()+1]); }

  //! Non-owning view of an array values.

  template<typename DataT,unsigned N>
  class ArrayView
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimensions = N;

  protected:

    //! Generate strides
      constexpr void make_strides(const IndexRange<N> &range)
      {
        int s = 1;
        for(unsigned i = N-1;i > 0;--i) {
          //std::cout << " " << i << " s=" << s << "\n";
          m_strides[i] = s;
          s *= range.size(i);
        }
        m_strides[0] = s;
      }

    constexpr int compute_origin_offset(const IndexRange<N> &range) const
      {
        int off = 0;
        for(unsigned i = 0;i < N;i++) {
          off -= range[i].min() * m_strides[i];
        }
        return off;
      }

      explicit constexpr ArrayView(const IndexRange<N> &range, const std::array<int,N> &strides)
        : m_range(range),
          m_strides(strides)
      {}

      explicit constexpr ArrayView(const IndexRange<N> &range)
        : m_range(range)
      {}


  public:

      explicit constexpr ArrayView(DataT *data, const IndexRange<N> &range, const std::array<int,N> &strides)
        : m_data(data),
          m_range(range),
          m_strides(strides)
      {}

      //! Create an empty array
      constexpr ArrayView()
      = default;

      template<typename ArrayT>
      requires WindowedArray<ArrayT,DataT,N>
      explicit constexpr ArrayView(ArrayT &array)
	: m_data(array.origin_address())
      {
	for(unsigned i = 0;i < N;i++) {
	  m_strides[i] = array.stride(i);
	  m_range[i] = array.range(i);
	}
      }

      template<typename ArrayT>
      requires WindowedArray<ArrayT,DataT,N>
      explicit constexpr ArrayView(ArrayT &array, const IndexRange<N> &range)
	: m_data(array.origin_address()),
          m_range(range)

      {
	for(int i = 0;i < N;i++) {
	  m_strides[i] = array.stride(i);
	  if(!array.range(i).contains(range[i]))
	    throw std::out_of_range("requested range is outside that of the original array");
	}
      }

      //! Get a view of the array with the requested 'range'
      [[nodiscard]] constexpr ArrayView<DataT,N> view(const IndexRange<N> &range)
      {
        return ArrayView<DataT,N>(m_data, range, m_strides);
      }

      //! Get a view of the array with the requested 'range'
      [[nodiscard]] constexpr ArrayView<const DataT,N> view(const IndexRange<N> &range) const
      {
        assert(m_range.contains(range));
	assert(m_data != nullptr);
        return ArrayView<const DataT,N>(m_data,range,m_strides);
      }

      //! Get an access of the array with the requested 'range'
      [[nodiscard]] constexpr ArrayAccess<DataT,N> access(const IndexRange<N> &range)
      {
	assert(m_range.contains(range));
	assert(m_data != nullptr);
	return ArrayAccess<DataT,N>(range.range_data(),m_data,m_strides.data());
      }

      //! Get an access of the array with the requested 'range'
      [[nodiscard]] constexpr ArrayAccess<const DataT,N> access(const IndexRange<N> &range) const
      {
        assert(m_range.contains(range));
	assert(m_data != nullptr);
        return ArrayAccess<const DataT,N>(range.range_data(),m_data,m_strides.data());
      }

      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] constexpr DataT *origin_address() const
      { return m_data; }

      //! Access next dimension of array.
      [[nodiscard]] constexpr auto operator[](int i)
      {
        assert(m_range[0].contains(i));
        return ArrayAccess<DataT, N - 1>(&m_range[1], m_data + i * m_strides[0], &m_strides[1]);
      }

      //! Partial index
      template<unsigned M>
      requires (M < N)
      [[nodiscard]] constexpr auto operator[](const Index<M> &ind)
      {
	DataT *mPtr = m_data;
	for(unsigned i = 0;i < M;i++)
	{
	  assert(m_range[i].contains(ind[i]));
	  mPtr += m_strides[i] * ind[i];
	}
	return ArrayAccess<DataT, N - M>(&m_range[M], mPtr, &m_strides[M]);
      }

      //! Partial index
      template<unsigned M>
      requires (M < N)
      [[nodiscard]] constexpr auto operator[](const Index<M> &ind) const
      {
	const DataT *mPtr = m_data;
	for(unsigned i = 0;i < M;i++)
	{
	  assert(m_range[i].contains(ind[i]));
	  mPtr += m_strides[i] * ind[i];
	}
	return ArrayAccess<const DataT, N - M>(&m_range[M], mPtr, &m_strides[M]);
      }

    //! Access next dimension of array.
      [[nodiscard]] constexpr auto operator[](int i) const
      {
        assert(m_range[0].contains(i));
        return ArrayAccess<const DataT, N - 1>(&m_range[1], m_data + i * m_strides[0], &m_strides[1]);
      }

      //! Access next dimension of array.
      [[nodiscard]] constexpr DataT &operator[](const Index<N> &ind)
      {
        int dind = 0;
        for(unsigned i = 0;i < N-1;i++) {
          assert(m_range[i].contains(ind.index(i)));
          dind += m_strides[i] * ind.index(i);
        }
        dind += ind.index(N-1);
        return m_data[dind];
      }

      //! Access next dimension of array.
      [[nodiscard]] constexpr const DataT &operator[](const Index<N> &ind) const
      {
        int dind = 0;
        for(unsigned i = 0;i < N-1;i++) {
          assert(m_range[i].contains(ind.index(i)));
          dind += m_strides[i] * ind.index(i);
        }
        dind += ind.index(N-1);
        return m_data[dind];
      }

      //! Get begin iterator
      [[nodiscard]] constexpr ArrayIter<DataT,N> begin() const
      {
        assert(m_range.empty() || m_data != nullptr);
        return ArrayIter<DataT,N>(m_range, m_data, m_strides);
      }

      //! Get end iterator
      [[nodiscard]] constexpr ArrayIter<DataT,N> end() const
      {
        assert(m_range.empty() || m_data != nullptr);
        return ArrayIter<DataT,N>::end(m_range, m_data, m_strides);
      }

    //! Get begin iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,N> cbegin() const
    {
      assert(m_range.empty() || m_data != nullptr);
      return ArrayIter<const DataT,N>(m_range, m_data, m_strides);
    }

    //! Get end iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,N> cend() const
    {
      assert(m_range.empty() || m_data != nullptr);
      return ArrayIter<const DataT,N>::end(m_range, m_data, m_strides);
    }

    //! access range of first index array
      [[nodiscard]] constexpr const IndexRange<N> &range() const
      { return m_range; }

      //! access range of first index array
      [[nodiscard]] constexpr const IndexRange<1> &range(unsigned dim) const
      {
        assert(dim < N);
        return m_range[dim];
      }

      //! Access range data
      [[nodiscard]] constexpr const IndexRange<1> *range_data() const
      { return m_range.range_data(); }

    //! Is array empty ?
      [[nodiscard]] constexpr bool empty() const noexcept
      { return m_range.empty(); }

      //! Access stride size for given dimension
      [[nodiscard]] constexpr int stride(unsigned dim) const
      { return m_strides[dim]; }

      //! Access strides for each dimension
      [[nodiscard]] constexpr const int *strides() const
      { return m_strides.data(); }

      //! Fill array with value.
      // cppcheck-suppress functionConst
      constexpr void fill(const DataT &val)
      {
        for(auto &at : *this) {
          // cppcheck-suppress useStlAlgorithm
          at = val;
        }
      }

  protected:
      DataT *m_data = nullptr;
      IndexRange<N> m_range;
      std::array<int,N> m_strides {};
  };


  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N=1>
  class Array
    : public ArrayView<DataT,N>
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimensions = N;

    //! Create an empty array
    constexpr Array()
    = default;

    //! Create an array of the given range.
    explicit constexpr Array(const IndexRange<N> &range)
     : ArrayView<DataT, N>(range),
       m_buffer(std::make_shared<DataT[]>(size_t(range.elements())))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer.get()[this->compute_origin_offset(range)]);
    }

    //! Construct with an existing buffer
    explicit constexpr Array(DataT *data, const IndexRange<N> &range, const std::array<int,N> &strides, const std::shared_ptr<DataT[]> &buffer)
      : ArrayView<DataT, N>(data,range,strides),
        m_buffer(buffer)
    {}

    //! Construct with an existing buffer
    explicit constexpr Array(DataT *data, const IndexRange<N> &range, const std::array<int,N> &strides, std::shared_ptr<DataT[]> &&buffer)
        : ArrayView<DataT, N>(data,range,strides),
          m_buffer(std::move(buffer))
    {}

    //! Create an array from a set of sizes.
    constexpr Array(std::initializer_list<IndexRange<1>> ranges)
      : ArrayView<DataT, N>(IndexRange<N>(ranges)),
        m_buffer(std::make_shared<DataT[]>(this->m_range.elements()))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer.get()[this->compute_origin_offset(this->m_range)]);
    }

    //! Create an array from a set of sizes.
    constexpr Array(std::initializer_list<size_t> sizes)
     : ArrayView<DataT, N>(IndexRange<N>(sizes)),
       m_buffer(std::make_shared<DataT[]>(this->m_range.elements()))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer.get()[this->compute_origin_offset(this->m_range)]);
    }

    //! Create an array from a set of sizes.
    constexpr Array(std::initializer_list<size_t> sizes, const DataT &fillData)
      : ArrayView<DataT, N>(IndexRange<N>(sizes)),
	m_buffer(std::make_shared<DataT[]>(this->m_range.elements(),fillData))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer.get()[this->compute_origin_offset(this->m_range)]);
    }

    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    constexpr Array(Array<DataT,N> &original, const IndexRange<N> &range)
     : ArrayView<DataT, N>(original),
       m_buffer(original.buffer())
    {
      if(!original.range().contains(range))
        throw std::out_of_range("requested range is outside that of the original array");
      this->m_range = range;
      this->m_data = original.origin_address();
    }

    using ArrayView<DataT,N>::begin;
    using ArrayView<DataT,N>::end;

    //! Access buffer.
    [[nodiscard]] constexpr auto &buffer()
    { return m_buffer; }
  protected:
    std::shared_ptr<DataT[]> m_buffer;
  };


  //! Access for an N dimensional element of an array.

  template<typename DataT>
  class ArrayView<DataT,1>
  {
  public:
      using value_type = DataT;
      constexpr static unsigned dimensions = 1;

      //! Create an sub array with the requested 'range'
      //! Range must be entirely contained in the original array.
      constexpr ArrayView(Array<DataT,1> &original, const IndexRange<1> &range)
         : m_data(original.origin_address()),
           m_range(range)
      {
        if(!original.range().contains(range))
          throw std::out_of_range("requested range is outside that of the original array");
      }

      explicit constexpr ArrayView(const std::vector<DataT> &vec)
          : m_data(vec.data()),
            m_range(vec.size())
      {}

      explicit constexpr ArrayView(DataT *data, const IndexRange<1> &range)
         : m_data(data),
            m_range(range)
      {}

      constexpr ArrayView() = default;
  protected:
      //! Create an array of the given size.
      explicit constexpr ArrayView(const IndexRange<1> &range)
        : m_range(range)
      {}

  public:
      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] constexpr DataT *origin_address() const
      { return m_data; }

      //! Access next dimension of array.
      [[nodiscard]] constexpr DataT &operator[](int i)
      {
        assert(m_range.contains(i));
        return m_data[i];
      }

      //! Access next dimension of array.
      [[nodiscard]] constexpr const DataT &operator[](int i) const
      {
        assert(m_range.contains(i));
        return m_data[i];
      }

      //! Access next dimension of array.
      [[nodiscard]] constexpr DataT &operator[](Index<1> i)
      {
        assert(m_range.contains(i));
        return m_data[i.index(0)];
      }

      //! Access next dimension of array.
      [[nodiscard]] constexpr const DataT &operator[](Index<1> i) const
      {
        assert(m_range.contains(i));
        return m_data[i.index(0)];
      }

      //! Access range of array
      [[nodiscard]] constexpr const IndexRange<1> &range() const
      { return m_range; }

      //! Is array empty ?
      [[nodiscard]] constexpr bool empty() const noexcept
      { return m_range.empty(); }

      //! Begin iterator
      [[nodiscard]] constexpr ArrayIter<DataT,1> begin()
      { return ArrayIter<DataT,1>(m_data); }

      //! End iterator
      [[nodiscard]] constexpr ArrayIter<DataT,1> end()
      { return ArrayIter<DataT,1>(m_data + m_range.size()); }

    //! Begin iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,1> begin() const
    { return ArrayIter<DataT,1>(m_data); }

    //! End iterator
    [[nodiscard]] constexpr ArrayIter<const DataT,1> end() const
    { return ArrayIter<DataT,1>(m_data + m_range.size()); }

    //! Get the index of the given pointer within the array.
      [[nodiscard]]
      constexpr Index<1> indexOf(const DataT *ptr) const
      {
        return Index<1>({int(ptr - m_data)});
      }

      //! Fill array with value.
      constexpr void fill(const DataT &val)
      {
        for(auto &at : *this) {
          // cppcheck-suppress useStlAlgorithm
          at = val;
        }
      }

    [[nodiscard]] constexpr static int stride([[maybe_unused]] unsigned dim)
      { return 1; }

    [[nodiscard]] constexpr static const int *strides()
      { return &gStride1; }

  protected:
      DataT *m_data = nullptr;
      IndexRange<1> m_range;
  };


  //! Access for an N dimensional element of an array.

  template<typename DataT>
  class Array<DataT,1>
     : public ArrayView<DataT,1>
  {
  public:
    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    constexpr Array(Array<DataT,1> &original, const IndexRange<1> &range)
     : ArrayView<DataT, 1>(original, range),
       m_buffer(original.buffer())
    {}

    explicit constexpr Array(std::vector<DataT> &&vec)
     : ArrayView<DataT, 1>(vec.data(),IndexRange<1>(0,int(vec.size())-1)),
       m_buffer(std::shared_ptr<DataT[]>(this->m_data,[aVec = std::move(vec)]([[maybe_unused]] DataT *delPtr){ assert(aVec.data() == delPtr);}))
    {}

    explicit constexpr Array(const std::vector<DataT> &vec)
     : ArrayView<DataT, 1>(IndexRange<1>(0,int(vec.size())-1)),
       m_buffer(std::make_shared<DataT[]>(vec.size()))
    {
      std::copy(vec.begin(),vec.end(),m_buffer.get());
      this->m_data = m_buffer.get();
    }

    //! Create an array of the given size.
    explicit constexpr Array(const IndexRange<1> &range)
     : ArrayView<DataT, 1>(range),
       m_buffer(std::make_shared<DataT[]>(range.elements()))
    {
      this->m_data = &m_buffer.get()[-range.min()];
    }

    //! Construct an empty array
    explicit constexpr Array() = default;

    //! Access buffer.
    [[nodiscard]] auto &buffer()
    { return m_buffer; }

    //! Begin iterator
  protected:
    std::shared_ptr<DataT[]> m_buffer;
  };

  //! Take a sub array of the given array.
  //! The range must be entirely contained in the original array and
  //! must exist for the lifetime of the sub array.
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  constexpr auto clip(ArrayT &array, const IndexRange<N> &range)
  {
    assert(array.range().contains(range));
    return ArrayAccess<DataT,N>(range,array.origin_address(),array.strides());
  }

  //! Take a sub array of the given array.
  //! The range must be entirely contained in the original array and
  //! must exist for the lifetime of the sub array.
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  std::ostream &operator<<(std::ostream &os, const ArrayT &array)
  {
    os << "Array " << array.range() << "\n  ";
    for(auto at = array.begin(); at.valid();) {
      do {
	os << *at << " ";
      } while(at.next());
      os << "\n  ";
    }
    os << "\n";
    return os;
  }

  void doNothing();

  // Let everyone know there's an implementation already generated for common cases
  extern template class Array<uint8_t,1>;
  extern template class Array<uint8_t,2>;
  extern template class ArrayAccess<uint8_t,1>;
  extern template class ArrayAccess<uint8_t,2>;
  extern template class ArrayIter<uint8_t,1>;
  extern template class ArrayIter<uint8_t,2>;

  extern template class Array<float,1>;
  extern template class Array<float,2>;
  extern template class ArrayAccess<float,1>;
  extern template class ArrayAccess<float,2>;
  extern template class ArrayIter<float,1>;
  extern template class ArrayIter<float,2>;
}

namespace fmt {
  template <typename DataT, unsigned N> struct formatter<Ravl2::Array<DataT,N>> : ostream_formatter {};
}


