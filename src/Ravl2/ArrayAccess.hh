/*
 * ArrayAccess.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */

#ifndef ARRAYACCESS_HH_
#define ARRAYACCESS_HH_

#include "Ravl2/RefCounter.hh"
#include "Ravl2/Index.hh"
#include <memory>
#include <vector>

namespace Ravl2
{

  //! Base class for buffer.
  class BufferBase
  {
  public:
    BufferBase(size_t size)
      : m_size(size)
    {}

    //! Virtual destructor
    virtual ~BufferBase()
    {}

    //! Access size of buffer.
    size_t size() const noexcept
    { return m_size; }

  protected:
    size_t m_size;
  };

  template<typename DataT>
  class Buffer
   : public BufferBase
  {
  public:
    Buffer(size_t theSize)
      : BufferBase(theSize)
    {}

    DataT *data()
    { return m_data; }

    const DataT *data() const
    { return m_data; }

  protected:
    DataT *m_data;
  };

  // Buffer based on a vector
  template<typename DataT>
  class BufferVector
   : public Buffer<DataT>
  {
  public:
    BufferVector(std::vector<DataT> &&vec)
      : Buffer<DataT>(0),
        m_values(vec)
    {
      if(this->m_size > 0)
        this->m_data = &m_values[0];
      this->m_size = m_values.size();
    }

#if 1
    BufferVector(const std::vector<DataT> &vec)
      : Buffer<DataT>(0),
        m_values(vec)
    {
      if(this->m_size > 0)
        this->m_data = &m_values[0];
      this->m_size = m_values.size();
    }
#endif


    BufferVector(size_t size)
      : Buffer<DataT>(size),
        m_values(size)
    {
      if(size > 0)
        this->m_data = &m_values[0];
    }

  protected:
    std::vector<DataT> m_values;
  };

  template<typename DataT,unsigned N>
  class ArrayAccessRef;

  //! 1 dimensional access

  template<typename DataT>
  class ArrayAccessRef<DataT,1>
  {
  public:
    ArrayAccessRef(const IndexRange<1> &rng,DataT *data,const int *strides)
     : m_ranges(&rng),
       m_data(data)
    {}

    //! Access indexed element
    DataT &operator[](int i)
    {
      assert((*m_ranges).contains(i));
      return m_data[i];
    }

    //! Access indexed element.
    const DataT &operator[](int i) const
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

    ArrayAccessRef(const IndexRange<N> &rng,DataT *data,const int *strides)
     : m_ranges(&rng[0]),
       m_data(data),
       m_strides(strides)
    {}

    //! Drop index one level.
    ArrayAccessRef<DataT,N-1> operator[](int i)
    {
      assert(m_ranges[0].contains(i));
      DataT *addr = &m_data[m_strides[0] * i];
      return ArrayAccessRef<DataT,N-1>(m_ranges+1,addr,m_strides+1);
    }

    //! Range of index's for row
    const IndexRange<1> &range() const
    { return *m_ranges; }

  protected:
    const IndexRange<1> *m_ranges;
    DataT *m_data;
    const int *m_strides; //! Strides of each dimension of the array.
  };



  //! Access for an N dimensional element of an array.

  template<typename DataT,unsigned N>
  class ArrayAccess
  {
  public:
    void make_strides(const IndexRange<N> &range)
    {
      int s = 1;
      for(unsigned i = N-1;i > 0;--i) {
        m_strides[i] = s;
        s *= range.size(i);
      }
    }

    //! Create an array of the given size.
    ArrayAccess(const IndexRange<N> &range)
     : m_buffer(new BufferVector<DataT>(range.elements()))
    {
      m_data = m_buffer->data();
      make_strides(range);
    }

    //! Access next dimension of array.
    ArrayAccessRef<DataT,N-1> operator[](int i)
    { return ArrayAccessRef<DataT,N-1>(&m_range[1],m_data,&m_strides[1]); }

    //! Access next dimension of array.
    ArrayAccessRef<const DataT,N-1> operator[](int i) const
    { return ArrayAccessRef<DataT,N-1>(&m_range[1],m_data,&m_strides[1]); }

    //! Access next dimension of array.
    DataT &operator[](const Index<N> &ind)
    {
      int dind = 0;
      for(unsigned i = 0;i < N-1;i++) {
        assert(m_range[i].contains(i));
        dind += m_strides[i] * ind.index(i);
      }
      dind += ind.index(N-1);
      return m_data[dind];
    }

    //! Access next dimension of array.
    const DataT &operator[](const Index<1> &ind) const
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
    const IndexRange<1> &range() const
    { return m_range[0]; }

  protected:
    DataT *m_data;
    std::shared_ptr<Buffer<DataT> > m_buffer;
    IndexRange<N> m_range;
    int m_strides[N];
  };

  //! Access for an N dimensional element of an array.

  template<typename DataT>
  class ArrayAccess<DataT,1>
  {
  public:
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
      m_data = m_buffer->data();
    }

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

  protected:
    DataT *m_data;
    std::shared_ptr<Buffer<DataT> > m_buffer;
    IndexRange<1> m_range;
  };

}




#endif /* ARRAYACCESS_HH_ */

