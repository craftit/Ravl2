// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Segmentation/ConnectedComponents.cc"

#include "Ravl/Image/ConnectedComponents.hh"
#include "Ravl/SArray1dIter.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlImageN {

  // The 'labelTable' represents a look-up table for labels. 
  // Each item contains a new label which can be the same
  // as the index of the item or smaller. Such 'labelTable' contains
  // a forest of labels, every tree of labels represents one component
  // which should have the same label. It is valid that a root item
  // of a tree has the same label value as the item index.
  
  UIntT ConnectedComponentsBaseBodyC::RelabelTable(SArray1dC<UIntT> &labelTable, UIntT currentMaxLabel) {
    ONDEBUG(cerr << "ConnectedComponentsBaseBodyC::RelabelTable(), Called. MaxLabel:" << currentMaxLabel << "\n");
    
    // Make all trees of labels have a depth of one.
    for(SArray1dIterC<UIntT> it(labelTable,currentMaxLabel+1);it;it++) {
      *it = labelTable[*it];
    }
    // Now all components in the 'labelTable' have a unique label.
    // But there can exist holes in the sequence of labels.
    
    // Squeeze the table. 
    UIntT n = 0;                     // the next new label  
    for(SArray1dIterC<UIntT> it2(labelTable,currentMaxLabel+1);it2;it2++) {
      UIntT m = labelTable[*it2];  // the label of the tree root
      
      // In the case m >= n the item with the index 'l' contains 
      // the root of the new tree,
      // because all processed roots have a label smaller than 'n'.
      // The root label 'm' has the same value as the index 'l'.
      // The root will be relabeled by a new label.
      *it2 = (m >= n) ? n++ : m;
    }
    ONDEBUG(cerr << "ConnectedComponentsBaseBodyC::RelabelTable(), Complete MaxLabel:" << (n-1) << "\n"); 
    return n - 1;  // the new last used label
  }
  
}
