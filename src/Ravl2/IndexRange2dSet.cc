// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//////////////////////////////////////////////
//! rcsid="$Id$"
//! lib=RavlCore
//! file="Ravl/Core/System/IndexRange2dSet.cc"

#include "Ravl/IndexRange2dSet.hh"
#include "Ravl/DLIter.hh"
#include "Ravl/DListExtra.hh"

#define DODEBUG 0
#if DODEBUG
#include "Ravl/Stream.hh"
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlN {

  //: Get minumum enclosing rectangle for set.

  IndexRange2dC IndexRange2dSetC::Enclosing() const {
    DLIterC<IndexRange2dC> it(*this);
    if(!it.IsElm())
      return IndexRange2dC(0,0,0,0);
    IndexRange2dC ret(it.Data());
    it.Next();
    for(;it.IsElm();it.Next())
      ret.Involve(it.Data());
    return ret;
  }

  //: Subtract rect2 from rect1.

  IndexRange2dSetC IndexRange2dSetC::Subtract(const IndexRange2dC &rect1,const IndexRange2dC &rect2) {
    if(((rect1.TRow() > rect2.BRow()) || (rect1.BRow() < rect2.TRow())))
      return IndexRange2dSetC(rect1); // Rects don't overlap!
    if(((rect1.LCol() > rect2.RCol())) || (rect1.RCol() < rect2.LCol()))
      return IndexRange2dSetC(rect1); // Rects don't overlap!
    IndexRange2dSetC ret;
    IndexRange2dC remainder(rect1);
  
    ONDEBUG(std::cerr << "Rectangles overlap. \n");
	  
    // Cut top.
  
    if(remainder.TRow() < rect2.TRow()) {
      ONDEBUG(std::cerr << "Top Cut. \n");
      ret.InsLast(IndexRange2dC(remainder.TRow(),rect2.TRow()-1,remainder.LCol(),remainder.RCol()));
      remainder.TRow() = rect2.TRow(); // Cut it down.
    }
  
    // Cut left.
  
    if(remainder.LCol() < rect2.LCol()) {
      ONDEBUG(std::cerr << "Left Cut. \n");
      ret.InsLast(IndexRange2dC(remainder.TRow(),remainder.BRow(),remainder.LCol(),rect2.LCol()-1));
      remainder.LCol() = rect2.LCol(); // Cut it down.
    }

    // Cut bottom.
  
    if(remainder.BRow() > rect2.BRow()) {
      ONDEBUG(std::cerr << "Bottom Cut. \n");
      ret.InsLast(IndexRange2dC(rect2.BRow()+1,remainder.BRow(),remainder.LCol(),remainder.RCol()));
      remainder.BRow() = rect2.BRow(); // Cut it down.
    }

    // Cut right.
  
    if(remainder.RCol() > rect2.RCol()) {
      ONDEBUG(std::cerr << "Right Cut. \n");
      ret.InsLast(IndexRange2dC(remainder.TRow(),remainder.BRow(),rect2.RCol()+1,remainder.RCol()));
      remainder.RCol() = rect2.RCol(); // Cut it down.
    }
  
    return ret;
  }

  //: Add rect2 and rect1.

  IndexRange2dSetC IndexRange2dSetC::Add(const IndexRange2dC &rect1,const IndexRange2dC &rect2) {
    IndexRange2dSetC ret;
    if(!rect1.IsOverlapping(rect2)) { // No overlap.
      ret.InsLast(rect1);
      ret.InsLast(rect2);
      return ret;
    }
    // Try and keep rects approximantly the same size....
    if(rect1.Area() > rect2.Area()) {
      ret += Subtract(rect1,rect2);
      ret.InsLast(rect2);
    } else {
      ret += Subtract(rect2,rect1);
      ret.InsLast(rect1);
    }
    return ret;
  }

  //: Remove 'rect' rectangle from the region given by the set.

  IndexRange2dSetC IndexRange2dSetC::Subtract(const IndexRange2dC &rect) const {
    IndexRange2dSetC ret;
    for(DLIterC<IndexRange2dC> it(*this);it.IsElm();it.Next())
      ret += Subtract(it.Data(),rect);
    return ret;
  }

  //: Remove 'rectset' from the region given by the set.

  IndexRange2dSetC IndexRange2dSetC::Subtract(const IndexRange2dSetC &rectset) const {
    IndexRange2dSetC ret = (*this);
    for(DLIterC<IndexRange2dC> it(rectset);it.IsElm();it.Next())
      ret = ret.Subtract(it.Data());
    return ret;
  }

  //: Remove set from rect.

  IndexRange2dSetC IndexRange2dSetC::SubtractFrom(const IndexRange2dC &rect) const 
  { return IndexRange2dSetC(rect).Subtract(*this); }

  //: Add this rectangle to the set.

  IndexRange2dSetC IndexRange2dSetC::Add(const IndexRange2dC &rect) const {
    IndexRange2dSetC ret = Subtract(rect);
    ret.InsLast(rect); // Well it works doesn't it!!!!
    return ret;
  }

  //: Add  rectangle set to this set.

  IndexRange2dSetC IndexRange2dSetC::Add(const IndexRange2dSetC &rect) const {
    IndexRange2dSetC ret(*this);
    for(DLIterC<IndexRange2dC> it(*this);it.IsElm();it.Next()) 
      ret = ret.Add(it.Data());
    return ret;
  }

  //: Does this set wholy contain 'rect' ?

  bool IndexRange2dSetC::Contains(const IndexRange2dC &rect) const {
    IndexRange2dSetC remainder(rect);
    for(DLIterC<IndexRange2dC> it(*this);it.IsElm();it.Next()) {
      remainder = remainder.Subtract(it.Data());
      if(remainder.IsEmpty())
	return true;
    }
    return false;
  }

  //: Total area of set.

  SizeT IndexRange2dSetC::Area() const {
    SizeT ret = 0;
    for(DLIterC<IndexRange2dC> it(*this);it.IsElm();it.Next()) {
      ONDEBUG(std::cerr << "+Area: " << it.Data().Area() << "\n");
      ret += it.Data().Area();
    }
    return ret;
  }

}
