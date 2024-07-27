// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_LINE2DITER_HEADER
#define RAVL_LINE2DITER_HEADER    1
////////////////////////////////////////////////////////////////////
//! author="Charles Galambos"

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/LinePP2d.hh"

namespace Ravl2
{
  
  //: Iterate through integer grid points along a 2d line.
  // Uses a version of the midpoint algorithm to iterate all
  // 8-connected grid points on a line between two positions.
  
  class Line2dIterC {
  public:
    Line2dIterC(const Index<2> &start,const Index<2> &end);
    //: Constructor.

    template<class RealT>
    explicit Line2dIterC(const LinePP2dC<RealT> &line)
     : Line2dIterC(toIndex(line.P1()), toIndex(line.P2())) {}
    //: Constructor.
    
    void First(const Index<2> &start,const Index<2> &end);
    //: Start line again.
    
    bool IsElm() const
    { return isElm; }
    //: Still in line.
    
    operator bool() const
    { return isElm; }
    //: At a pixel in line ?
    
    bool Next();
    //: Goto next point on line.
    
    void operator++(int)
    { Next(); }
    //: Goto next point on line.

    void operator++()
    { Next(); }
    //: Goto next point on line.

    Index<2> Data() const
    { return Index<2>(x,y); }
    //: Access position.
    
    Index<2> operator*() const
    { return Index<2>(x,y); }
    //: Access position.
    
  protected:
    int dx,dy;
    int d;
    int incrE,incrNE;
    int x,y;
    int xe,ye;
    int xc,yc;
    bool isElm;
  };
}


#endif
