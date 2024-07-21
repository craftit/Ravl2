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
  
  template<class DataT>
  void DrawCross(ArrayAccess<DataT,2> &dat,const DataT &value,const Index<2> &at,unsigned size = 3) {
    // Cross entirely in the image ?
    if(dat.Frame().Contains(IndexRange<2>(at,at).expand(size))) {
      // Its completely inside, we can just change the pixels.
      dat[at] = value;
      for(unsigned i = 1;i < size;i++) {
	Index<2> off1(i,i);
	dat[at + off1] = value;
	dat[at - off1] = value;
	off1[0] = -i;
	dat[at + off1] = value;
	dat[at - off1] = value;
      }
    } else {
      // Cross is not entirely in the image.
      if(dat.Frame().Contains(at))
	dat[at] = value;
      for(unsigned i = 1;i < size;i++) {
	Index<2> off1(i,i);
	Index<2> pat =  at + off1;
	if(dat.Frame().Contains(pat))
	  dat[pat] = value;
	pat =  at - off1;
	if(dat.Frame().Contains(pat))
	  dat[pat] = value;
	off1[0] = -i;
	pat =  at + off1;
	if(dat.Frame().Contains(pat))
	  dat[pat] = value;
	pat =  at - off1;
	if(dat.Frame().Contains(pat))
	  dat[pat] = value;
      }      
    }
  }
  //: Draw a cross in an image.
  
}

#endif
