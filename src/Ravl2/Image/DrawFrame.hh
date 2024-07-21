// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once
///////////////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! author="Charles Galambos"
//! date="22/04/2002"
//! docentry="Ravl.API.Images.Drawing"
//! lib=RavlImage
//! userlevel=Normal
//! file="Ravl/Image/Base/DrawFrame.hh"

#include "Ravl2/Array.hh"

namespace Ravl2
{
  
  template<class DataT>
  void DrawFrame(Array2dC<DataT> &dat,const DataT &value,const IndexRange<2> &rect, bool fill=false) {
    IndexRange<2> dr = rect.clip(dat.Frame());
    if(dr.empty())
      return ; // Nothing to draw around.
    
    if (fill) {
      for (Array2dIterC<DataT> it(dat,dr); it; it++) 
	*it = value;
      return;
    }
    
    DataT *it1,*it2,*eor;
    IntT ColN = dr.Cols();
    if(rect.TRow() == dr.TRow() && rect.BRow() == dr.BRow()) { // The rectangle wasn't clipped.
      // Do horizontal lines.
      it1 = &(dat[dr.TRow()][dr.LCol().V()]);
      it2 = &(dat[dr.BRow()][dr.LCol().V()]);
      eor = &(it1[ColN]);
      for(;it1 != eor;) {
	*(it1++) = value;
	*(it2++) = value;
      }
    } else {
      // Do top and bottom lines seperatly
      if(rect.TRow() == dr.TRow()) {
	// Do top horizontal line.
	it1 = &(dat[dr.TRow()][dr.LCol().V()]);
	eor = &(it1[ColN]);
	for(;it1 != eor;)
	  *(it1++) = value;
      }
      if(rect.BRow() == dr.BRow()) {
	// Do bottom horizontal line.
	it1 = &(dat[dr.BRow()][dr.LCol().V()]);
	eor = &(it1[ColN]);
	for(;it1 != eor;)
	  *(it1++) = value;	
      }
    }
    // Do vertical lines.
    ColN--;
    if(dr.LCol() == rect.LCol() && dr.RCol() == rect.RCol()) {// Not clipped.
      for(IndexC r = dr.TRow()+1; r < dr.BRow(); r++) {
	it1 = &(dat[r][dr.LCol().V()]);
	it1[0] = value;
	it1[ColN] = value;
      }
    } else { // Clipped.
      if(dr.LCol() == rect.LCol()) {
	for(IndexC r = dr.TRow()+1; r < dr.BRow(); r++)
	  dat[r][dr.LCol().V()] = value;
      }
      if(dr.RCol() == rect.RCol()) {
	for(IndexC r = dr.TRow()+1; r < dr.BRow(); r++)
	  dat[r][dr.RCol().V()] = value;	
      }
    }
  }
  //: Draw a rectangle in an image.
  
  template<class DataT>
  void DrawFrame(Array2dC<DataT> &dat,const DataT &value,int width,const IndexRange<2> &outerRect) {
    IndexRange<2> innerRect = outerRect.Shrink(width);
    IndexRange<2> outerClipped = outerRect;
    outerClipped.ClipBy(dat.Frame());
    if(outerClipped.Area() <= 0 || width == 0)
      return ; // Nothing to draw..
    
    //cerr << "Inner=" << innerRect << "\n";
    //cerr << "Outer=" << outerRect << "\n";
    //cerr << "Clipped=" << outerClipped << "\n";
    
    DataT *it1,*it2,*eor;
    IntT ColN = outerClipped.Cols();
    if(outerRect.TRow() == outerClipped.TRow() && outerRect.BRow() == outerClipped.BRow()) { // The innerRect wasn't clipped.
      // Do horizontal lines.
      for(int i = 0;i < width;i++) {
	it1 = &(dat[outerClipped.TRow()+i][outerClipped.LCol().V()]);
	it2 = &(dat[outerClipped.BRow()-i][outerClipped.LCol().V()]);
	eor = &(it1[ColN]);
	for(;it1 != eor;) {
	  *(it1++) = value;
	  *(it2++) = value;
	}
      }
    } else {
      // Do top and bottom lines seperatly
      for(int i = 0;i < width;i++) {
	if(dat.Frame().TRow() <= (outerRect.TRow() - i)) {
	  // Do top horizontal line.
	  it1 = &(dat[outerRect.TRow() - i][outerClipped.LCol().V()]);
	  eor = &(it1[ColN]);
	  for(;it1 != eor;)
	    *(it1++) = value;
	}
	if(dat.Frame().BRow() >= (outerRect.BRow() + i)) {
	  // Do bottom horizontal line.
	  it1 = &(dat[outerRect.BRow() + i][outerClipped.LCol().V()]);
	  eor = &(it1[ColN]);
	  for(;it1 != eor;)
	    *(it1++) = value;	
	}
      }
    }
    // Do vertical lines.
    if(outerClipped.LCol() == outerRect.LCol() && outerClipped.RCol() == outerRect.RCol()) {// Not clipped.
      for(IndexC r = outerRect.TRow()+1; r < outerRect.BRow(); r++) {
	it1 = &(dat[r][outerRect.LCol().V()]);
	it2 = &(dat[r][innerRect.RCol().V()+1]);
	eor = &(it1[width]);
	for(;it1 != eor;it1++,it2++) {
	  *it1 = value;
	  *it2 = value;
	}
      }
    } else { // Clipped.
      IndexRange<2> r1(Index2dC(innerRect.TRow(),outerRect.LCol()),Index2dC(innerRect.BRow(),innerRect.LCol()-1));
      r1.ClipBy(dat.Frame());
      if(r1.Area() > 0) {
	for (Array2dIterC<DataT> it(dat,r1); it; it++) 
	  *it = value;
      }
      IndexRange<2> r2(Index2dC(innerRect.TRow(),innerRect.RCol()+1),Index2dC(innerRect.BRow(),outerRect.RCol()));
      r2.ClipBy(dat.Frame());
      if(r2.Area() > 0) {
	for (Array2dIterC<DataT> it(dat,r2); it; it++) 
	  *it = value;
      }
    }
    
  }
  //: Draw a rectangle in an image of given width
  // The rectangle is assumed to be the outer one and the image will be filled inside it by 'width' pixels.
  
}

