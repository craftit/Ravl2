// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_SQUAREITER_HEADER
#define RAVL_SQUAREITER_HEADER 1
/////////////////////////////////////////////////////////
//! author="Charles Galambos"
//! docentry="Ravl.API.Math.Sequences"
//! example=testSquareIterFill.cc
//! date="29/08/2000"

#include "Ravl2/Types.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/IndexRange.hh"

namespace Ravl2 {

  //: Clockwise iterate through a square.
  // starting at the centre working outward.
  
  class SquareIterC {
  public:  
    inline SquareIterC(int theSize,Index<2> theCentre = Index<2>(0,0))
      : centre(theCentre),
	maxSize(theSize)
    {
      assert(theSize >= 1);
      First();
    }
    //: Constructor.
    
    inline void First() { 
      state = 1;
      at = centre; 
      size = 1;
    }
    //: Goto first point on square.
    
    inline bool IsElm() const
    { return state != 0; }
    //: At valid position ?
    
    operator bool() const
    { return state != 0; }
    //: Test if we're at a valid point.
    
    inline const Index<2> &Data() const
    { return at; }
    //: Get point.
    
    const Index<2> &operator*() const
    { return at; }
    //: Get location of current point.
    
    bool Next();
    //: Goto next point.
    
    void operator++(int)
    { Next(); }
    //: Goto next point on square.
    
  private:
    int state = 0;     // State we're in.
    int end = 0;       // End of current side.
    Index<2> at;    // Current pixel.
    Index<2> centre; // Centre of square.
    int size = 0;    // Current size.
    int maxSize = 0; // Maximum size.
  };
  
}
#endif
