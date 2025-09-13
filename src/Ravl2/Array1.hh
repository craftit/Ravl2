//
// Created by charles on 07/08/24.
//

#pragma once

#include <array>
#include <cereal/cereal.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Concepts.hh"
#include "Ravl2/Sentinel.hh"
#include "Ravl2/IndexRange1.hh"

namespace Ravl2
{
  //! Global stride for 1 dimensional arrays. Always has a value of 1.
  extern const int gStride1;

  template <typename DataT, unsigned N>
  class Array;

  template <typename DataT, unsigned N>
  class ArrayAccess;

  template <typename DataT, unsigned N>
  class ArrayView;

  template <typename DataT, unsigned N>
  class ArrayIter;

  //! Iterator for 1 dimensional array.

  template <typename DataT>
  class ArrayIter<DataT, 1>
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = DataT;
    constexpr static unsigned dimensions = 1;

    constexpr explicit ArrayIter() = default;

    constexpr explicit ArrayIter(DataT *array)
        : mPtr(array)
    {}

    constexpr explicit ArrayIter(DataT *array, [[maybe_unused]] IndexRange<1> &rng)
        : mPtr(array)
    {}

    //! Increment iterator
    constexpr ArrayIter<DataT, 1> &operator++()
    {
      mPtr++;
      return *this;
    }

    //! Post Increment iterator
    constexpr ArrayIter<DataT, 1> operator++(int)
    {
      ArrayIter<DataT, 1> iter = *this;
      mPtr++;
      return iter;
    }

    //! Decrement iterator
    constexpr ArrayIter<DataT, 1> &operator--()
    {
      mPtr--;
      return *this;
    }

    //! Decrement iterator
    constexpr ArrayIter<DataT, 1> operator--(int)
    {
      ArrayIter<DataT, 1> iter = *this;
      mPtr--;
      return iter;
    }

    //! Add an offset to the iterator
    template <typename IntegerT>
      requires std::is_integral_v<IntegerT>
    constexpr ArrayIter<DataT, 1> &operator+=(IntegerT offset)
    {
      mPtr += offset;
      return *this;
    }

    //! Add an offset to the iterator
    template <typename IntegerT>
      requires std::is_integral_v<IntegerT>
    constexpr ArrayIter<DataT, 1> &operator-=(IntegerT offset)
    {
      mPtr -= offset;
      return *this;
    }

    //! Spaceship operator
    [[nodiscard]] constexpr auto operator<=>(const ArrayIter<DataT, 1> &other) const
    {
      return mPtr <=> other.mPtr;
    }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator==(const ArrayIter<DataT, 1> &other) const
    {
      return mPtr == other.mPtr;
    }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator!=(const ArrayIter<DataT, 1> &other) const
    {
      return mPtr != other.mPtr;
    }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator==(const DataT *other) const
    {
      return mPtr == other;
    }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator!=(const DataT *other) const
    {
      return mPtr != other;
    }

    //! Access element
    [[nodiscard]] constexpr DataT &operator*() const
    {
      return *mPtr;
    }

    //! Index access
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    [[nodiscard]] constexpr DataT &operator[](int i) const
    {
      return mPtr[i];
    }

    //! Index access
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template<IndexType IndexT>
    [[nodiscard]] constexpr DataT &operator()(IndexT i) const
    {
      return mPtr[int(i)];
    }

    //! Index access
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template<IndexType IndexT>
    [[nodiscard]] constexpr inline const auto &operator()(IndexT i) const
    {
      return mPtr[int(i)];
    }

    //! Access relative element
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template<IndexType IndexT>
    [[nodiscard]] constexpr inline const auto &at(IndexT i) const
    {
      return mPtr[int(i)];
    }

    //! Access relative element
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template<IndexType IndexT>
    [[nodiscard]] constexpr inline auto &at(IndexT i)
    {
      return mPtr[int(i)];
    }

    //! Access element point
    [[nodiscard]] constexpr DataT *data() const
    {
      return mPtr;
    }

  private:
    DataT *mPtr = nullptr;
  };

  //! Add to the iterator
  template <typename DataT, typename IntegerT>
    requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT, 1> operator+(const ArrayIter<DataT, 1> &it, IntegerT offset)
  {
    return ArrayIter<DataT, 1>(it.data() + offset);
  }

  //! Subtract from the iterator
  template <typename DataT, typename IntegerT>
    requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT, 1> operator-(const ArrayIter<DataT, 1> &it, IntegerT offset)
  {
    return ArrayIter<DataT, 1>(it.data() + offset);
  }

  //! Add to the iterator
  template <typename DataT, typename IntegerT>
    requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT, 1> operator+(IntegerT offset, const ArrayIter<DataT, 1> &it)
  {
    return ArrayIter<DataT, 1>(offset + it.data());
  }

  //! Subtract from the iterator
  template <typename DataT, typename IntegerT>
    requires std::is_integral_v<IntegerT>
  [[nodiscard]] constexpr ArrayIter<DataT, 1> operator-(IntegerT offset, const ArrayIter<DataT, 1> &it)
  {
    return ArrayIter<DataT, 1>(offset + it.data());
  }

  //! Add an offset to the iterator
  template <typename DataT>
  [[nodiscard]] constexpr typename ArrayIter<DataT, 1>::difference_type operator-(const ArrayIter<DataT, 1> &o1, const ArrayIter<DataT, 1> &o2)
  {
    return o1.data() - o2.data();
  }

  //! 1 dimensional access

  template <typename DataT>
  class ArrayAccess<DataT, 1>
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimensions = 1;
    constexpr ArrayAccess() = default;

    constexpr ArrayAccess(DataT *data, const IndexRange<1> *rng, [[maybe_unused]] const int *strides)
        : m_ranges(rng),
          m_data(data)
    {
      assert(*strides == 1);
    }

    constexpr ArrayAccess(DataT *data, const IndexRange<1> &rng, [[maybe_unused]] const int *strides)
        : m_ranges(&rng),
          m_data(data)
    {
      assert(*strides == 1);
    }

    constexpr ArrayAccess(const IndexRange<1> *rng, DataT *data)
        : m_ranges(rng),
          m_data(data)
    {}

    template <typename ArrayT>
      requires WindowedArray<ArrayT, DataT, 1>
    explicit constexpr ArrayAccess(ArrayT &array)
        : m_ranges(array.range().range_data()),
          m_data(array.origin_address())
    {}

    //! Access origin address
    [[nodiscard]] constexpr DataT *origin_address() const
    {
      return m_data;
    }

    //! Access indexed element
    [[nodiscard]] inline constexpr DataT &operator[](int i) noexcept
    {
      assert(m_ranges != nullptr);
      assert(m_ranges->contains(i));
      return m_data[i];
    }

    //! Access indexed element
    [[nodiscard]] inline constexpr DataT &operator[](Index<1> i) noexcept
    {
      assert(m_ranges != nullptr);
      assert(m_ranges->contains(i));
      return m_data[i.index(0)];
    }

    //! Access indexed element.
    [[nodiscard]] inline constexpr const DataT &operator[](int i) const noexcept
    {
      assert(m_ranges != nullptr);
      assert(m_ranges->contains(i));
      return m_data[i];
    }

    //! Access indexed element.
    [[nodiscard]] inline constexpr const DataT &operator[](Index<1> i) const noexcept
    {
      assert(m_ranges != nullptr);
      assert(m_ranges->contains(i));
      return m_data[i.index(0)];
    }

    //! Access indexed element.
    [[nodiscard]] inline constexpr DataT &operator()(int i) noexcept
    {
      assert(m_ranges != nullptr);
      assert(m_ranges->contains(i));
      return m_data[i];
    }

    //! Access indexed element.
    [[nodiscard]] inline constexpr const DataT &operator()(int i) const noexcept
    {
      assert(m_ranges != nullptr);
      assert(m_ranges->contains(i));
      return m_data[i];
    }

    //! Range of index's for row
    [[nodiscard]] constexpr const IndexRange<1> &range() const
    {
      assert(m_ranges != nullptr);
      return *m_ranges;
    }

    //! Range of index's for dim
    [[nodiscard]] constexpr const IndexRange<1> &range([[maybe_unused]] unsigned i) const
    {
      assert(m_ranges != nullptr);
      assert(i == 0);
      return *m_ranges;
    }

    //! Access range data
    [[nodiscard]] constexpr const IndexRange<1> *range_data() const
    {
      assert(m_ranges != nullptr);
      return m_ranges;
    }

    //! Is array empty ?
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      return m_ranges->empty();
    }

    //! vertexBegin iterator
    [[nodiscard]] constexpr ArrayIter<DataT, 1> begin() const;

    //! vertexEnd iterator
    [[nodiscard]] constexpr ArrayIter<DataT, 1> end() const;

    //! vertexBegin iterator
    [[nodiscard]] constexpr ArrayIter<const DataT, 1> cbegin() const;

    //! vertexEnd iterator
    [[nodiscard]] constexpr ArrayIter<const DataT, 1> cend() const;

    //! Get stride for dimension
    [[nodiscard]] constexpr static int stride([[maybe_unused]] unsigned dim)
    {
      return 1;
    }

    //! Access strides
    [[nodiscard]] constexpr static const int *strides()
    {
      return &gStride1;
    }

#if 0
    //! Copy data from another array
    //! Experimental
    template<typename ArrayT,typename OtherDataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT,OtherDataT,1> && std::is_convertible_v<OtherDataT,DataT>
    auto &operator=(const ArrayT &array)
    {
      assert(array.range().size() == m_ranges->size());
      SPDLOG_INFO("Copy data from another array {} ", array.range().size());
      std::copy(array.begin(),array.end(),begin());
      return *this;
    }
#endif
    //! Broadcast assignment
    //! This fills the array with the given value.
    auto &operator=(const DataT &value)
    {
      fill(*this, value);
      return *this;
    }

  protected:
    const IndexRange<1> *m_ranges = nullptr;
    DataT *m_data = nullptr;
  };

  template <typename DataT>
  constexpr ArrayIter<DataT, 1> ArrayAccess<DataT, 1>::begin() const
  {
    return ArrayIter<DataT, 1>(&m_data[m_ranges->min()]);
  }

  template <typename DataT>
  constexpr ArrayIter<DataT, 1> ArrayAccess<DataT, 1>::end() const
  {
    return ArrayIter<DataT, 1>(&m_data[m_ranges->max() + 1]);
  }

  template <typename DataT>
  constexpr ArrayIter<const DataT, 1> ArrayAccess<DataT, 1>::cbegin() const
  {
    return ArrayIter<const DataT, 1>(&m_data[m_ranges->min()]);
  }

  template <typename DataT>
  constexpr ArrayIter<const DataT, 1> ArrayAccess<DataT, 1>::cend() const
  {
    return ArrayIter<const DataT, 1>(&m_data[m_ranges->max() + 1]);
  }

  //! Access for an N dimensional element of an array.

  template <typename DataT>
  class ArrayView<DataT, 1>
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimensions = 1;

    //! Given strides and range compute the offset to the origin from the start of the data.
    [[nodiscard]] static constexpr int compute_origin_offset(const IndexRange<1> &range)
    {
      return - range.min();
    }

    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    constexpr ArrayView(Array<DataT, 1> &original, const IndexRange<1> &range)
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

    explicit constexpr ArrayView(DataT *data, const IndexRange<1> &range, [[maybe_unused]] const int *strides)
        : m_data(data),
          m_range(range)
    {
      assert(*strides == 1);
    }

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
    {
      return m_data;
    }

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

    //! Access next dimension of array.
    template <IndexType IndexT>
    [[nodiscard]] constexpr DataT &operator()(IndexT i)
    {
      assert(m_range.contains(int(i)));
      return m_data[i];
    }

    //! Access next dimension of array.
    template <IndexType IndexT>
    [[nodiscard]] constexpr const DataT &operator()(IndexT i) const
    {
      assert(m_range.contains(int(i)));
      return m_data[i];
    }

    //! Access range of array
    [[nodiscard]] constexpr const IndexRange<1> &range() const
    {
      return m_range;
    }

    //! Access range of array
    [[nodiscard]] constexpr const IndexRange<1> &range([[maybe_unused]] unsigned n) const
    {
      assert(n == 0);
      return m_range;
    }

    //! Inplace clipping of the array bounds.
    //! This clips the array to be within the given range.
    //! Returns false if the resulting array is empty.
    bool clipBy(const IndexRange<1> &range)
    {
      return m_range.clipBy(range);
    }

    //! Is array empty ?
    [[nodiscard]] constexpr bool empty() const noexcept
    {
      return m_range.empty();
    }

    //! vertexBegin iterator
    [[nodiscard]] constexpr ArrayIter<DataT, 1> begin()
    {
      return ArrayIter<DataT, 1>(m_data);
    }

    //! vertexEnd iterator
    [[nodiscard]] constexpr ArrayIter<DataT, 1> end()
    {
      return ArrayIter<DataT, 1>(m_data + m_range.size());
    }

    //! vertexBegin iterator
    [[nodiscard]] constexpr ArrayIter<DataT, 1> begin() const
    {
      return ArrayIter<DataT, 1>(m_data);
    }

    //! vertexEnd iterator
    [[nodiscard]] constexpr ArrayIter<DataT, 1> end() const
    {
      return ArrayIter<DataT, 1>(m_data + m_range.size());
    }

    //! Get the index of the given pointer within the array.
    [[nodiscard]]
    constexpr Index<1> indexOf(const DataT *ptr) const
    {
      return Index<1>({int(ptr - m_data)});
    }

    [[nodiscard]] constexpr static int stride([[maybe_unused]] unsigned dim)
    {
      return 1;
    }

    [[nodiscard]] constexpr static const int *strides()
    {
      return &gStride1;
    }
    
    //! Broadcast assignment
    auto &operator=(const DataT &value)
    {
      fill(*this, value);
      return *this;
    }
  
  protected:
    DataT *m_data = nullptr;
    IndexRange<1> m_range;
  };

  //! Array for an N dimensional element of an array.

  template <typename DataT>
  class Array<DataT, 1> : public ArrayView<DataT, 1>
  {
  public:
    //! Create an sub array with the requested 'range'
    //! Range must be entirely contained in the original array.
    constexpr Array(Array<DataT, 1> &original, const IndexRange<1> &range)
        : ArrayView<DataT, 1>(original, range),
          m_buffer(original.buffer())
    {}

    explicit constexpr Array(std::vector<DataT> &&vec)
        : ArrayView<DataT, 1>(vec.data(), IndexRange<1>(0, int(vec.size()) - 1)),
          m_buffer(std::shared_ptr<DataT[]>(this->m_data, [aVec = std::move(vec)]([[maybe_unused]] DataT *delPtr) { assert(aVec.data() == delPtr); }))
    {}

    explicit constexpr Array(const std::vector<DataT> &vec)
        : ArrayView<DataT, 1>(IndexRange<1>(0, int(vec.size()) - 1)),
          m_buffer(std::make_shared<DataT[]>(vec.size()))
    {
      std::copy(vec.begin(), vec.end(), m_buffer.get());
      this->m_data = m_buffer.get();
    }

    //! Create an array of the given size.
    explicit constexpr Array(const IndexRange<1> &range)
        : ArrayView<DataT, 1>(range),
          m_buffer(std::make_shared<DataT[]>(range.elements()))
    {
      this->m_data = &m_buffer.get()[-range.min()];
    }

    explicit constexpr Array(const IndexRange<1> &range, const DataT &initialData)
        : ArrayView<DataT, 1>(range),
          m_buffer(std::make_shared<DataT[]>(range.elements(), initialData))
    {
      this->m_data = &m_buffer.get()[-range.min()];
    }

    //! Construct an empty array
    explicit constexpr Array() = default;

    //! Access buffer.
    [[nodiscard]] auto &buffer()
    {
      return m_buffer;
    }

  protected:
    std::shared_ptr<DataT[]> m_buffer;
  };

  //! Copy data from a source array to destination array
  template <typename Array1T, typename Array2T>
    requires(WindowedArray<Array1T, typename Array1T::value_type, 1> && WindowedArray<Array2T, typename Array2T::value_type, 1>) && std::is_convertible_v<typename Array2T::value_type, typename Array1T::value_type>
  constexpr void copy(const Array1T &dest, const Array2T &src)
  {
    auto *srcPtr = addressOfMin(src);
    auto copySize = std::min(dest.range().size(), src.range().size());
    if(copySize <= 0)
      return;
    std::copy(srcPtr, srcPtr + copySize, addressOfMin(dest));
  }

  template <typename ArrayT, typename DataT = typename ArrayT::value_type, typename OtherDataT>
    requires WindowedArray<ArrayT, DataT, 1> && std::is_convertible_v<OtherDataT, DataT>
  constexpr void fill(const ArrayT &array, const OtherDataT &value)
  {
    // 1d arrays are simple
    for(auto &at : array)
      at = value;
  }

  //! Get a view of the array, the returned object is a non-owning view of the array that has its own
  //! bounds.
  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, 1>
  [[nodiscard]] constexpr ArrayView<DataT, N> view(const ArrayT &array)
  {
    return ArrayView<DataT, N>(array);
  }

  //! Get an access to the array,
  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, 1>
  [[nodiscard]] constexpr ArrayAccess<DataT, N> access(const ArrayT &array)
  {
    return ArrayAccess<DataT, N>(array);
  }

  namespace detail
  {
    //! Helper to serialize the content of an array without the range.

    template <typename ArrayT>
    struct CerealDataBlock {
      CerealDataBlock()
          : mData(nullptr)
      {}

      explicit CerealDataBlock(ArrayT &data)
          : mData(&data)
      {}

      template <typename ArchiveT>
      void serialize(ArchiveT &archive) const
      {
        assert(mData != nullptr);
        cereal::size_type size = mData->range().elements();
        archive(cereal::make_size_tag(size));
        if(size != mData->range().elements()) {
          throw std::out_of_range("unexpected size");
        }
        for(auto &at : (*mData)) {
          archive(at);
        }
      }

      ArrayT *mData;
    };
  }// namespace detail

  //! Serialize the content of an array.

  template <CerealArchive ArchiveT, typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  void save(ArchiveT &archive,
            ArrayT const &arr)
  {
    archive(cereal::make_nvp("range", arr.range()));
    detail::CerealDataBlock<const ArrayT> blk(arr);
    archive(cereal::make_nvp("data", blk));
  }

  //! Deserialize the content of an array.

  template <CerealArchive ArchiveT, typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  void load(ArchiveT &archive,
            ArrayT &arr)
  {
    IndexRange<N> range;
    archive(cereal::make_nvp("range", range));
    if constexpr(std::is_same_v<ArrayT, Array<DataT, N>>) {
      arr = Array<DataT, N>(range);
    } else {
      if(!arr.range().contains(range)) {
        throw std::out_of_range("requested range is outside that of the allocated array");
      }
      arr = clipUnsafe(arr, range);
    }
    detail::CerealDataBlock<ArrayT> blk(arr);
    archive(cereal::make_nvp("data", blk));
  }

  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
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

  //! Some common instantiations

  extern template class Array<uint8_t, 1>;
  extern template class ArrayAccess<uint8_t, 1>;
  extern template class ArrayIter<uint8_t, 1>;

  extern template class Array<float, 1>;
  extern template class ArrayAccess<float, 1>;
  extern template class ArrayIter<float, 1>;

}// namespace Ravl2

namespace fmt
{
  template <typename DataT, unsigned N>
  struct formatter<Ravl2::Array<DataT, N>> : ostream_formatter {
  };
}// namespace fmt
