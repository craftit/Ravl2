// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="22/04/2002"
#pragma once

#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! Draw a rectangle in an image.
  template<class DataT>
  void DrawFilledFrame(Array<DataT,2> &dat,const DataT &value,const IndexRange<2> &rect)
  {
    IndexRange<2> dr = rect.clip(dat.Frame());
    if (dr.empty())
      return; // Nothing to draw around.
    dat.access(dr).fill(value);
    return;
  }

    //! Draw a rectangle in an image.
  template<class DataT>
  void DrawFrame(Array<DataT,2> &dat,const DataT &value,const IndexRange<2> &rect) {
    IndexRange<2> dr = rect.clip(dat.Frame());
    if(dr.empty())
      return ; // Nothing to draw around.

    DataT *it1 = nullptr;
    const DataT *eor = nullptr;
    int ColN = dr.range(1).size(); // Number of columns.
    if(rect.range(0).min() == dr.range(0).min() && rect.range(0).max() == dr.range(0).max()) { // The rectangle wasn't clipped.
      // Do horizontal lines.
      DataT *it2 = nullptr;
      it1 = &(dat[dr.range(0).min()][dr.range(0).min()]);
      it2 = &(dat[dr.range(0).max()][dr.range(0).min()]);
      eor = &(it1[ColN]);
      for(;it1 != eor;) {
	*(it1++) = value;
	*(it2++) = value;
      }
    } else {
      // Do top and bottom lines seperatly
      if(rect.range(0).min() == dr.range(0).min()) {
	// Do top horizontal line.
	it1 = &(dat[dr.range(0).min()][dr.range(0).min()]);
	eor = &(it1[ColN]);
	for(;it1 != eor;)
	  *(it1++) = value;
      }
      if(rect.range(0).max() == dr.range(0).max()) {
	// Do bottom horizontal line.
	it1 = &(dat[dr.range(0).max()][dr.range(0).min()]);
	eor = &(it1[ColN]);
	for(;it1 != eor;)
	  *(it1++) = value;	
      }
    }
    // Do vertical lines.
    ColN--;
    if(dr.range(0).min() == rect.range(0).min() && dr.range(1).max() == rect.range(1).max()) {// Not clipped.
      for(auto r = dr.range(0).min()+1; r < dr.range(0).max(); r++) {
	it1 = &(dat[r][dr.range(0).min()]);
	it1[0] = value;
	it1[ColN] = value;
      }
    } else { // Clipped.
      if(dr.range(0).min() == rect.range(0).min()) {
	for(int r = dr.range(0).min()+1; r < dr.range(0).max(); r++)
	  dat[r][dr.range(0).min()] = value;
      }
      if(dr.range(1).max() == rect.range(1).max()) {
	for(int r = dr.range(0).min()+1; r < dr.range(0).max(); r++)
	  dat[r][dr.range(1).max()] = value;	
      }
    }
  }

  //! Draw a rectangle in an image of given width
  //! The rectangle is assumed to be the outer one and the image will be filled inside it by 'width' pixels.
  template<class DataT>
  void DrawFrame(Array<DataT,2> &dat,const DataT &value,int width,const IndexRange<2> &outerRect) {
    IndexRange<2> innerRect = outerRect.shrink(width);
    IndexRange<2> outerClipped = outerRect;
    if(!outerClipped.clipBy(dat.Frame()) || width == 0)
      return ; // Nothing to draw.

    //cerr << "Inner=" << innerRect << "\n";
    //cerr << "Outer=" << outerRect << "\n";
    //cerr << "Clipped=" << outerClipped << "\n";
    
    DataT *it1,*it2;
    const DataT *eor;
    int ColN = outerClipped.range(1).size(); // Number of columns.
    if(outerRect.range(0).min() == outerClipped.range(0).min() && outerRect.range(0).max() == outerClipped.range(0).max()) { // The innerRect wasn't clipped.
      // Do horizontal lines.
      for(int i = 0;i < width;i++) {
	it1 = &(dat[outerClipped.range(0).min()+i][outerClipped.range(0).min()]);
	it2 = &(dat[outerClipped.range(0).max()-i][outerClipped.range(0).min()]);
	eor = &(it1[ColN]);
	for(;it1 != eor;) {
	  *(it1++) = value;
	  *(it2++) = value;
	}
      }
    } else {
      // Do top and bottom lines seperatly
      for(int i = 0;i < width;i++) {
	if(dat.Frame().range(0).min() <= (outerRect.range(0).min() - i)) {
	  // Do top horizontal line.
	  it1 = &(dat[outerRect.range(0).min() - i][outerClipped.range(0).min()]);
	  eor = &(it1[ColN]);
	  for(;it1 != eor;)
	    *(it1++) = value;
	}
	if(dat.Frame().range(0).max() >= (outerRect.range(0).max() + i)) {
	  // Do bottom horizontal line.
	  it1 = &(dat[outerRect.range(0).max() + i][outerClipped.range(0).min()]);
	  eor = &(it1[ColN]);
	  for(;it1 != eor;)
	    *(it1++) = value;	
	}
      }
    }
    // Do vertical lines.
    if(outerClipped.range(0).min() == outerRect.range(0).min() && outerClipped.range(1).max() == outerRect.range(1).max()) {// Not clipped.
      for(int r = outerRect.range(0).min()+1; r < outerRect.range(0).max(); r++) {
	it1 = &(dat[r][outerRect.range(0).min()]);
	it2 = &(dat[r][innerRect.range(1).max()+1]);
	eor = &(it1[width]);
	for(;it1 != eor;it1++,it2++) {
	  *it1 = value;
	  *it2 = value;
	}
      }
    } else { // Clipped.
      IndexRange<2> r1(Index<2>(innerRect.range(0).min(),outerRect.range(0).min()),Index<2>(innerRect.range(0).max(),innerRect.range(0).min()-1));
      if(r1.clipBy(dat.Frame())) {
        dat.access(r1).fill(value);
      }
      IndexRange<2> r2(Index<2>(innerRect.range(0).min(),innerRect.range(1).max()+1),Index<2>(innerRect.range(0).max(),outerRect.range(1).max()));
      if(r2.clipBy(dat.Frame())) {
        dat.access(r2).fill(value);
      }
    }
  }

}
