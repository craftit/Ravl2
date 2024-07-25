// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_INDEXRANGE2DSET_HEADER
#define RAVL_INDEXRANGE2DSET_HEADER 1
/////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! file="Ravl/Core/System/IndexRange2dSet.hh"
//! lib=RavlCore
//! docentry="Ravl.API.Core.Indexing"
//! author="Charles Galambos"
//! date="11/10/1999"

#include "Ravl/IndexRange2d.hh"
#include "Ravl/DList.hh"

namespace RavlN {
  //! userlevel=Normal
  //: Rectangle set.
  // Handles a set region defined by a set of non-overlapping rectangles.
  // The methods in this class ensure that each part of the range is only
  // covered by a single rectangle.
  
  class IndexRange2dSetC
    : public DListC<IndexRange2dC>
  {
  public:
    IndexRange2dSetC()
    {}
    //: Default constructor.
  
    IndexRange2dSetC(const IndexRange2dC &rect)
    { InsLast(rect); }
    //: Constructor.
    
    static IndexRange2dSetC Subtract(const IndexRange2dC &rect1,const IndexRange2dC &rect2);
    //: Subtract rect2 from rect1.
  
    static IndexRange2dSetC Add(const IndexRange2dC &rect1,const IndexRange2dC &rect2);
    //: Add rect2 and rect1.
  
    IndexRange2dC Enclosing() const;
    //: Get minimum enclosing rectangle for set.
    
    IndexRange2dSetC Subtract(const IndexRange2dC &rect) const;
    //: Remove 'rect' rectangle from the region given by the set.

    IndexRange2dSetC Subtract(const IndexRange2dSetC &rectset) const;
    //: Remove 'rectset' from the region given by the set.

    IndexRange2dSetC SubtractFrom(const IndexRange2dC &rect) const;
    //: Remove set from rect.
  
    IndexRange2dSetC Add(const IndexRange2dC &rect) const;
    //: Add this rectangle to the set.

    IndexRange2dSetC Add(const IndexRange2dSetC &rect) const;
    //: Add  rectangle set to this set.
  
    bool Contains(const IndexRange2dC &rect) const;
    //: Does this set wholy contain 'rect' ?
 
    SizeT Area() const;
    //: Total area of set.
  };

}


#endif
