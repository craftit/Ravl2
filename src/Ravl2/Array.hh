/*
 * Array.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */

#pragma once

#include <array>
#include "Ravl2/Index.hh"
#include "Ravl2/Buffer.hh"

namespace Ravl2
{
  template<typename DataT,unsigned N>
  class Array;

  template<typename DataT,unsigned N>
  class ArrayAccess;

  template<typename DataT,unsigned N>
  class ArrayIter;

  //! Iterator for 1 dimensional array.

  template<typename DataT>
  class ArrayIter<DataT,1>
  {
  public:
      explicit ArrayIter(DataT *array)
        : mPtr(array)
      {}

      //! Increment iterator
      ArrayIter<DataT,1> &operator++()
      {
          mPtr++;
          return *this;
      }

      //! Decrement iterator
      ArrayIter<DataT,1> &operator--()
      {
          mPtr--;
          return *this;
      }

      //! Add an offset to the iterator
      ArrayIter<DataT,1> &operator+=(int offset)
      {
	mPtr += offset;
          return *this;
      }

      //! Add an offset to the iterator
      ArrayIter<DataT,1> operator-(int offset) const
      {
          return ArrayIter<DataT,1>(mPtr - offset);
      }

      //! Add an offset to the iterator
      ArrayIter<DataT,1> operator+(int offset) const
      {
        return ArrayIter<DataT,1>(mPtr + offset);
      }

      //! Compare iterators
      bool operator==(const ArrayIter<DataT,1> &other) const
      { return mPtr == other.mPtr; }

      //! Compare iterators
      bool operator!=(const ArrayIter<DataT,1> &other) const
      { return mPtr != other.mPtr; }

      //! Compare iterators
      bool operator==(const DataT *other) const
      { return mPtr == other; }

      //! Compare iterators
      bool operator!=(const DataT *other) const
      { return mPtr != other; }

      //! Access element
      DataT &operator*() const
      { return *mPtr; }
  private:
      DataT *mPtr = nullptr;
  };

  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N>
  class ArrayAccess
  {
  public:
    ArrayAccess()
    = default;

    ArrayAccess(const IndexRange<N> &rng, DataT *data, const int *strides)
      : m_ranges(rng.range_data()),
	m_data(data),
	m_strides(strides)
    {}

    ArrayAccess(const IndexRange<1> *rng, DataT *data, const int *strides)
      : m_ranges(rng),
	m_data(data),
	m_strides(strides)
    {}

    explicit ArrayAccess(Array<DataT,N> &array);

    //! Access origin address
    [[nodiscard]] DataT *origin_address()
    { return m_data; }

    //! Access origin address
    [[nodiscard]] const DataT *origin_address() const
    { return m_data; }

    //! Drop index one level.
    [[nodiscard]] ArrayAccess<DataT, N - 1> operator[](int i)
    {
      assert(m_ranges[0].contains(i));
      return ArrayAccess<DataT, N - 1>(m_ranges + 1, m_data + (m_strides[0] * i), m_strides + 1);
    }

    //! Drop index one level.
    [[nodiscard]] DataT &operator[](const Index<N> &ind)
    {
      DataT *mPtr = m_data;
      for(int i = 0;i < N;i++)
      {
	assert(m_ranges[i].contains(ind[i]));
	mPtr += m_strides[i] * ind[i];
      }
      return *mPtr;
    }

    //! Range of index's for row
    [[nodiscard]] const IndexRange<1> &range1() const
    { return *m_ranges; }

    //! Range of index's for dim
    [[nodiscard]] const IndexRange<1> &range(unsigned i) const
    {
      assert(i < N);
      return m_ranges[i];
    }

    //! Range of index's for row
    [[nodiscard]] IndexRange<N> range() const
    {
      IndexRange<N> rng;
      for(int i = 0;i < N;i++)
	rng[i] = m_ranges[i];
      return rng;
    }

    //! Is array empty ?
    [[nodiscard]] bool empty() const noexcept
    {
      if(m_ranges == nullptr) return true;
      for(int i = 0;i < N;i++)
	if(m_ranges[i].empty())
	  return true;
      return false;
    }

    //! Begin iterator
    [[nodiscard]] ArrayIter<DataT,N> begin() const;

    //! End iterator
    [[nodiscard]] ArrayIter<DataT,N> end() const;

    //! Get stride for dimension
    [[nodiscard]] int stride(int dim) const
    { return m_strides[dim]; }

    //! Access strides
    [[nodiscard]] const int *strides() const
    { return m_strides; }
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
    ArrayIter()
    = default;

    ArrayIter(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
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

    ArrayIter(const IndexRange<N> &rng, DataT *data, const int *strides) noexcept
     : ArrayIter(rng.ranges().data(),data,strides)
    {}

    ArrayIter(const IndexRange<N> &rng, DataT *data, const std::array<int,N> &strides) noexcept
     : ArrayIter(rng.ranges().data(),data,strides.data())
    {}

    //! Construct end iterator
    static ArrayIter<DataT,N> end(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
    {
      ArrayIter<DataT,N> iter(rng,data,strides);
      iter.mPtr += strides[0] * rng[0].size();
      iter.mEnd = iter.mPtr;
      return iter;
    }

    //! Construct end iterator
    static ArrayIter<DataT,N> end(const IndexRange<N> &rng, DataT *data, const std::array<int,N> &strides) noexcept
    {
      return end(rng.ranges().data(),data,strides.data());
    }

  protected:
    void next_ptr()
    {
      for(unsigned i = N-2;i > 0;--i) {
	++mIndex[i];
	if(mIndex[i] <= m_access.range(i).max())
	  goto done;
	mIndex[i] = m_access.range(i).min();
      }
      // On the last index we don't need to update
      ++mIndex[0];
    done:
      mPtr = m_access.origin_address();
      for(int i = 0;i < N;i++)
      {
	mPtr += m_access.stride(i) * mIndex[i];
      }
      mEnd = mPtr + m_access.strides()[N-1] * m_access.range(N-1).size();
    }

  public:
    //! Increment iterator
    inline ArrayIter<DataT,N> &operator++()
    {
      mPtr++;
      if(mPtr == mEnd) {
        next_ptr();
      }
      return *this;
    }

    //! Increment iterator
    //! Returns true while we're on the same row.
    inline bool next_col()
    {
      mPtr++;
      if(mPtr == mEnd) {
	next_ptr();
        return false;
      }
      return true;
    }

    //! Test if the iterator is valid.
    [[nodiscard]] bool valid() const noexcept
    { return mIndex[0] <= m_access.range(0).max();  }

    //! Test if the iterator is finished.
    [[nodiscard]] bool done() const noexcept
    { return mIndex[0] > m_access.range(0).max();  }

    //! Compare iterators
    bool operator==(const ArrayIter<DataT,N> &other) const
    { return (mPtr == other.mPtr); }

    //! Compare iterators
    bool operator!=(const ArrayIter<DataT,N> &other) const
    { return !this->operator==(other); }

    //! Access element
    DataT &operator*() const noexcept
    { return *mPtr; }

    //! Access strides
    [[nodiscard]] const int *strides() const
    { return m_access.strides(); }
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
    ArrayAccess(const IndexRange<1> *rng, DataT *data, [[maybe_unused]] const int *strides)
     : m_ranges(rng),
       m_data(data)
    {
      assert(*strides == 1);
    }

    ArrayAccess(const IndexRange<1> *rng, DataT *data)
      : m_ranges(rng),
	m_data(data)
    {}

    explicit ArrayAccess(Array<DataT,1> &array);

    //! Access origin address
    [[nodiscard]] DataT *origin_address()
    { return m_data; }

    //! Access origin address
    [[nodiscard]] const DataT *origin_address() const
    { return m_data; }

    //! Access indexed element
    [[nodiscard]] inline DataT &operator[](int i) noexcept
    {
      assert((*m_ranges).contains(i));
      return m_data[i];
    }

    //! Access indexed element.
    [[nodiscard]] inline const DataT &operator[](int i) const noexcept
    {
      assert((*m_ranges).contains(i));
      return m_data[i];
    }

    //! Range of index's for row
    [[nodiscard]] const IndexRange<1> &range() const
    { return *m_ranges; }

    //! Is array empty ?
    [[nodiscard]] bool empty() const noexcept
    { return m_ranges->empty(); }

    //! Begin iterator
    [[nodiscard]] ArrayIter<DataT,1> begin() const;

    //! End iterator
    [[nodiscard]] ArrayIter<DataT,1> end() const;

  protected:
    const IndexRange<1> *m_ranges;
    DataT *m_data;
  };

  template<typename DataT>
  ArrayIter<DataT, 1>
  ArrayAccess<DataT, 1>::begin() const { return ArrayIter<DataT,1>(&m_data[m_ranges->min()]); }

  template<typename DataT>
  ArrayAccess<DataT, 1>::ArrayAccess(Array<DataT, 1> &array)
    : m_ranges(array.range().range_data()),
      m_data(array.origin_address())
  {}

  template<typename DataT, unsigned int N>
  ArrayIter<DataT, N>
  ArrayAccess<DataT, N>::end() const { return ArrayIter<DataT,N>::end(m_ranges, m_data, m_strides); }

  template<typename DataT, unsigned int N>
  ArrayIter<DataT, N>
  ArrayAccess<DataT, N>::begin() const { return ArrayIter<DataT,N>(m_ranges, m_data, m_strides); }

  template<typename DataT>
  ArrayIter<DataT, 1>
  ArrayAccess<DataT, 1>::end() const { return ArrayIter<DataT,1>(&m_data[m_ranges->max()+1]); }

  template<typename DataT, unsigned int N>
  ArrayAccess<DataT, N>::ArrayAccess(Array<DataT, N> &array)
    : m_ranges(&array.range().ranges()),
      m_data(array.origin_address()),
      m_strides(array.strides().data())
  {
  }

  //! Non-owning view of an array values.

  template<typename DataT,unsigned N=1>
  class ArrayView
  {
  protected:
      //! Generate strides
      void make_strides(const IndexRange<N> &range)
      {
        int s = 1;
        for(int i = N-1;i >= 0;--i) {
          //std::cout << " " << i << " s=" << s << "\n";
          m_strides[i] = s;
          s *= range.size(i);
        }
      }

      int compute_origin_offset(const IndexRange<N> &range)
      {
        int off = 0;
        for(unsigned i = 0;i < N;i++) {
          off -= range[i].min() * m_strides[i];
        }
        return off;
      }

      explicit ArrayView(const IndexRange<N> &range, const std::array<int,N> &strides)
        : m_range(range),
          m_strides(strides)
      {}

      explicit ArrayView(const IndexRange<N> &range)
        : m_range(range)
      {}


  public:
      explicit ArrayView(DataT *data, const IndexRange<N> &range, const std::array<int,N> &strides)
        : m_data(data),
          m_range(range),
          m_strides(strides)
      {}

      //! Create an empty array
      ArrayView()
      = default;

      //! Create an sub array with the requested 'range'
      //! Range must be entirely contained in the original array.
      ArrayView(ArrayView<DataT,N> &original, const IndexRange<N> &range)
          : m_range(range)
      {
        if(!original.range().contains(range))
          throw std::out_of_range("requested range is outside that of the original array");
        m_data = original.origin_address();
      }

      //! Create an sub array with the requested 'range'
      //! Range must be entirely contained in the original array.
      ArrayView(const ArrayView<const DataT,N> &original, const IndexRange<N> &range)
              : m_range(range)
      {
        if(!original.range().contains(range))
          throw std::out_of_range("requested range is outside that of the original array");
        m_data = original.origin_address();
      }

      //! Get a view of the array with the requested 'range'
      ArrayView<DataT,N> view(const IndexRange<N> &range)
      {
        return ArrayView<DataT,N>(m_data, range, m_strides);
      }

      //! Get a view of the array with the requested 'range'
      ArrayView<const DataT,N> view(const IndexRange<N> &range) const
      {
        assert(m_range.contains(range));
	assert(m_data != nullptr);
        return ArrayView<const DataT,N>(m_data,range,m_strides);
      }

      //! Get an access of the array with the requested 'range'
      ArrayAccess<DataT,N> access(const IndexRange<N> &range)
      {
	assert(m_range.contains(range));
	assert(m_data != nullptr);
	return ArrayAccess<DataT,N>(range.range_data(),m_data,m_strides.data());
      }

      //! Get an access of the array with the requested 'range'
      ArrayAccess<const DataT,N> access(const IndexRange<N> &range) const
      {
        assert(m_range.contains(range));
	assert(m_data != nullptr);
        return ArrayAccess<const DataT,N>(range.range_data(),m_data,m_strides.data());
      }

      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] DataT *origin_address()
      { return m_data; }

      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] DataT *origin_address() const
      { return m_data; }

      //! Access next dimension of array.
      [[nodiscard]] ArrayAccess<DataT, N - 1> operator[](int i)
      {
        assert(m_range[0].contains(i));
        return ArrayAccess<DataT, N - 1>(&m_range[1], m_data + i * m_strides[0], &m_strides[1]);
      }

      //! Access next dimension of array.
      [[nodiscard]] ArrayAccess<const DataT, N - 1> operator[](int i) const
      {
        assert(m_range[0].contains(i));
        return ArrayAccess<const DataT, N - 1>(&m_range[1], m_data + i * m_strides[0], &m_strides[1]);
      }

      //! Access next dimension of array.
      [[nodiscard]] DataT &operator[](const Index<N> &ind)
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
      [[nodiscard]] const DataT &operator[](const Index<N> &ind) const
      {
        int dind = 0;
        for(unsigned i = 0;i < N-1;i++) {
          assert(m_range[i].contains(i));
          dind += m_strides[i] * ind.index(i);
        }
        dind += ind.index(N-1);
        return m_data[dind];
      }

      //! Get begin iterator
      ArrayIter<DataT,N> begin() const
      {
        assert(m_data != nullptr);
        return ArrayIter<DataT,N>(m_range, m_data, m_strides);
      }

      //! Get end iterator
      ArrayIter<DataT,N> end() const
      {
        assert(m_data != nullptr);
        return ArrayIter<DataT,N>::end(m_range, m_data, m_strides);
      }

      //! access range of first index array
      [[nodiscard]] const IndexRange<N> &range() const
      { return m_range; }

      //! Is array empty ?
      [[nodiscard]] bool empty() const noexcept
      { return m_range.empty(); }

      //! Access stride size for given dimension
      [[nodiscard]] int stride(int dim) const
      { return m_strides[dim]; }

      //! Access strides for each dimension
      [[nodiscard]] const int *strides() const
      { return m_strides.data(); }

      //! Fill array with value.
      void fill(DataT val)
      {
        for(auto at : *this)
          at = val;
      }

  protected:
      DataT *m_data = nullptr;
      IndexRange<N> m_range;
      std::array<int,N> m_strides;
  };


  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N=1>
  class Array
    : public ArrayView<DataT,N>
  {
  public:
    //! Create an empty array
    Array()
    = default;

    //! Create an array of the given range.
    explicit Array(const IndexRange<N> &range)
     : ArrayView<DataT, N>(range),
       m_buffer(new BufferVector<DataT>(range.elements()))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer->data()[this->compute_origin_offset(range)]);
    }

    //! Create an array from a set of sizes.
    Array(std::initializer_list<IndexRange<N>> ranges)
      : ArrayView<DataT, N>(IndexRange<N>(ranges)),
        m_buffer(std::make_shared<BufferVector<DataT> >(this->m_range.elements()))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer->data()[this->compute_origin_offset(this->m_range)]);
    }


      //! Create an array from a set of sizes.
    Array(std::initializer_list<int> sizes)
     : ArrayView<DataT, N>(IndexRange<N>(sizes)),
       m_buffer(std::make_shared<BufferVector<DataT> >(this->m_range.elements()))
    {
      this->make_strides(this->m_range);
      this->m_data = &(m_buffer->data()[this->compute_origin_offset(this->m_range)]);
    }

    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    Array(Array<DataT,N> &original, const IndexRange<N> &range)
     : ArrayView<DataT, N>(original),
       m_buffer(original.buffer())
    {
      if(!original.range().contains(range))
        throw std::out_of_range("requested range is outside that of the original array");
      this->m_range = range;
      this->m_data = original.origin_address();
    }

    //! Access buffer.
    [[nodiscard]] std::shared_ptr<Buffer<DataT> > &buffer()
    { return m_buffer; }
  protected:
    std::shared_ptr<Buffer<DataT> > m_buffer;
  };


  //! Access for an N dimensional element of an array.

  template<typename DataT>
  class ArrayView<DataT,1>
  {
  public:
      //! Create an sub array with the requested 'range'
      //! Range must be entirely contained in the original array.
      ArrayView(Array<DataT,1> &original, const IndexRange<1> &range)
         : m_range(range)
      {
        if(!original.range().contains(range))
          throw std::out_of_range("requested range is outside that of the original array");
        m_data = original.origin_address();
      }

      explicit ArrayView(const std::vector<DataT> &vec)
          : m_range(vec.size())
      {
        m_data = vec.data();
      }

  protected:
      //! Create an array of the given size.
      explicit ArrayView(const IndexRange<1> &range)
        : m_range(range)
      {}

  public:

      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] DataT *origin_address()
      { return m_data; }

      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] const DataT *origin_address() const
      { return m_data; }


      //! Access next dimension of array.
      [[nodiscard]] DataT &operator[](int i)
      {
        assert(m_range.contains(i));
        return m_data[i];
      }

      //! Access next dimension of array.
      [[nodiscard]] const DataT &operator[](int i) const
      {
        assert(m_range.contains(i));
        return m_data[i];
      }

      //! Access next dimension of array.
      [[nodiscard]] DataT &operator[](Index<1> i)
      {
        assert(m_range.contains(i));
        return m_data[i.index(0)];
      }

      //! Access next dimension of array.
      [[nodiscard]] const DataT &operator[](Index<1> i) const
      {
        assert(m_range.contains(i));
        return m_data[i.index(0)];
      }

      //! Access range of array
      [[nodiscard]] const IndexRange<1> &range() const
      { return m_range; }

      //! Is array empty ?
      [[nodiscard]] bool empty() const noexcept
      { return m_range.empty(); }

      //! Begin iterator
      ArrayIter<DataT,1> begin() const
      { return ArrayIter<DataT,1>(m_data); }

      //! End iterator
      ArrayIter<DataT,1> end() const
      { return ArrayIter<DataT,1>(m_data + m_range.size()); }

      //! Get the index of the given pointer within the array.
      [[nodiscard]]
      Index<1> indexOf(const DataT *ptr) const
      {
        return Index<1>({int(ptr - m_data)});
      }

      //! Fill array with value.
      void fill(const DataT &val)
      {
        for(auto at :*this)
          *at = val;
      }

  protected:
      DataT *m_data;
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
    Array(Array<DataT,1> &original, const IndexRange<1> &range)
     : ArrayView<DataT, 1>(original, range),
       m_buffer(original.buffer())
    {}

    explicit Array(std::vector<DataT> &&vec)
     : ArrayView<DataT, 1>(IndexRange<1>(vec.size())),
       m_buffer(new BufferVector<DataT>(std::move(vec)))
    {
      this->m_data = m_buffer->data();
    }

    explicit Array(const std::vector<DataT> &vec)
     : ArrayView<DataT, 1>(IndexRange<1>(vec.size())),
       m_buffer(new BufferVector<DataT>(vec))
    {
      this->m_data = m_buffer->data();
    }

    //! Create an array of the given size.
    explicit Array(const IndexRange<1> &range)
     : ArrayView<DataT, 1>(range),
       m_buffer(new BufferVector<DataT>(range.elements()))
    {
      this->m_data = &m_buffer->data()[-range.min()];
    }

    //! Access buffer.
    [[nodiscard]] std::shared_ptr<Buffer<DataT> > &buffer()
    { return m_buffer; }

    //! Begin iterator
  protected:
    std::shared_ptr<Buffer<DataT> > m_buffer;
  };

}


