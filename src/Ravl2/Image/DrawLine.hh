// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_DRAWLINE_HEADER
#define RAVLIMAGE_DRAWLINE_HEADER 1
///////////////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! author="Charles Galambos"
//! date="22/04/2002"
//! docentry="Ravl.API.Images.Drawing"
//! lib=RavlImage
//! userlevel=Normal
//! file="Ravl/Image/Base/DrawLine.hh"

#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Line2dIter.hh"
#include "Ravl2/Geometry/LinePP2d.hh"

namespace Ravl2 {
  
  template<class DataT>
  void DrawLine(ArrayAccess<DataT,2> &Dat,const DataT &Value,const LinePP2dC &Line)
  {
    LinePP2dC line = Line;
    if (line.ClipBy(Dat.Frame()))
      for(Line2dIterC it(line.P1(),line.P2());it;it++)
        Dat[*it] = Value;
    return ;
  }
  //: Draw a line in an image.

  template<class DataT>
  void DrawLine(ArrayAccess<DataT,2> &Dat,const DataT &Value,const Index<2> &From,const Index<2> &To) {
    DrawLine(Dat, Value, LinePP2dC(From,To));
  }
  //: Draw a line in an image.

  template<class DataT>
  void DrawLine(Array2dC<DataT> &Dat,const DataT &ValueFrom,const DataT &ValueTo,const LinePP2dC &Line) {
    LinePP2dC line = Line;
    RealRange2dC frame(Dat.Frame().TRow(),Dat.Frame().BRow(),Dat.Frame().LCol(),Dat.Frame().RCol());
    if (line.ClipBy(frame)) {
      RealT length = line.Length();
      // If both start and end are inside the image, all pixels in between are.
      for(Line2dIterC it(line.P1(),line.P2());it;it++) {
        RealT alpha = sqrt(static_cast<double>((it.Data() - Index2dC(line.P1())).SumOfSqr().V())) / length;
        Dat[*it] = DataT((ValueFrom*(1-alpha)) + (ValueTo*alpha));
      }
    }
    return ;
  }
  //: Draw a line in an image, shaded between two colours <code>valuefrom</code> and <code>valueto</code>
  // This function requires that DataT has a working operator*(double) function

  template<class DataT>
  void DrawLine(Array<DataT,2> &dat,const DataT &valuefrom,const DataT &valueto,const Index<2> &from,const Index<2> &to) {
    DrawLine(dat, valuefrom, valueto, LinePP2dC(from,to));
  }
  //: Draw a line in an image, shaded between two colours <code>valuefrom</code> and <code>valueto</code>
  // This function requires that DataT has a working operator*(double) function
}

#endif
