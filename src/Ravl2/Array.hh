/*
 * Array.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */

#ifndef RAVL2_ARRAYACCESS_HH_
#define RAVL2_ARRAYACCESS_HH_

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
        : m_at(array)
      {}

      //! Increment iterator
      ArrayIter<DataT,1> &operator++()
      {
          m_at++;
          return *this;
      }

      //! Decrement iterator
      ArrayIter<DataT,1> &operator--()
      {
          m_at--;
          return *this;
      }

      //! Add an offset to the iterator
      ArrayIter<DataT,1> &operator+=(int offset)
      {
          m_at += offset;
          return *this;
      }

      //! Add an offset to the iterator
      ArrayIter<DataT,1> operator-(int offset) const
      {
          return ArrayIter<DataT,1>(m_at - offset);
      }

      //! Add an offset to the iterator
      ArrayIter<DataT,1> operator+(int offset) const
      {
        return ArrayIter<DataT,1>(m_at + offset);
      }

      //! Compare iterators
      bool operator==(const ArrayIter<DataT,1> &other) const
      { return m_at == other.m_at; }

      //! Compare iterators
      bool operator!=(const ArrayIter<DataT,1> &other) const
      { return m_at != other.m_at; }

      //! Compare iterators
      bool operator==(const DataT *other) const
      { return m_at == other; }

      //! Compare iterators
      bool operator!=(const DataT *other) const
      { return m_at != other; }

      //! Access element
      DataT &operator*()
      { return *m_at; }
  private:
      DataT *m_at = nullptr;
  };


  //! Iterator for N dimensional array.

  template<typename DataT,unsigned N>
  class ArrayIter
  {
  public:
    ArrayIter()
    = default;

    ArrayIter(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
    {
      assert(data != nullptr);
      for(int i = 0;i < N;i++) {
        m_at[i] = data + rng[i].min() * strides[i];
        m_end[i] = m_at[i] + rng[i].size() * strides[i];
      }
      m_strides = strides;
    }

    ArrayIter(const IndexRange<N> &rng, DataT *data, const std::array<int,N> &strides) noexcept
     : ArrayIter(rng.ranges().data(),data,strides.data())
    {}

    //! Construct end iterator
    static ArrayIter<DataT,N> end(const IndexRange<N> &rng, DataT *data, const std::array<int,N> &strides) noexcept
    {
      ArrayIter<DataT,N> iter;
      DataT *at = data;
      for(int i = 0;i < N;i++) {
        at += rng.max()[i] * strides[i];
        iter.m_end[i] = at + strides[i];
        iter.m_at[i] = iter.m_end[i];
      }
      iter.m_strides = strides.data();
      return iter;
    }

    //! Construct end iterator
    static ArrayIter<DataT,N> end(const IndexRange<1> *rng, DataT *data, const int *strides) noexcept
    {
      ArrayIter<DataT,N> iter;
      DataT *at = data;
      for(int i = 0;i < N;i++) {
        at += rng[i].max() * strides[i];
        iter.m_end[i] = at + strides[i];
        iter.m_at[i] = iter.m_end[i];
      }
      iter.m_strides = strides;
      return iter;
    }

  protected:
    //! The slower part of going to the next row,
    //! this allows the fast path to be inlined without bloating the code.
    void next_row()
    {
      m_at[N-2] += m_strides[N-2];
      m_at[N-1] = m_at[N-2];
      m_end[N-1] += m_strides[N-2];
      for(int i = N-2;i > 0;i--) {
        if(m_at[i-1] != m_end[i-1]) {
          break;
        }
        m_at[i-1] += m_strides[i-1];
        m_at[i] = m_at[i-1];
        m_end[i] += m_strides[i-1];
      }
    }

  public:
    //! Increment iterator
    inline ArrayIter<DataT,N> &operator++()
    {
      m_at[N-1]++;
      if(m_at[N-1] == m_end[N-1]) {
        next_row();
      }
      return *this;
    }

    //! Increment iterator
    //! Returns true while we're on the same row.
    inline bool next_col()
    {
      m_at[N-1]++;
      if(m_at[N-1] == m_end[N-1]) {
        next_row();
        return false;
      }
      return true;
    }

    [[nodiscard]] bool valid() const
    {
      return m_at[0] != m_end[0];
    }

    //! Compare iterators
    bool operator==(const ArrayIter<DataT,N> &other) const
    {
      // Start at the last dimension and work backwards as it is most likely to be different.
      for(int i = N-1;i >= 0;i--) {
        if(m_at[i] != other.m_at[i])
          return false;
      }
      return true;
    }

    //! Compare iterators
    bool operator!=(const ArrayIter<DataT,N> &other) const
    { return !this->operator==(other); }

    //! Access element
    DataT &operator*() const noexcept
    {
      return *m_at[N-1];
    }

  protected:
     std::array<DataT *,N> m_at {};
     std::array<DataT *,N> m_end {};
     const int *m_strides = nullptr;
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

    ArrayAccess(Array<DataT,1> &array);

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
    [[nodiscard]] ArrayIter<DataT,1> begin() const
    { return ArrayIter<DataT,1>(&m_data[m_ranges->min()]); }

    //! End iterator
    [[nodiscard]] ArrayIter<DataT,1> end() const
    { return ArrayIter<DataT,1>(&m_data[m_ranges->max()+1]); }

  protected:
    const IndexRange<1> *m_ranges;
    DataT *m_data;
  };

  template<typename DataT>
  ArrayAccess<DataT, 1>::ArrayAccess(Array<DataT, 1> &array)
    : m_ranges(array.range().range_data()),
      m_data(array.origin_address())
  {}


  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N>
  class ArrayAccess
  {
  public:
    ArrayAccess()
    = default;

    ArrayAccess(const IndexRange<1> *rng, DataT *data, const int *strides)
     : m_ranges(rng),
       m_data(data),
       m_strides(strides)
    {}

    explicit ArrayAccess(Array<DataT,N> &array);

      //! Drop index one level.
    [[nodiscard]] ArrayAccess<DataT, N - 1> operator[](int i)
    {
      assert(m_ranges[0].contains(i));
      return ArrayAccess<DataT, N - 1>(m_ranges + 1, m_data + (m_strides[0] * i), m_strides + 1);
    }

    //! Range of index's for row
    [[nodiscard]] const IndexRange<1> &range1() const
    { return *m_ranges; }

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
    [[nodiscard]] ArrayIter<DataT,N> begin() const
    { return ArrayIter<DataT,N>(m_ranges, m_data, m_strides); }

    //! End iterator
    [[nodiscard]] ArrayIter<DataT,N> end() const
    { return ArrayIter<DataT,N>::end(m_ranges, m_data, m_strides); }

  protected:
    const IndexRange<1> *m_ranges {nullptr};
    DataT *m_data {nullptr};
    const int *m_strides {nullptr}; //! Strides of each dimension of the array.
  };


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
        return ArrayView<DataT,N>(range,m_data,m_strides);
      }

      //! Get a view of the array with the requested 'range'
      ArrayView<const DataT,N> view(const IndexRange<N> &range) const
      {
        assert(m_range.contains(range));
        return ArrayView<const DataT,N>(m_data,range,m_strides);
      }

      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] DataT *origin_address()
      { return m_data; }

      //! Access address of origin element
      //! Note: this may not point to a valid element
      [[nodiscard]] const DataT *origin_address() const
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


#endif /* RAVL2_ARRAYACCESS_HH_ */

