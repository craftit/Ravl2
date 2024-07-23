// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/ArrayIterZip.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! userlevel=Normal
  //: Iterator which slides a 2x2 window over two arrays.
  // The square can be accessed with DataBL,DataBR,DataUL and DataUR
  // which access the following array elements. <p>
  //  TL TR <br>
  //  BL BR <br>
  
  template<class Data1T,class Data2T>
  class Array2dSqr2Iter2C
  {
  public:
    //: Default constructor.
    Array2dSqr2Iter2C() = default;

    //: Constructor.
    template<typename Array1T, typename Array2T>
    requires WindowedArray<Array1T,Data1T,2> && WindowedArray<Array2T,Data2T,2>
    Array2dSqr2Iter2C(const Array1T &array1,const Array2T &array2)
    {
      range1 = IndexRange<2>({{array1.range(0).min()+1,array1.range(0).max()},
			     {array1.range(1).min()+1,array1.range(1).max()}
			    });
      range2 = IndexRange<2>({{array2.range(0).min()+1,array2.range(0).max()},
			     {array2.range(1).min()+1,array2.range(1).max()}
			    });
      assert(!range1.empty());
      assert(!range2.empty());
      auto access1 = clip(array1,range1);
      auto access2 = clip(array2,range2);
      cit = ArrayIterZipN<2,Data1T,Data2T>(access1,access2);
      up1 = &(this->cit.template data<0>()) - cit.template strides<0>()[0];
      up2 = &(this->cit.template data<1>()) - cit.template strides<1>()[0];
    }

    //! Goto next element.
    // Return true if pixel is on the same row.
    bool Next() {
      up1++;
      up2++;
      if(!this->cit.next())
      {
	up1 = &(this->cit.template data<0>()) - cit.template strides<0>()[0];
	up2 = &(this->cit.template data<1>()) - cit.template strides<1>()[0];
	return false;
      }
      return true;
    }

    //! Test if iterator is at a valid element.
    [[nodiscard]] bool IsElm() const
    { return this->cit.valid(); }

    //! Test if iterator is at a valid element.
    operator bool() const
    { return this->cit.valid(); }

    [[nodiscard]] bool valid() const
    { return this->cit.valid(); }

    void operator++() 
    { Next(); }
    //: Goto next element.

    void operator++(int) 
    { Next(); }
    //: Goto next element.

    [[nodiscard]] Data1T &DataBR1()
    { return this->cit.template data<0>(); }
    //: Access bottom right data element 

    [[nodiscard]] const Data1T &DataBR1() const
    { return this->cit.template data<0>(); }
    //: Access bottom right data element 

    [[nodiscard]] Data1T &DataBL1()
    { return this->cit.template dataPtr<0>()[-1]; }
    //: Access bottom left data element 

    [[nodiscard]] const Data1T &DataBL1() const
    { return this->cit.template dataPtr<0>()[-1]; }
    //: Access bottom left data element 

    [[nodiscard]] Data1T &DataTR1()
    { return *up1; }
    //: Access upper right data element 

    [[nodiscard]] const Data1T &DataTR1() const
    { return *up1; }
    //: Access upper right data element

    [[nodiscard]] Data1T &DataTL1()
    { return up1[-1]; }
    //: Access upper left data element.

    [[nodiscard]] const Data1T &DataTL1() const
    { return up1[-1]; }
    //: Access upper left data element

    [[nodiscard]] Data2T &DataBR2()
    { return this->cit.template data<1>(); }
    //: Access bottom right data element 

    [[nodiscard]] const Data2T &DataBR2() const
    { return this->cit.template data<1>(); }
    //: Access bottom right data element 

    [[nodiscard]] Data2T &DataBL2()
    { return this->cit.template dataPtr<1>()[-1]; }
    //: Access bottom left data element 

    [[nodiscard]] const Data2T &DataBL2() const
    { return this->cit.template dataPtr<1>()[-1]; }
    //: Access bottom left data element 

    [[nodiscard]] Data2T &DataTR2()
    { return *up2; }
    //: Access upper right data element 

    [[nodiscard]] const Data2T &DataTR2() const
    { return *up2; }
    //: Access upper right data element

    [[nodiscard]] Data2T &DataTL2()
    { return up2[-1]; }
    //: Access upper left data element.

    [[nodiscard]] const Data2T &DataTL2() const
    { return up2[-1]; }
    //: Access upper left data element

  protected:
    IndexRange<2> range1;
    IndexRange<2> range2;
    ArrayIterZipN<2,Data1T,Data2T> cit;
    Data1T *up1;
    Data2T *up2;
  };
}
