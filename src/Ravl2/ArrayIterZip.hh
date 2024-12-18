
#pragma once

#include <tuple>
#include <cassert>
#include <cstdint>
#include "Ravl2/Array.hh"

namespace Ravl2
{

  template <typename ElementT, unsigned N>
  class SlaveIter
  {
  public:
    using value_type = ElementT;
    constexpr static unsigned dimensions = N;

    constexpr SlaveIter() = default;

    constexpr SlaveIter(const IndexRange<1> *rng, ElementT *data, const int *strides) noexcept
        : mAccess(data, rng, strides)
    {
      static_assert(N > 1);
      assert(data != nullptr);
      assert(strides[N - 1] == 1);
      mIndex[N - 1] = rng[N - 1].min();
      mPtrStart = data + rng[N - 1].min();
      mPtr = mPtrStart;
      for(unsigned i = 0; i < N - 1; i++) {
        mPtr += rng[i].min() * strides[i];
        mIndex[i] = rng[i].min();
      }
    }

    template <typename ArrayT>
      requires WindowedArray<ArrayT, ElementT, N>
    explicit constexpr SlaveIter(ArrayT &img) noexcept
        : SlaveIter(img.range_data(), img.origin_address(), img.strides())
    {}

    constexpr void next()
    {
      mPtr++;
    }

  private:
    constexpr inline void nextIndex() noexcept
    {
      for(unsigned i = N - 2; i > 0; --i) {
        ++mIndex[i];
        if(mIndex[i] <= mAccess.range(i).max())
          return;
        mIndex[i] = mAccess.range(i).min();
      }
      // On the last index we don't need to update
      ++mIndex[0];
    }

  public:
    constexpr void nextRow() noexcept
    {
      nextIndex();
      mPtr = mPtrStart;
      for(unsigned i = 0; i < N - 1; i++) {
        mPtr += mAccess.stride(i) * mIndex[i];
      }
    }

    [[nodiscard]] constexpr ElementT &data() const
    {
      return *mPtr;
    }

    [[nodiscard]] constexpr ElementT *dataPtr() const
    {
      return mPtr;
    }

    [[nodiscard]] constexpr Index<N> index() const
    {
      Index<N> ret = mIndex;
      auto rowStart = mPtrStart;
      for(unsigned i = 0; i < N - 1; i++) {
        rowStart += mAccess.stride(i) * mIndex[i];
      }
      ret[N - 1] += int(mPtr - rowStart);
      return ret;
    }


    //! Access window with the current element at the origin.
    [[nodiscard]] constexpr ArrayAccess<ElementT, N> window() const noexcept
    {
      return ArrayAccess<ElementT, N>(mAccess.range() - index(), mPtr, mAccess.strides());
    }

    //! Access a point relative to the current element
    [[nodiscard]] constexpr ElementT &operator[](const Index<N> &ind) const
    {
      ElementT *tPtr = mPtr;
      for(unsigned i = 0; i < N-1; i++) {
        tPtr += mAccess.stride(i) * ind[i];
      }
      tPtr += ind[N-1];
      return *tPtr;
    }

    //! Partial index with current point as origin
    template <unsigned M>
      requires(M < N)
    [[nodiscard]] constexpr auto operator[](const Index<M> &ind) const
    {
      ElementT *tPtr = mPtr;
      for(unsigned i = 0; i < M; i++) {
        tPtr += mAccess.stride(i) * ind[i];
      }
      return ArrayAccess<ElementT, N - M>(&(mAccess.range_data()[M]), tPtr, &mAccess.strides()[M]);
    }

    //! Element by element call operator access
    template <IndexType... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
      requires (IndexN <= N)
    constexpr inline auto& operator()(IndexDataT... data) const
    {
      return operator[](Index<IndexN>({int(data)...}));
    }

    [[nodiscard]] constexpr const IndexRange<N> &range() const
    {
      return mAccess.range();
    }

    [[nodiscard]] constexpr const int *strides() const
    {
      return mAccess.strides();
    }

    //! Get end of the first dimension.
    [[nodiscard]] constexpr ElementT *end() const
    {
      return mPtr + mAccess.range(N - 1).size();
    }

    //! Test if the iterator is valid.
    [[nodiscard]] constexpr bool valid() const noexcept
    {
      return mIndex[0] <= mAccess.range(0).max();
    }

  private:
    ElementT *mPtr = nullptr;
    ElementT *mPtrStart = nullptr;
    Index<N> mIndex {};// Index of the beginning of the last dimension.
    Ravl2::ArrayAccess<ElementT, N> mAccess;
  };

  template <unsigned N, typename... DataT>
  class ArrayIterZipN
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<DataT &...>;
    constexpr static unsigned dimensions = N;

    constexpr ArrayIterZipN() = default;

    template <typename... ArrayAccessT>
      requires(WindowedArray<ArrayAccessT, typename ArrayAccessT::value_type, N> && ...)
    constexpr explicit ArrayIterZipN(ArrayAccessT &...arrays) noexcept
        : mIters(arrays...)
    {
      mEnd = std::get<0>(mIters).end();
    }

    //! Increment iterator, return true while we're on the same row.
    inline constexpr bool next()
    {
      // Increment all the iterators
      std::apply([](auto &...args) { (args.next(), ...); }, mIters);
      if(std::get<0>(mIters).dataPtr() == mEnd) {
        // If we're at the end of the first dimension, reset all the iterators.
        std::apply([](auto &...args) { (args.nextRow(), ...); }, mIters);
        mEnd = std::get<0>(mIters).end();
        return false;
      }
      return true;
    }

    //! Increment iterator
    inline constexpr auto &operator++()
    {
      // Increment all the iterators
      std::apply([](auto &...args) { (args.next(), ...); }, mIters);
      if(std::get<0>(mIters).dataPtr() == mEnd) {
        // If we're at the end of the first dimension, reset all the iterators.
        std::apply([](auto &...args) { (args.nextRow(), ...); }, mIters);
        mEnd = std::get<0>(mIters).end();
      }
      return *this;
    }

    //! Post Increment iterator
    inline constexpr auto operator++(int)
    {
      auto ret = *this;
      ++(*this);
      return ret;
    }

    //! Test if the iterator is valid.
    [[nodiscard]] constexpr inline bool valid() const noexcept
    {
      return std::get<0>(mIters).valid();
    }

    //! Get data for an element
    template <unsigned Ind>
    [[nodiscard]] constexpr auto &data()
    {
      return std::get<Ind>(mIters).data();
    }

    //! Get data for an element
    template <unsigned Ind>
    [[nodiscard]] constexpr auto dataPtr()
    {
      return std::get<Ind>(mIters).dataPtr();
    }

    //! Access the iterator
    template <unsigned Ind>
    [[nodiscard]] constexpr auto &iter()
    {
      return std::get<Ind>(mIters);
    }

    //! Get the index in the array the iterator is at
    template <unsigned Ind>
    [[nodiscard]] constexpr auto index() const
    {
      return std::get<Ind>(mIters).index();
    }

    //! Get the index the first array iterator is at
    [[nodiscard]] constexpr auto index() const
    {
      return std::get<0>(mIters).index();
    }

    //! Element by element call operator access,
    //! This access isn't range checked, it is up to the user to ensure the index is valid.
    template <unsigned Ind, IndexType... IndexDataT, unsigned IndexN = sizeof...(IndexDataT)>
      requires (IndexN <= N)
    constexpr inline auto& at(IndexDataT... data) const
    {
      return std::get<Ind>(mIters).operator[](Index<IndexN>({int(data)...}));
    }

    //! Get the index in the array the iterator is at
    template <unsigned Ind>
    [[nodiscard]] constexpr auto strides() const
    {
      return std::get<Ind>(mIters).strides();
    }

    //! Access tuple of elements
    [[nodiscard]] constexpr value_type operator*() const
    {
      return std::apply([](auto &...args) { return std::make_tuple(std::ref(args.data())...); }, mIters);
    }

    //! Compare iterators
    [[nodiscard]] constexpr bool operator==(const ArrayIterZipN &other) const noexcept
    {
      return std::get<0>(mIters).dataPtr() == std::get<0>(other.mIters).dataPtr();
    }

  private:
    std::tuple<SlaveIter<DataT, N>...> mIters;
    typename std::tuple_element<0, std::tuple<DataT...>>::type *mEnd = nullptr;
  };

  //! Make zip N constructor
  template <typename... ArrayT, unsigned N = std::tuple_element<0, std::tuple<ArrayT...>>::type::dimensions>
    requires(WindowedArray<ArrayT, typename ArrayT::value_type, N> && ...)
  constexpr auto zip(const ArrayT &...arrays) noexcept -> ArrayIterZipN<N, typename ArrayT::value_type...>
  {
    return ArrayIterZipN<N, typename ArrayT::value_type...>(arrays...);
  }

  //! Just begin one array
  template <typename ArrayT, unsigned N = ArrayT::dimensions>
    requires(WindowedArray<ArrayT, typename ArrayT::value_type, N>)
  constexpr auto begin(const ArrayT &arrays) noexcept -> ArrayIter<typename ArrayT::value_type, N>
  {
    return arrays.begin();
  }

  //! Expand a function call with the data from the iterators
  template <typename Func, typename... DataT, unsigned N, std::size_t... I>
  constexpr auto expandCall(ArrayIterZipN<N, DataT...> &iter, Func func, std::index_sequence<I...>)
  {
    return func(iter.template data<I>()...);
  }

  //! Go through all the elements of an n-d array, and call a function on each element.
  //! This also provides the index of the element in the first array
  template <unsigned N, typename FuncT, typename... ArrayT>
  constexpr void forEach(FuncT func, ArrayT &...arrays)
  {
    auto iter = zip(arrays...);
    while(iter.valid()) {
      do {
        expandCall(iter, func, std::make_index_sequence<sizeof...(ArrayT)>());
      } while(iter.next());
    }
  }

  //! Expand a function call with the data from the iterators
  template <typename Func, typename... DataT, unsigned N, std::size_t... I>
  constexpr auto expandCallIndexFirst(ArrayIterZipN<N, DataT...> &iter, Func func, std::index_sequence<I...>)
  {
    return func(iter.template index<0>(), iter.template data<I>()...);
  }

  //! Go through all the elements of an n-d array, and call a function on each element.
  //! This also provides the index of the element in the first array
  template <unsigned N, typename FuncT, typename... ArrayT>
  constexpr void forEachIndexFirst(FuncT func, ArrayT &...arrays)
  {
    auto iter = begin<N>(arrays...);
    while(iter.valid()) {
      do {
        expandCall(iter, func, std::make_index_sequence<sizeof...(ArrayT)>());
      } while(iter.next());
    }
  }

  extern template class ArrayIterZipN<1, uint8_t, uint8_t>;
  extern template class ArrayIterZipN<2, uint8_t, uint8_t>;
  extern template class ArrayIterZipN<1, float, float>;
  extern template class ArrayIterZipN<2, float, float>;

}// namespace Ravl2
