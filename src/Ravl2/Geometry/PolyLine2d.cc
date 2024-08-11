// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PolyLine2d.hh"
#include "Ravl2/LinePP2d.hh"

namespace Ravl2 {

   bool PolyLine2dC::IsSelfIntersecting() const {
     DLIterC<Point<RealT,2>> ft(*this);
     if(!ft) return false;
     while(1) {
       LinePP2dC l1(ft.Data(), ft.NextData());
       ft++;
       DLIterC<Point<RealT,2>> it2 = ft;
       if (!it2) break;
       for (; it2; it2++) {
         LinePP2dC l2(it2.Data(), it2.NextData());
         if (l1.HasInnerIntersection(l2))
           return true;
       }
     }
     return false;
   }

  //: Measure the length of the poly line in euclidean space.
  
  RealT PolyLine2dC::Length() const {
    DLIterC<Point<RealT,2>> ft(*this);
    if(!ft) return 0.0;
    Point<RealT,2> last = *ft;
    RealT len = 0;
    for(ft++;ft;ft++) {
      len += last.EuclidDistance(*ft);
      last = *ft;
    }
    return len;
  }

}
