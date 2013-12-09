
/*
 * ArrayAccess.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */

#ifndef RAVL2_BUFFER_HH_
#define RAVL2_BUFFER_HH_

#include "Ravl2/Index.hh"
#include <memory>
#include <vector>

namespace Ravl2
{

  //! Concrete base class for buffer.

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

  //! Buffer with pointer to data.

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
    DataT *m_data {0};
  };


  //! Buffer based on a stl vector

  template<typename DataT>
  class BufferVector
   : public Buffer<DataT>
  {
  public:
    //! Move from a vector.
    BufferVector(std::vector<DataT> &&vec)
      : Buffer<DataT>(0),
        m_values(std::move(vec))
    {
      if(this->m_size > 0)
        this->m_data = &m_values[0];
      this->m_size = m_values.size();
    }

    //! Copy from a vector.
    BufferVector(const std::vector<DataT> &vec)
      : Buffer<DataT>(0),
        m_values(vec)
    {
      if(this->m_size > 0)
        this->m_data = &m_values[0];
      this->m_size = m_values.size();
    }

    //! Construct with a given size
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

}

#endif
