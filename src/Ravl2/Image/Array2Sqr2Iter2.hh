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
    Array2dSqr2Iter2C() = default;
    //: Default constructor.

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
      cit = ArrayIterZip<2,Data1T,Data2T>(array1,array2);
      up1 = &(this->cit.data1()) - cit.strides1()[0];
      up2 = &(this->cit.data2()) - cit.strides2()[0];
    }
    //: Constructor.

    bool Next() { 
      up1++;
      up2++;
      if(!this->cit.next())
      {
	up1 = &(this->cit.data1()) - cit.strides1()[0];
	up2 = &(this->cit.data2()) - cit.strides2()[0];
	return false;
      }
      return true;
    }
    //: Goto next element.
    // Return true if pixel is on the same row.
    
    bool IsElm() const
    { return this->cit.valid(); }
    //: Test if iterator is at a valid element.
    
    operator bool() const
    { return this->cit.valid(); }
    //: Test if iterator is at a valid element.

    [[nodiscard]] bool valid() const
    { return this->cit.valid(); }

    void operator++() 
    { Next(); }
    //: Goto next element.

    void operator++(int) 
    { Next(); }
    //: Goto next element.
    
    Data1T &DataBR1() 
    { return this->cit.data1(); }
    //: Access bottom right data element 

    const Data1T &DataBR1() const
    { return this->cit.data1(); }
    //: Access bottom right data element 

    Data1T &DataBL1() 
    { return this->cit.dataPtr1()[-1]; }
    //: Access bottom left data element 

    const Data1T &DataBL1() const
    { return this->cit.dataPtr1()[-1]; }
    //: Access bottom left data element 
    
    Data1T &DataTR1() 
    { return *up1; }
    //: Access upper right data element 
    
    const Data1T &DataTR1() const
    { return *up1; }
    //: Access upper right data element
    
    Data1T &DataTL1() 
    { return up1[-1]; }
    //: Access upper left data element.
    
    const Data1T &DataTL1() const
    { return up1[-1]; }
    //: Access upper left data element
    
    Data2T &DataBR2() 
    { return this->cit.data2(); }
    //: Access bottom right data element 

    const Data2T &DataBR2() const
    { return this->cit.data2(); }
    //: Access bottom right data element 

    Data2T &DataBL2() 
    { return this->cit.dataPtr2()[-1]; }
    //: Access bottom left data element 

    const Data2T &DataBL2() const
    { return this->cit.dataPtr2()[-1]; }
    //: Access bottom left data element 
    
    Data2T &DataTR2() 
    { return *up2; }
    //: Access upper right data element 
    
    const Data2T &DataTR2() const
    { return *up2; }
    //: Access upper right data element
    
    Data2T &DataTL2() 
    { return up2[-1]; }
    //: Access upper left data element.
    
    const Data2T &DataTL2() const
    { return up2[-1]; }
    //: Access upper left data element

  protected:
    IndexRange<2> range1;
    IndexRange<2> range2;
    ArrayIterZip<2,Data1T,Data2T> cit;
    Data1T *up1;
    Data2T *up2;
  };
}
