
#pragma once

#include <cassert>
#include "Ravl2/Array.hh"
#include "Ravl2/Index.hh"

namespace Ravl2
{
  template<class DataT,unsigned N>
  class ScanWindow;

  //! Iterate a window over an Array<T,1>

  template<class DataT>
  class ScanWindow<DataT,1>
  {
  public:
    ScanWindow(ArrayAccess<DataT, 1> img, const IndexRange<1> &window)
      : mArea(img.range() - window),
	mAt(img.origin_address() + mArea.min()),
	mEnd(img.origin_address() + mArea.max() + 1),
	mWindowRange(window)
    {
      assert(!mArea.empty());
    }

    ScanWindow(ArrayView<DataT, 1> &img, const IndexRange<1> &window)
      : mArea(img.range() - window),
        mAt(img.origin_address() + mArea.min()),
        mEnd(img.origin_address() + mArea.max() + 1),
        mWindowRange(window)
    {
      assert(!mArea.empty());
    }

      //! Get the window area we're scanning over.
    [[nodiscard]] const IndexRange<1> &scanArea() const
    {
      return mArea;
    }

    //! Move to the next window position.
    void operator++()
    {
      ++mAt;
    }

    //! Test if we're at the end of the window.
    [[nodiscard]] bool done() const
    {
      return mAt == mEnd;
    }

    //! Get the current window
    [[nodiscard]] ArrayAccess<DataT,1> window() const
    {
      return ArrayAccess<DataT,1>(&mWindowRange,&(*mAt));
    }

    //! Get current index of the window position 0,0 in the image
    [[nodiscard]] Index<1> indexIn(const Ravl2::ArrayView<DataT,1> &img)
    { return img.indexOf(&*mAt); }

  protected:
    IndexRange<1> mArea; // Area we're scanning over
    ArrayIter<DataT, 1> mAt;
    ArrayIter<DataT, 1> mEnd;
    IndexRange<1> mWindowRange;
  };


  //! Iterate a window over an Array<T,N>

  template<class DataT,unsigned N>
  class ScanWindow
  {
  public:
    ScanWindow(const Ravl2::ArrayAccess<DataT,N> &img, const IndexRange<N> &window)
      : mWindowRange(window),
        mArea(img.range() - window),
        mAt(mArea, img.origin_address(),img.strides())
    {
      assert(!mArea.empty());
    }

    ScanWindow(const Ravl2::ArrayView<DataT,N> &img, const IndexRange<N> &window)
      : mWindowRange(window),
        mArea(img.range() - window),
	mAt(mArea, img.origin_address(),img.strides())
    {
      assert(!mArea.empty());
    }


    //! Get the window area we're scanning over.
    [[nodiscard]] const IndexRange<N> &scanArea() const
    {
      return mArea;
    }

    //! Move to the next window position.
    void operator++()
    {
      ++mAt;
    }

    //! Next element, return true if we're on the same row.
    bool next()
    { return mAt.next(); }

    //! Test if we're at the end of the window.
    [[nodiscard]] bool done() const
    { return mAt.done(); }

    //! Test if we're at a valid element.
    [[nodiscard]] bool valid() const
    { return mAt.valid(); }

    //! Get the current window
    [[nodiscard]] ArrayAccess<DataT,N> window() const
    {
      return ArrayAccess<DataT,N>(mWindowRange,&(*mAt),mAt.strides());
    }

  protected:
    IndexRange<N> mWindowRange;
    IndexRange<N> mArea; // Area we're scanning over
    ArrayIter<DataT, N> mAt;
  };
}