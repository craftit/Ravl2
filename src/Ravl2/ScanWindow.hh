
#pragma once

#include <cassert>
#include <cstdint>
#include "Ravl2/Array.hh"
#include "Ravl2/Index.hh"

namespace Ravl2
{
  template <class DataT, unsigned N>
  class ScanWindow;

  //! Iterate a window over an Array<T,1>

  template <class DataT>
  class ScanWindow<DataT, 1>
  {
  public:
    using value_type = ArrayAccess<DataT, 1>;
    constexpr static unsigned dimensions = 1;

    constexpr ScanWindow(const ArrayAccess<DataT, 1> &img, const IndexRange<1> &window)
        : mArea(img.range() - window),
          mWindowRange(window),
          mAt(img.origin_address() + mArea.min()),
          mEnd(img.origin_address() + mArea.max() + 1)
    {
      assert(!mArea.empty());
    }

    constexpr ScanWindow(ArrayView<DataT, 1> &img, const IndexRange<1> &window)
        : mArea(img.range() - window),
          mWindowRange(window),
          mAt(img.origin_address() + mArea.min()),
          mEnd(img.origin_address() + mArea.max() + 1)
    {
      assert(!mArea.empty());
    }

    //! Get the window area we're scanning over.
    [[nodiscard]] constexpr const IndexRange<1> &scanArea() const
    {
      return mArea;
    }

    //! Move to the next window position.
    constexpr ScanWindow &operator++()
    {
      ++mAt;
      return *this;
    }

    //! Test if we're at the end of the window.
    [[nodiscard]] constexpr bool valid() const
    {
      return mAt < mEnd;
    }

    //! Next element, return true if we're on the same row.
    [[nodiscard]] constexpr bool next()
    {
      ++mAt;
      return mAt < mEnd;
    }

    //! Get the current window
    [[nodiscard]] constexpr ArrayAccess<DataT, 1> window() const
    {
      return ArrayAccess<DataT, 1>(&mWindowRange, &(*mAt));
    }

    //! Access window
    [[nodiscard]] constexpr ArrayAccess<DataT, 1> operator*() const
    {
      return window();
    }

    //! Get current index of the window position 0,0 in the image
    [[nodiscard]] inline constexpr Index<1> index() const
    {
      return Index<1>(int((mEnd - mAt) - mArea.max()));
    }

  protected:
    IndexRange<1> mArea;// Area we're scanning over
    IndexRange<1> mWindowRange;
    ArrayIter<DataT, 1> mAt;
    ArrayIter<DataT, 1> mEnd;
  };

  //! Iterate a window over an Array<T,N>

  template <class DataT, unsigned N>
  class ScanWindow
  {
  public:
    using value_type = ArrayAccess<DataT, N>;
    constexpr static unsigned dimensions = N;

    constexpr ScanWindow(const Ravl2::ArrayAccess<DataT, N> &img, const IndexRange<N> &window)
        : mWindowRange(window),
          mArea(img.range() - window),
          mAt(mArea, img.origin_address(), img.strides())
    {
      assert(!mArea.empty());
    }

    constexpr ScanWindow(const Ravl2::ArrayView<DataT, N> &img, const IndexRange<N> &window)
        : mWindowRange(window),
          mArea(img.range() - window),
          mAt(mArea, img.origin_address(), img.strides())
    {
      assert(!mArea.empty());
    }

    //! Get the window area we're scanning over.
    [[nodiscard]] constexpr const IndexRange<N> &scanArea() const
    {
      return mArea;
    }

    //! Move to the next window position.
    inline constexpr ScanWindow &operator++()
    {
      ++mAt;
      return *this;
    }

    //! Next element, return true if we're on the same row.
    inline constexpr bool next()
    {
      return mAt.next();
    }

    //! Test if we're at a valid element.
    [[nodiscard]] inline constexpr bool valid() const
    {
      return mAt.valid();
    }

    //! Get the current window
    [[nodiscard]] inline constexpr ArrayAccess<DataT, N> window() const
    {
      return ArrayAccess<DataT, N>(mWindowRange, &(*mAt), mAt.strides());
    }

    //! Access window
    [[nodiscard]] inline constexpr ArrayAccess<DataT, N> operator*() const
    {
      return window();
    }

    //! Index of the window in the image.
    [[nodiscard]] inline constexpr Index<N> index() const
    {
      return mAt.index();
    }

  protected:
    IndexRange<N> mWindowRange;
    IndexRange<N> mArea;// Area we're scanning over
    ArrayIter<DataT, N> mAt;
  };

  extern template class ScanWindow<uint8_t, 1>;
  extern template class ScanWindow<uint8_t, 2>;
  extern template class ScanWindow<float, 1>;
  extern template class ScanWindow<float, 2>;

}// namespace Ravl2