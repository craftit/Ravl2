// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2013, React AI Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//////////////////////////////////////////////
//! lib=RavlCore
//! file="Ravl/Core/System/IndexRangeSet.cc"

#include "Ravl/IndexRangeSet.hh"
#include "Ravl/DLIter.hh"
#include "Ravl/SysLog.hh"

#define DODEBUG 0
#if DODEBUG
#include "Ravl/Stream.hh"
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlN {

  //! Add index to set.
  void IndexRangeSetC::Add(IndexC index)
  {
    if(IsEmpty()) {
      InsLast(IndexRangeC(index,index));
      return ;
    }
    // Quick append check, most likely case.
    if(Last().Max() < index) {
      if((Last().Max() + 1) == index)
        Last().Max()++;
      else
        InsLast(IndexRangeC(index,index));
      return ;
    }
    for(DLIterC<IndexRangeC> it(*this);it;it++) {
      // Already covered ?
      if(it->Contains(index))
        return ;
      // Prepend to range ?
      if(index < it->Min()) {
        if((it->Min() - 1) == index) {
          it->Min()--;
        } else {
          ONDEBUG(RavlDebug("Prepend %d ",index.V()));
          it.InsertBef(IndexRangeC(index,index));
          // If this caused a merge it would have happened in the last iteration.
        }
        return;
      }
      if(index > it->Max()) {
        if((it->Max() + 1) == index) {
          it->Max()++;
          // Merge with next?
          if(!it.IsLast() && (it.NextData().Min() == (it->Max()+1))) {
            it->Max() = it.NextData().Max();
            it++;
            it.Del(); // Delete next range.
          }
          return ;
        }
      }
    }

  }

  //! Test if index is contained in the set.
  bool IndexRangeSetC::Contains(IndexC index) const
  {
    for(DLIterC<IndexRangeC> it(*this);it;it++) {
      // Already covered ?
      if(it->Contains(index))
        return true;
      if(index < it->Min())
        break;
    }
    return false;
  }

}
