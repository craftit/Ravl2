// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Line2Iter.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //: Constructor.

  Line2IterC::Line2IterC(const Index<2> &start, const Index<2> &end)
  {
    First(start, end);
  }

  //: Start line again.

  void Line2IterC::First(const Index<2> &start, const Index<2> &end)
  {
    dx = (end[0] - start[0]);
    dy = (end[1] - start[1]);
    if(dx >= 0) {
      xc = 1;
    } else {
      xc = -1;
      dx *= -1;
    }
    if(dy >= 0)
      yc = 1;
    else {
      yc = -1;
      dy *= -1;
    }
    if(dx >= dy) {
      d = 2 * dy - dx;
      incrE = 2 * dy;
      incrNE = 2 * (dy - dx);
    } else {
      d = 2 * dx - dy;
      incrE = 2 * dx;
      incrNE = 2 * (dx - dy);
    }
    x = start[0];
    y = start[1];
    xe = end[0];
    ye = end[1];
    isElm = true;
    ONDEBUG(std::cerr << "dx=" << dx << " dy=" << dy << " incrE=" << incrE << " incrNE=" << incrNE << "\n");
  }

  //: Goto next point.

  bool Line2IterC::Next()
  {
    if(x == xe && y == ye) {
      isElm = false;
      return false;
    }
    if(dx >= dy) {
      if(d <= 0) {
        d += incrE;
        x += xc;
      } else {
        d += incrNE;
        x += xc;
        y += yc;
      }
    } else {
      if(d <= 0) {
        d += incrE;
        y += yc;
      } else {
        d += incrNE;
        x += xc;
        y += yc;
      }
    }
    return true;
  }
}// namespace Ravl2
