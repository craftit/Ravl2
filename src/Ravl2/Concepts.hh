//
// Created by charles galambos on 11/08/2024.
//

#pragma once

#include <concepts>
#include <cereal/cereal.hpp>

namespace Ravl2
{
  template <unsigned N>
  class Index;

  template <unsigned N>
  class IndexRange;

  template <typename DataT, unsigned N>
  class ArrayAccess;

  //! Define types that can be used as an index
  template <typename DataT>
  concept IndexType = requires(DataT data)
  { std::is_convertible_v<DataT, int>; };

  //! SimpleArray concept,  0 based array with a size() method
  //! Where
  //!   a[ind] - returns a value of type DataT
  //!   a.size() - returns the size of the array
  //!   a.begin() - returns an iterator to the beginning of the array
  //!   a.end() - returns an iterator to the end of the array or a sentinel

  template <typename ArrayT, typename DataT = typename ArrayT::value_type>
  concept SimpleArray = requires(ArrayT a, int ind) {
    { a[ind] } -> std::convertible_to<DataT>;
    { a.size() } -> std::convertible_to<unsigned>;
    { a.begin() };
    { a.end() };
  };

  //! Declaration of the concept “WindowedArray”, which is satisfied by any type “ArrayT”
  //! Where
  //!   ArrayT::dimensions - returns the number of dimensions of the array
  //!   ArrayT::value_type - returns the type of the data we're iterating over
  //!   a[ind] - returns a value of type DataT
  //!   a.range() - returns the range of the array
  //!   a.origin_address() - returns the address of the element at index 0 in the array, may not be a valid location
  //!   a.strides() - returns the strides of the array

  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
  concept WindowedArray = requires(ArrayT a, Index<N> ind, IndexRange<N> rng) {
    { ArrayT::dimensions } -> std::convertible_to<unsigned>;
    { a[ind] } -> std::convertible_to<DataT>;
    { a.range() } -> std::convertible_to<IndexRange<N>>;
    { a.origin_address() } -> std::convertible_to<DataT *>;
    { a.strides() } -> std::convertible_to<const int *>;
  };

  //! Define the WindowedIterator concept
  //! Where
  //!   IterT::dimensions - returns the number of dimensions of the array
  //!   IterT::value_type - returns the type of the data we're iterating over
  //!   *iter - dereferencing the iterator returns a value of type DataT
  //!   ++iter - incrementing the iterator returns a reference to the iterator
  //!   iter.valid() - returns a boolean indicating if the iterator is valid
  //!   iter.next() - returns a boolean, true if we're on the same row. False otherwise
  //!   iter.index() - returns the index of the current element, maybe computed from the iterator

  template <typename IterT, typename DataT = typename IterT::value_type, unsigned N = IterT::dimensions>
  concept WindowedIterator = requires(IterT iter) {
    { *iter } -> std::convertible_to<DataT>;
    { ++iter } -> std::convertible_to<IterT &>;
    { iter.valid() } -> std::convertible_to<bool>;
    { iter.next() } -> std::convertible_to<bool>;
    { iter.index() } -> std::convertible_to<Index<N>>;
  };

  //! Define the WindowedRowIterator concept
  //! Where
  //!   iter.valid() - returns a boolean indicating if the iterator is valid
  //!   iter.row() - returns the the remaining elements in the current row, the first element is at 'index()'.
  //!   iter.nextRow() - Goto the next row.
  //!   iter.index() - returns the index of the current element, maybe computed from the iterator

  template <typename IterT, typename DataT = typename IterT::value_type, unsigned N = IterT::dimensions>
  concept WindowedRowIterator = requires(IterT iter) {
    { iter.valid() } -> std::convertible_to<bool>;
    { iter.index() } -> std::convertible_to<Index<N>>;
    { iter.row() } -> SimpleArray;
    { iter.nextRow() };
  };

  //! Test if an object is an cereal style archive
  template <typename Archive>
  concept CerealArchive = requires(Archive &archive) {
    { archive(cereal::make_nvp("test", 0)) };
  };
}// namespace Ravl2