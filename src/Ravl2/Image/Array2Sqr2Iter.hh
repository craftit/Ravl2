// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! @brief Iterator which slides a 2x2 window over an array.
  //! This is for backward compatibility with Ravl code.  New code supports relative access from normal iterators. with at(-1,0) etc.
  // The square can be accessed with DataBL,DataBR,DataUL and DataUR
  // which access the following array elements. <p>
  //  TL TR <br>
  //  BL BR <br>

  template <class DataT>
  class Array2dSqr2IterC
  {
  public:
    constexpr Array2dSqr2IterC() = default;
    //: Default constructor.

    template <typename ArrayT>
      requires WindowedArray<ArrayT, DataT, 2>
    explicit constexpr Array2dSqr2IterC(const ArrayT &narray)
        : range({{narray.range(0).min() + 1, narray.range(0).max()},
                 {narray.range(1).min() + 1, narray.range(1).max()}})
    {
      assert(!range.empty());
      cit = clipUnsafe(narray, range).begin();
      up = cit.data() - cit.strides()[0];
    }
    //: Constructor.

    constexpr bool next()
    {
      up++;
      if(!this->cit.next()) {
        up = this->cit.data() - cit.strides()[0];
        return false;
      }
      return true;
    }
    //: Goto next element.
    // Returns true if its on the same row.

    constexpr bool Next(int n)
    {
      bool sameRow = true;
      for(int i = 0; i < n; i++) {
        if(!next())
          sameRow = false;
      }
      return sameRow;
    }
    //: Goto next element.
    // Returns true if its on the same row.

    //    void NextCol(int n) {
    //      up += n;
    //      this->cit += n;
    //    }
    //: Goto next column
    // This will NOT automatically go to the next row.
    // Returns true if is a valid element.

    constexpr operator bool() const
    {
      return this->cit.valid();
    }
    //: Test if iterator is at a valid element.

    [[nodiscard]] constexpr bool valid() const noexcept
    {
      return this->cit.valid();
    }

    constexpr void operator++()
    {
      next();
    }
    //: Goto next element.

    constexpr void operator++(int)
    {
      next();
    }
    //: Goto next element.

    constexpr DataT &operator*()
    {
      return *this->cit;
    }
    //: Access bottom right data element

    constexpr const DataT &operator*() const
    {
      return *this->cit;
    }
    //: Access bottom right data element

    constexpr DataT &DataBR()
    {
      return *this->cit;
    }
    //: Access bottom right data element

    constexpr const DataT &DataBR() const
    {
      return *this->cit;
    }
    //: Access bottom right data element

    constexpr DataT &DataBL()
    {
      return this->cit.data()[-1];
    }
    //: Access bottom left data element

    constexpr const DataT &DataBL() const
    {
      return this->cit.data()[-1];
    }
    //: Access bottom left data element

    constexpr DataT &DataTR()
    {
      return *up;
    }
    //: Access upper right data element

    constexpr const DataT &DataTR() const
    {
      return *up;
    }
    //: Access upper right data element

    constexpr DataT &DataTL()
    {
      return up[-1];
    }
    //: Access upper left data element.

    constexpr const DataT &DataTL() const
    {
      return up[-1];
    }
    //: Access upper left data element

    //! Get the index of the DataBR() element.
    [[nodiscard]] constexpr auto indexBR() const
    {
      return this->cit.index();
    }

    //! Get the index of the DataBR() element.
    [[nodiscard]] constexpr auto indexTR() const
    {
      return this->cit.index() + toIndex(-1, 0);
    }

    //! Get the index of the DataBR() element.
    [[nodiscard]] constexpr auto indexTL() const
    {
      return this->cit.index() + toIndex(-1, -1);
    }

    [[nodiscard]] constexpr auto indexBL() const
    {
      return this->cit.index() + toIndex(0, -1);
    }

  protected:
    IndexRange<2> range;
    ArrayIter<DataT, 2> cit;
    DataT *up = nullptr;
  };
}// namespace Ravl2
