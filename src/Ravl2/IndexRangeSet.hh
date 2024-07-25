// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2013, React AI Ltd
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL) v2.1 . See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_INDEXRANGESET_HEADER
#define RAVL_INDEXRANGESET_HEADER 1
/////////////////////////////////////////////////////////
//! file="Ravl/Core/System/IndexRangeSet.hh"
//! lib=RavlCore
//! docentry="Ravl.API.Core.Indexing"
//! author="Charles Galambos"
//! date="19/9/2013"

#include "Ravl/IndexRange1d.hh"
#include "Ravl/DList.hh"

namespace RavlN {
  //! userlevel=Normal
  //: Range set, a set of continuous non overlapping ranges.
  // The ranges are stored in numerical order and can be accessed with DLIterC<IndexRangeC>
  
  class IndexRangeSetC
    : public DListC<IndexRangeC>
  {
  public:
    IndexRangeSetC()
    {}
    //: Default constructor.
  
    IndexRangeSetC(const IndexRangeC &rect)
    { InsLast(rect); }
    //: Constructor.

    //! Add a single index to set.
    void Add(IndexC index);

    //! Test if index is contained in the set.
    bool Contains(IndexC index) const;
  };

}


#endif
