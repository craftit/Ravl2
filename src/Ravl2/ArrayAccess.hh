/*
 * ArrayAccess.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */

#ifndef RAVL2_ARRAYACCESS_HH_
#define RAVL2_ARRAYACCESS_HH_

#include <array>
#include "Ravl2/RefCounter.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Buffer.hh"

namespace Ravl2
{

  template<typename DataT,unsigned N>
  class ArrayAccessRef;

  //! 1 dimensional access

  template<typename DataT>
  class ArrayAccessRef<DataT,1>
  {
  public:
    ArrayAccessRef(const IndexRange<1> *rng,DataT *data,const int *strides)
     : m_ranges(rng),
       m_data(data)
    {}

    //! Access indexed element
    inline DataT &operator[](int i) noexcept
    {
      assert((*m_ranges).contains(i));
      return m_data[i];
    }

    //! Access indexed element.
    inline const DataT &operator[](int i) const noexcept
    {
      assert((*m_ranges).contains(i));
      return m_data[i];
    }

    //! Range of index's for row
    const IndexRange<1> &range() const
    { return *m_ranges; }

  protected:
    const IndexRange<1> *m_ranges;
    DataT *m_data;
  };

  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N>
  class ArrayAccessRef
  {
  public:
    ArrayAccessRef()
    {}

    ArrayAccessRef(const IndexRange<1> *rng,DataT *data,const int *strides)
     : m_ranges(rng),
       m_data(data),
       m_strides(strides)
    {}

    //! Drop index one level.
    ArrayAccessRef<DataT,N-1> operator[](int i)
    {
      assert(m_ranges[0].contains(i));
      return ArrayAccessRef<DataT,N-1>(m_ranges+1,m_data + (m_strides[0] * i),m_strides +1);
    }

    //! Range of index's for row
    const IndexRange<1> &range() const
    { return *m_ranges; }

    //! Is array empty ?
    bool empty() const noexcept
    {
      if(m_ranges == 0) return true;
      for(int i = 0;i < N;i++)
        if(m_ranges[i].empty())
          return true;
      return false;
    }

  protected:
    const IndexRange<1> *m_ranges {0};
    DataT *m_data {0};
    const int *m_strides {0}; //! Strides of each dimension of the array.
  };



  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N>
  class ArrayAccess
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
  public:
    //! Create an empty array
    ArrayAccess()
    {}

    //! Create an array of the given range.
    ArrayAccess(const IndexRange<N> &range)
     : m_buffer(new BufferVector<DataT>(range.elements())),
       m_range(range)
    {
      make_strides(range);
      m_data = &(m_buffer->data()[compute_origin_offset(range)]);
    }

    //! Create an array from a set of sizes.
    ArrayAccess(std::initializer_list<int> sizes)
     : m_range(sizes)
    {
      make_strides(m_range);
      m_buffer = std::shared_ptr<Buffer<DataT> >(new BufferVector<DataT>(m_range.elements()));
      m_data = &(m_buffer->data()[compute_origin_offset(m_range)]);
    }

    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    ArrayAccess(ArrayAccess<DataT,N> &original,const IndexRange<N> &range)
     : m_buffer(original.buffer()),
       m_range(range)
    {
      if(!original.range().contains(range))
        throw std::out_of_range("requested range is outside that of the original array");
      m_data = original.origin_address();
    }

    //! Access address of origin element
    //! Note: this may not point to a valid element
    DataT *origin_address()
    { return m_data; }

    //! Access address of origin element
    //! Note: this may not point to a valid element
    const DataT *origin_address() const
    { return m_data; }

    //! Access next dimension of array.
    ArrayAccessRef<DataT,N-1> operator[](int i)
    {
      assert(m_range[0].contains(i));
      return ArrayAccessRef<DataT,N-1>(&m_range[1],m_data + i * m_strides[0],&m_strides[1]);
    }

    //! Access next dimension of array.
    ArrayAccessRef<const DataT,N-1> operator[](int i) const
    {
      assert(m_range[0].contains(i));
      return ArrayAccessRef<const DataT,N-1>(&m_range[1],m_data + i * m_strides[0],&m_strides[1]);
    }

    //! Access next dimension of array.
    DataT &operator[](const Index<N> &ind)
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
    const DataT &operator[](const Index<N> &ind) const
    {
      int dind = 0;
      for(unsigned i = 0;i < N-1;i++) {
        assert(m_range[i].contains(i));
        dind += m_strides[i] * ind.index(i);
      }
      dind += ind.index(N-1);
      return m_data[dind];
    }

    //! access range of first index array
    const IndexRange<N> &range() const
    { return m_range; }

    //! Is array empty ?
    bool empty() const noexcept
    { return m_range.empty(); }

    //! Access stride size for given dimension
    int stride(int dim) const
    { return m_strides[dim]; }
  protected:
    DataT *m_data;
    std::shared_ptr<Buffer<DataT> > m_buffer;
    IndexRange<N> m_range;
    std::array<int,N> m_strides;
  };

  //! Access for an N dimensional element of an array.

  template<typename DataT>
  class ArrayAccess<DataT,1>
  {
  public:
    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    ArrayAccess(ArrayAccess<DataT,1> &original,const IndexRange<1> &range)
     : m_buffer(original.buffer()),
       m_range(range)
    {
      if(!original.range().contains(range))
        throw std::out_of_range("requested range is outside that of the original array");
      m_data = original.origin_address();
    }

    ArrayAccess(std::vector<DataT> &&vec)
     : m_buffer(new BufferVector<DataT>(std::move(vec))),
       m_range(m_buffer->size())
    {
      m_data = m_buffer->data();
    }

    ArrayAccess(const std::vector<DataT> &vec)
     : m_buffer(new BufferVector<DataT>(vec)),
       m_range(m_buffer->size())
    {
      m_data = m_buffer->data();
    }

    //! Create an array of the given size.
    ArrayAccess(const IndexRange<1> &range)
     : m_buffer(new BufferVector<DataT>(range.elements())),
       m_range(range)
    {
      m_data = &m_buffer->data()[-range.min()];
    }

    //! Access address of origin element
    //! Note: this may not point to a valid element
    DataT *origin_address()
    { return m_data; }

    //! Access address of origin element
    //! Note: this may not point to a valid element
    const DataT *origin_address() const
    { return m_data; }

    //! Access next dimension of array.
    DataT &operator[](int i)
    {
      assert(m_range.contains(i));
      return m_data[i];
    }

    //! Access next dimension of array.
    const DataT &operator[](int i) const
    {
      assert(m_range.contains(i));
      return m_data[i];
    }

    //! Access next dimension of array.
    DataT &operator[](Index<1> i)
    {
      assert(m_range.contains(i));
      return m_data[i.index(0)];
    }

    //! Access next dimension of array.
    const DataT &operator[](Index<1> i) const
    {
      assert(m_range.contains(i));
      return m_data[i.index(0)];
    }

    //! Access range of array
    const IndexRange<1> &range() const
    { return m_range; }

    //! Is array empty ?
    bool empty() const noexcept
    { return m_range.empty(); }

    //! Access buffer.
    std::shared_ptr<Buffer<DataT> > &buffer()
    { return m_buffer; }

  protected:
    DataT *m_data;
    std::shared_ptr<Buffer<DataT> > m_buffer;
    IndexRange<1> m_range;
  };

}




#endif /* ARRAYACCESS_HH_ */

