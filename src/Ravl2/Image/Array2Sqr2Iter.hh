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

  //: Iterator which slides a 2x2 window over an array.
  // The square can be accessed with DataBL,DataBR,DataUL and DataUR
  // which access the following array elements. <p>
  //  TL TR <br>
  //  BL BR <br>
  
  template<class DataT>
  class Array2dSqr2IterC
  {
  public:
    Array2dSqr2IterC()
    = default;
    //: Default constructor.

    template<typename ArrayT>
    requires WindowedArray<ArrayT,DataT,2>
    explicit Array2dSqr2IterC(const ArrayT &narray)
     : range({{narray.range(0).min()+1,narray.range(0).max()},
              {narray.range(1).min()+1,narray.range(1).max()}
             })
    {
      assert(!range.empty());
      cit = clip(narray,range).begin();
      up = cit.data() - cit.strides()[0];
    }
    //: Constructor.

    bool next()
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

    bool Next(int n) {
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

    operator bool() const
    { return this->cit.valid(); }
    //: Test if iterator is at a valid element.

    [[nodiscard]] bool valid() const noexcept
    { return this->cit.valid(); }

    void operator++() 
    { next(); }
    //: Goto next element.

    void operator++(int)
    { next(); }
    //: Goto next element.

    DataT &operator*() 
    { return *this->cit; }
    //: Access bottom right data element 
    
    const DataT &operator*() const
    { return *this->cit; }
    //: Access bottom right data element 
    
    DataT &DataBR() 
    { return *this->cit; }
    //: Access bottom right data element 

    const DataT &DataBR() const
    { return *this->cit; }
    //: Access bottom right data element 

    DataT &DataBL() 
    { return this->cit.data()[-1]; }
    //: Access bottom left data element 

    const DataT &DataBL() const
    { return this->cit.data()[-1]; }
    //: Access bottom left data element 
    
    DataT &DataTR() 
    { return *up; }
    //: Access upper right data element 
    
    const DataT &DataTR() const
    { return *up; }
    //: Access upper right data element
    
    DataT &DataTL() 
    { return up[-1]; }
    //: Access upper left data element.
    
    const DataT &DataTL() const
    { return up[-1]; }
    //: Access upper left data element

    //! Get the index of the DataBR() element.
    [[nodiscard]] auto indexBR() const
    { return this->cit.index(); }

    //! Get the index of the DataBR() element.
    [[nodiscard]] auto indexTR() const
    { return this->cit.index() + toIndex(-1,0); }

    //! Get the index of the DataBR() element.
    [[nodiscard]] auto indexTL() const
    { return this->cit.index() + toIndex(-1,-1); }

    [[nodiscard]] auto indexBL() const
    { return this->cit.index() + toIndex(0,-1); }

  protected:
    IndexRange<2> range;
    ArrayIter<DataT,2> cit;
    DataT *up = nullptr;
  };
}

