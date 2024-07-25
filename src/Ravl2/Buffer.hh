
/*
 * Array.hh
 *
 *  Created on: Dec 4, 2013
 *      Author: charlesgalambos
 */


#include <memory>
#include <vector>
#include "Ravl2/Index.hh"

namespace Ravl2
{

  //! Concrete base class for buffer.

  class BufferBase
  {
  public:
    explicit BufferBase(size_t size)
      : m_size(size)
    {}

    //! Virtual destructor
    virtual ~BufferBase()
    = default;

    //! Access size of buffer.
    [[nodiscard]] size_t size() const noexcept
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
    explicit Buffer(size_t theSize)
      : BufferBase(theSize)
    {}

    [[nodiscard]] DataT *data()
    { return m_data; }

    [[nodiscard]] const DataT *data() const
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
    explicit BufferVector(std::vector<DataT> &&vec)
      : Buffer<DataT>(vec.size()),
        m_values(std::move(vec))
    {
      if(this->m_size > 0)
        this->m_data = m_values.data();
    }

    //! Copy from a vector.
    explicit BufferVector(const std::vector<DataT> &vec)
      : Buffer<DataT>(vec.size()),
        m_values(vec)
    {
      if(this->m_size > 0)
        this->m_data = m_values.data();
    }

    //! Construct with a given size
    explicit BufferVector(size_t size)
      : Buffer<DataT>(size),
        m_values(this->size())
    {
      if(size > 0)
        this->m_data = m_values.data();
    }

    //! Construct with a given size
    explicit BufferVector(int size)
        : Buffer<DataT>(size > 0 ? size_t(size) : 0),
          m_values(this->size())
    {
      if(size > 0)
        this->m_data = m_values.data();
    }

    //! Construct with a given size
    explicit BufferVector(int bufferSize, const DataT &value)
        : Buffer<DataT>(bufferSize > 0 ? size_t(bufferSize) : 0),
          m_values(this->size(), value)
    {
      if(bufferSize > 0)
        this->m_data = m_values.data();
    }

  protected:
    std::vector<DataT> m_values;
  };

  extern template class BufferVector<uint8_t>;
  extern template class BufferVector<float>;

}

