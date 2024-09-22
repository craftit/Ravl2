/*
 * Array.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */

#pragma once

#include <array>
#include <span>
#include <cereal/cereal.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Sentinel.hh"
#include "Ravl2/IndexRange.hh"
#include "Ravl2/Array1.hh"

namespace Ravl2
{

  //! Access for an N dimensional element of an array.

  template <typename DataT, unsigned N>
  class ArrayAccess
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimensions = N;

    ArrayAccess() = default;

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

    template <typename ArrayT>
      requires WindowedArray<ArrayT, DataT, N>
    constexpr explicit ArrayAccess(ArrayT &array)
        : m_ranges(array.range_data()),
          m_data(array.origin_address()),
          m_strides(array.strides())
    {}

    template <typename ArrayT>
      requires WindowedArray<ArrayT, DataT, N>
    constexpr explicit ArrayAccess(ArrayT &array, IndexRange<N> &range)
        : m_ranges(range.range_data()),
          m_data(array.origin_address()),
          m_strides(array.strides())
    {
      assert(array.range().contains(range));
    }

    //! Access origin address
    [[nodiscard]] constexpr DataT *origin_address() const
    {
      return m_data;
    }

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
      for(unsigned i = 0; i < N - 1; i++) {
        assert(m_ranges[i].contains(ind[i]));
        mPtr += m_strides[i] * ind[i];
      }
      mPtr += ind[N - 1];
      return *mPtr;
    }

    //! Partial index
    template <unsigned M>
      requires(M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind)
    {
      DataT *mPtr = m_data;
      for(unsigned i = 0; i < M; i++) {
        assert(m_ranges[i].contains(ind[i]));
        mPtr += m_strides[i] * ind[i];
      }
      return ArrayAccess<DataT, N - M>(m_ranges + M, mPtr, m_strides + M);
    }

    //! Partial index
    template <unsigned M>
      requires(M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind) const
    {
      const DataT *mPtr = m_data;
      for(unsigned i = 0; i < M; i++) {
        assert(m_ranges[i].contains(ind[i]));
        mPtr += m_strides[i] * ind[i];
      }
      return ArrayAccess<const DataT, N - M>(m_ranges + M, mPtr, m_strides + M);
    }

    //! Index an element
    [[nodiscard]] constexpr const DataT &operator[](const Index<N> &ind) const
    {
      const DataT *mPtr = m_data;
      for(unsigned i = 0; i < N - 1; i++) {
        assert(m_ranges[i].contains(ind[i]));
        mPtr += m_strides[i] * ind[i];
      }
      mPtr += ind[N - 1];
      return *mPtr;
    }

    //! Element by element call operator access
    template <typename... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
    constexpr inline auto operator()(IndexDataT... data)
    {
      return operator[](Index<IndexN>({int(data)...}));
    }

    //! Element by element call operator access
    template <typename... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
    constexpr inline auto operator()(IndexDataT... data) const
    {
      return operator[](Index<IndexN>({int(data)...}));
    }

    //! Range of index's for dim
    [[nodiscard]] constexpr const IndexRange<1> &range(unsigned i) const
    {
      assert(i < N);
      return m_ranges[i];
    }

    //! Access range data
    [[nodiscard]] constexpr const IndexRange<1> *range_data() const
    {
      return m_ranges;
    }

    //! Range of index's for row
    [[nodiscard]] constexpr IndexRange<N> range() const
    {
      IndexRange<N> rng;
      for(unsigned i = 0; i < N; i++)
        rng[i] = m_ranges[i];
      return rng;
    }

    //! Is array empty ?
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      if(m_ranges == nullptr) return true;
      for(unsigned i = 0; i < N; i++)
        if(m_ranges[i].empty())
          return true;
      return false;
    }

    //! vertexBegin iterator
    [[nodiscard]] constexpr ArrayIter<DataT, N> begin() const;

    //! vertexEnd iterator
    [[nodiscard]] constexpr Sentinel end() const
    {
      return {};
    }

    //! vertexBegin iterator
    [[nodiscard]] constexpr ArrayIter<const DataT, N> cbegin() const;

    //! vertexEnd iterator
    [[nodiscard]] constexpr Sentinel cend() const
    {
      return {};
    }

    //! Get stride for dimension
    [[nodiscard]] constexpr int stride(unsigned dim) const
    {
      assert(dim < N);
      return m_strides[dim];
    }

    //! Access strides
    [[nodiscard]] constexpr const int *strides() const
    {
      return m_strides;
    }

    //! Broadcast assignment
    auto &operator=(const DataT &value)
    {
      fill(*this, value);
      return *this;
    }

#if 0
    //! Copy data from another array
    //! Experimental
    template<typename ArrayT,typename OtherDataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT,OtherDataT,N> && std::is_convertible_v<OtherDataT,DataT>
    auto &operator=(const ArrayT &array)
    {
      assert(array.range().size() == range().size());
      SPDLOG_INFO("Copy data from another array {}  ({}) ", array.range().size(),N);
      std::copy(array.begin(),array.end(),begin());
      return *this;
    }
#endif

  protected:
    const IndexRange<1> *m_ranges {nullptr};
    DataT *m_data {nullptr};
    const int *m_strides {nullptr};//! Strides of each dimension of the array.
  };

  //! Iterator for N dimensional array.

  template <typename DataT, unsigned N>
  class ArrayIter
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = DataT;
    constexpr static unsigned dimensions = N;

    constexpr ArrayIter() = default;

    constexpr ArrayIter(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
        : m_access(rng, data, strides)
    {
      assert(data != nullptr);
      mPtr = data;
      for(unsigned i = 0; i < N; i++) {
        mPtr += rng[i].min() * strides[i];
        mIndex[i] = rng[i].min();
      }
      mEnd = mPtr + rng[N - 1].size() * strides[N - 1];
    }

    constexpr ArrayIter(const IndexRange<N> &rng, DataT *data, const int *strides) noexcept
        : ArrayIter(rng.ranges().data(), data, strides)
    {}

    constexpr ArrayIter(const IndexRange<N> &rng, DataT *data, const std::array<int, N> &strides) noexcept
        : ArrayIter(rng.ranges().data(), data, strides.data())
    {}

    //! Construct end iterator
    static constexpr ArrayIter<DataT, N> end(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
    {
      ArrayIter<DataT, N> iter(rng, data, strides);
      iter.mPtr += strides[0] * rng[0].size();
      iter.mEnd = iter.mPtr;
      return iter;
    }

    //! Construct end iterator
    static constexpr ArrayIter<DataT, N> end(const IndexRange<N> &rng, DataT *data, const std::array<int, N> &strides) noexcept
    {
      return end(rng.ranges().data(), data, strides.data());
    }

  private:
    constexpr inline void nextIndex()
    {
      for(unsigned i = N - 2; i > 0; --i) {
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
      for(unsigned i = 0; i < N - 1; i++) {
        mPtr += m_access.stride(i) * mIndex[i];
      }
      mPtr += mIndex[N - 1];
      mEnd = mPtr + m_access.strides()[N - 1] * m_access.range(N - 1).size();
    }

    //! Increment iterator
    constexpr inline ArrayIter<DataT, N> &operator++()
    {
      mPtr++;
      if(mPtr == mEnd) {
        nextRow();
      }
      return *this;
    }

    //! Post increment iterator
    constexpr inline ArrayIter<DataT, N> operator++(int)
    {
      ArrayIter<DataT, N> iter = *this;
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
    {
      return mIndex[0] <= m_access.range(0).max();
    }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator==(const ArrayIter<DataT, N> &other) const
    {
      return (mPtr == other.mPtr);
    }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator!=(const ArrayIter<DataT, N> &other) const
    {
      return !this->operator==(other);
    }

    //! Access element
    [[nodiscard]] constexpr DataT &operator*() const noexcept
    {
      return *mPtr;
    }

    //! Access element point
    [[nodiscard]] constexpr DataT *data() const noexcept
    {
      return mPtr;
    }

    //! Access strides
    [[nodiscard]] constexpr const int *strides() const noexcept
    {
      return m_access.strides();
    }

    //! Access Index
    [[nodiscard]] constexpr Index<N> index() const noexcept
    {
      Index<N> ind = mIndex;
      ind[N - 1] = int(mPtr - mEnd + m_access.range(N - 1).max() + 1);
      return ind;
    }

    //! Access window with the current element at the origin.
    [[nodiscard]] constexpr ArrayAccess<DataT, N> window() const noexcept
    {
      return ArrayAccess<DataT, N>(m_access.range() - index(), mPtr, m_access.strides());
    }

    //! Access a point relative to the current element
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    [[nodiscard]] constexpr DataT &operator[](const Index<N> &ind) const
    {
      DataT *tPtr = mPtr;
      for(unsigned i = 0; i < N-1; i++) {
        tPtr += m_access.stride(i) * ind[i];
      }
      tPtr += ind[N-1];
      return *tPtr;
    }

    //! Partial index with current point as origin
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template <unsigned M>
      requires(M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind)
    {
      DataT *tPtr = mPtr;
      for(unsigned i = 0; i < M; i++) {
        tPtr += m_access.stride(i) * ind[i];
      }
      return ArrayAccess<DataT, N - M>(&(m_access.range_data()[M]), tPtr, &m_access.strides()[M]);
    }

    //! Element by element call operator access,
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template <IndexType... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
      requires (IndexN <= N)
    constexpr inline auto& at(IndexDataT... data) const
    {
      return operator[](Index<IndexN>({int(data)...}));
    }

    //! Element by element call operator access
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template <IndexType... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
      requires (IndexN <= N)
    constexpr inline auto& operator()(IndexDataT... data) const
    {
      return operator[](Index<IndexN>({int(data)...}));
    }

    //! Iterator difference
    //! This is the number of elements between the two iterators in the first iterators range.
    [[nodiscard]] constexpr difference_type operator-(const ArrayIter<DataT, N> &other) const noexcept
    {
      auto at1 = index();
      auto at2 = other.index();
      difference_type diff = (at1[1] - at2[1]) + (at1[0] - at2[0]) * m_access.range(0).size();
      return diff;
    }

    //! Access the remains of the row
    [[nodiscard]] constexpr std::span<DataT> row() const noexcept
    {
      return std::span<DataT>(mPtr, size_t(mEnd - mPtr));
    }

  protected:
    DataT *mPtr {};
    const DataT *mEnd {};
    Index<N> mIndex {};// Index of the beginning of the last dimension.
    ArrayAccess<DataT, N> m_access;
  };

  template <typename DataT, unsigned int N>
  constexpr ArrayIter<const DataT, N> ArrayAccess<DataT, N>::cbegin() const
  {
    return ArrayIter<const DataT, N>(m_ranges, m_data, m_strides);
  }

  template <typename DataT, unsigned int N>
  constexpr ArrayIter<DataT, N> ArrayAccess<DataT, N>::begin() const
  {
    return ArrayIter<DataT, N>(m_ranges, m_data, m_strides);
  }

  //! Non-owning view of an array values.

  template <typename DataT, unsigned N>
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
      for(unsigned i = N - 1; i > 0; --i) {
        //std::cout << " " << i << " s=" << s << "\n";
        m_strides[i] = s;
        s *= range.size(i);
      }
      m_strides[0] = s;
    }

    [[nodiscard]] constexpr int compute_origin_offset(const IndexRange<N> &range) const
    {
      int off = 0;
      for(unsigned i = 0; i < N; i++) {
        off -= range[i].min() * m_strides[i];
      }
      return off;
    }

    explicit constexpr ArrayView(const IndexRange<N> &range, const std::array<int, N> &strides)
        : m_range(range),
          m_strides(strides)
    {}

    explicit constexpr ArrayView(const IndexRange<N> &range)
        : m_range(range)
    {}

  public:
    explicit constexpr ArrayView(DataT *data, const IndexRange<N> &range, const std::array<int, N> &strides)
        : m_data(data),
          m_range(range),
          m_strides(strides)
    {}

    explicit constexpr ArrayView(DataT *data, const IndexRange<N> &range, const int *strides)
        : m_data(data),
          m_range(range)
    {
      memccpy(m_strides.data(), strides, sizeof(int), N);
    }

    //! Create an empty array
    constexpr ArrayView() = default;

    template <typename ArrayT>
      requires WindowedArray<ArrayT, DataT, N>
    explicit constexpr ArrayView(ArrayT &array)
        : m_data(array.origin_address())
    {
      for(unsigned i = 0; i < N; i++) {
        m_strides[i] = array.stride(i);
        m_range[i] = array.range(i);
      }
    }

    template <typename ArrayT>
      requires WindowedArray<ArrayT, DataT, N>
    explicit constexpr ArrayView(ArrayT &array, const IndexRange<N> &range)
        : m_data(array.origin_address()),
          m_range(range)

    {
      for(int i = 0; i < N; i++) {
        m_strides[i] = array.stride(i);
        if(!array.range(i).contains(range[i]))
          throw std::out_of_range("requested range is outside that of the original array");
      }
    }

    //! Access address of origin element
    //! Note: this may not point to a valid element
    [[nodiscard]] constexpr DataT *origin_address() const
    {
      return m_data;
    }

    //! Access next dimension of array.
    [[nodiscard]] constexpr auto operator[](int i)
    {
      assert(m_range[0].contains(i));
      return ArrayAccess<DataT, N - 1>(&m_range[1], m_data + i * m_strides[0], &m_strides[1]);
    }

    //! Partial index
    template <unsigned M>
      requires(M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind)
    {
      DataT *mPtr = m_data;
      for(unsigned i = 0; i < M; i++) {
        assert(m_range[i].contains(ind[i]));
        mPtr += m_strides[i] * ind[i];
      }
      return ArrayAccess<DataT, N - M>(&m_range[M], mPtr, &m_strides[M]);
    }

    //! Partial index
    template <unsigned M>
      requires(M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind) const
    {
      const DataT *mPtr = m_data;
      for(unsigned i = 0; i < M; i++) {
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
      for(unsigned i = 0; i < N - 1; i++) {
        assert(m_range[i].contains(ind.index(i)));
        dind += m_strides[i] * ind.index(i);
      }
      dind += ind.index(N - 1);
      return m_data[dind];
    }

    //! Access next dimension of array.
    [[nodiscard]] constexpr const DataT &operator[](const Index<N> &ind) const
    {
      int dind = 0;
      for(unsigned i = 0; i < N - 1; i++) {
        assert(m_range[i].contains(ind.index(i)));
        dind += m_strides[i] * ind.index(i);
      }
      dind += ind.index(N - 1);
      return m_data[dind];
    }

    //! Element by element call operator access
    template <IndexType... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
      requires (IndexN <= N)
    constexpr inline auto& operator()(IndexDataT... data)
    {
      return operator[](Index<IndexN>({int(data)...}));
    }

    //! Element by element call operator access
    template <IndexType... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
     requires (IndexN <= N)
    constexpr inline const auto &operator()(IndexDataT... data) const
    {
      return operator[](Index<IndexN>({int(data)...}));
    }

    //! Index an element
    template<unsigned M>
      requires(M <= N)
    [[nodiscard]]
    constexpr inline auto operator()(Index<M> ind) const
    {
      return operator[](ind);
    }

    //! Index an element
    template<unsigned M>
      requires(M <= N)
    [[nodiscard]]
    constexpr inline auto& operator()(Index<M> ind)
    {
      return operator[](ind);
    }


    //! Get begin iterator
    [[nodiscard]] constexpr ArrayIter<DataT, N> begin() const
    {
      assert(m_range.empty() || m_data != nullptr);
      return ArrayIter<DataT, N>(m_range, m_data, m_strides);
    }

    //! Get end iterator
    [[nodiscard]] constexpr Sentinel end() const
    {
      return {};
    }

    //! Get begin iterator
    [[nodiscard]] constexpr ArrayIter<const DataT, N> cbegin() const
    {
      assert(m_range.empty() || m_data != nullptr);
      return ArrayIter<const DataT, N>(m_range, m_data, m_strides);
    }

    //! Get end iterator
    [[nodiscard]] constexpr Sentinel cend() const
    {
      return {};
    }

    //! access range of first index array
    [[nodiscard]] constexpr const IndexRange<N> &range() const
    {
      return m_range;
    }

    //! access range of first index array
    [[nodiscard]] constexpr const IndexRange<1> &range(unsigned dim) const
    {
      assert(dim < N);
      return m_range[dim];
    }

    //! Access range data
    [[nodiscard]] constexpr const IndexRange<1> *range_data() const
    {
      return m_range.range_data();
    }

    //! Inplace clipping of the array bounds.
    //! This clips the array to be within the given range.
    //! Returns false if the resulting array is empty.
    bool clipBy(const IndexRange<N> &range)
    {
      return m_range.clipBy(range);
    }

    //! Is array empty ?
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      return m_range.empty();
    }

    //! Access stride size for given dimension
    [[nodiscard]] constexpr int stride(unsigned dim) const
    {
      return m_strides[dim];
    }

    //! Access strides for each dimension
    [[nodiscard]] constexpr const int *strides() const
    {
      return m_strides.data();
    }

  protected:
    DataT *m_data = nullptr;
    IndexRange<N> m_range;
    std::array<int, N> m_strides {};
  };

  //! Access for an N dimensional element of an array.

  template <typename DataT, unsigned N = 1>
  class Array : public ArrayView<DataT, N>
  {
  public:
    //! Create an empty array
    constexpr Array() = default;

    //! Create an array of the given range.
    explicit constexpr Array(const IndexRange<N> &range)
        : ArrayView<DataT, N>(range),
          m_buffer(std::make_shared<DataT[]>(size_t(range.elements())))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer.get()[this->compute_origin_offset(range)]);
    }

    //! Create an array of the given range.
    explicit constexpr Array(const IndexRange<N> &range, const DataT &initialData)
        : ArrayView<DataT, N>(range),
          m_buffer(std::make_shared<DataT[]>(size_t(range.elements()), initialData))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer.get()[this->compute_origin_offset(range)]);
    }

    //! Construct with an existing buffer
    explicit constexpr Array(DataT *data, const IndexRange<N> &range, const std::array<int, N> &strides, const std::shared_ptr<DataT[]> &buffer)
        : ArrayView<DataT, N>(data, range, strides),
          m_buffer(buffer)
    {}

    //! Construct with an existing buffer
    explicit constexpr Array(DataT *data, const IndexRange<N> &range, const std::array<int, N> &strides, std::shared_ptr<DataT[]> &&buffer)
        : ArrayView<DataT, N>(data, range, strides),
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
    constexpr Array(std::initializer_list<IndexRange<1>> ranges, const DataT &data)
        : ArrayView<DataT, N>(IndexRange<N>(ranges)),
          m_buffer(std::make_shared<DataT[]>(this->m_range.elements(), data))
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
          m_buffer(std::make_shared<DataT[]>(this->m_range.elements(), fillData))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer.get()[this->compute_origin_offset(this->m_range)]);
    }

    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    constexpr Array(Array<DataT, N> &original, const IndexRange<N> &range)
        : ArrayView<DataT, N>(original),
          m_buffer(original.buffer())
    {
      if(!original.range().contains(range))
        throw std::out_of_range("requested range is outside that of the original array");
      this->m_range = range;
      this->m_data = original.origin_address();
    }

    using ArrayView<DataT, N>::begin;
    using ArrayView<DataT, N>::end;

    //! Access buffer.
    [[nodiscard]] constexpr auto &buffer()
    {
      return m_buffer;
    }

    //! Access buffer.
    [[nodiscard]] constexpr const auto &buffer() const
    {
      return m_buffer;
    }

  protected:
    std::shared_ptr<DataT[]> m_buffer;
  };

  //! Take a sub array of the given array.
  //! The range must be entirely contained in the original array and
  //! must exist for the lifetime of the sub array.
  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  constexpr auto clip(const ArrayT &array, const IndexRange<N> &range)
  {
    assert(array.range().contains(range));
    return ArrayAccess<DataT, N>(range, array.origin_address(), array.strides());
  }

  //! Copy data from a source array to destination array
  template <typename Array1T, typename Array2T, unsigned N = Array1T::dimensions>
    requires(WindowedArray<Array1T, typename Array1T::value_type, N> && WindowedArray<Array2T, typename Array2T::value_type, N>) && (N >= 2) && std::is_convertible_v<typename Array2T::value_type, typename Array1T::value_type>
  constexpr void copy(const Array1T &dest, const Array2T &src)
  {
    assert(dest.range().contains(src.range()));
    auto iterDest = dest.begin();
    auto iterSrc = src.begin();
    const auto srcRowSize = src.range(N - 1).size();
    for(; iterDest.valid(); iterDest.nextRow(), iterSrc.nextRow()) {
      std::copy(iterSrc.data(), iterSrc.data() + srcRowSize, iterDest.data());
    }
  }

  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  constexpr auto clone(const ArrayT &array)
  {
    Array<DataT, N> ret(array.range());
    copy(ret, array);
    return ret;
  }

  template <typename ArrayT, typename DataT = typename ArrayT::value_type, typename OtherDataT, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N> && std::is_convertible_v<OtherDataT, DataT> && (N > 1)
  constexpr void fill(const ArrayT &array, const OtherDataT &value)
  {
    if(array.empty())
      return;
    const size_t rowSize = size_t(array.range(N - 1).size());
    // Fill row by row, this allows optimisations such as memset.
    for(auto at = array.begin(); at.valid(); at.nextRow()) {
      std::fill(at.data(), at.data() + rowSize, value);
    }
  }

  //! Get the address of the minimum element of the array.
  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  [[nodiscard]] constexpr DataT *addressOfMin(const ArrayT &arr)
  {
    DataT *mPtr = arr.origin_address();
    for(unsigned i = 0; i < N - 1; i++) {
      mPtr += arr.stride(i) * arr.range(i).min();
    }
    mPtr += arr.range(N - 1).min();// The last index always has a stride of 1.
    return mPtr;
  }

  // Let everyone know there's an implementation already generated for common cases
  extern template class Array<uint8_t, 2>;
  extern template class ArrayAccess<uint8_t, 2>;
  extern template class ArrayIter<uint8_t, 2>;

  extern template class Array<uint16_t, 2>;
  extern template class ArrayAccess<uint16_t, 2>;
  extern template class ArrayIter<uint16_t, 2>;

  extern template class Array<int32_t, 2>;
  extern template class Array<int64_t, 2>;

  extern template class Array<float, 2>;
  extern template class ArrayAccess<float, 2>;
  extern template class ArrayIter<float, 2>;
}// namespace Ravl2
