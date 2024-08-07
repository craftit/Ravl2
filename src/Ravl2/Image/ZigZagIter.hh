// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_ZIGZAGITER_HEADER
#define RAVL_ZIGZAGITER_HEADER 1
//! rcsid="$Id$"
//! lib=RavlMath
//! author="Charles Galambos"
//! file="Ravl/Math/Sequence/ZigZagIter.hh"
//! docentry="Ravl.API.Math.Sequences"

#include "Ravl/IndexRange2d.hh"

namespace RavlN {
  
  //! userlevel=Normal
  //: Iterate a rectangle in a Zig Zag pattern from the top left corner.
  // Usefull when dealing with DCT coefficients.
  
  class ZigZagIterC {
  public:
    ZigZagIterC(const IndexRange2dC &nrect);
    //: Constuctor
    // Note: rectangle must be square at the moment.
    
    IndexRange2dC &Frame()
    { return rect; }
    //: Access image frame.
    
    const IndexRange2dC &Frame() const
    { return rect; }
    //: Access image frame.
    
    bool First();
    //: Goto first index in sequence.
    
    bool IsElm() const
    { return ok; }
    //: Test if we're at a valid element.
    
    const Index2dC &Data() const
    { return at; }
    //: Get current pixel;
  
    const Index2dC &operator*() const 
    { return at; }
    //: Get current pixel;
    
    bool Next();
    //: Goto next pixel in scan pattern.
    
    bool operator++(int)
    { return Next(); }
    //: Goto next pixel in scan pattern.
    
    operator bool() const
    { return ok; }
    //: Test if we're at a valid element.
    
  protected:
    Index2dC at;
    IndexRange2dC rect;
    bool ok;
  };
}


#endif
