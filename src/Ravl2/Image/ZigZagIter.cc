// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Sequence/ZigZagIter.cc"

#include "Ravl2/Image/ZigZagIter.hh"

namespace RavlN {
  
  //: Constuctor
  
  ZigZagIterC::ZigZagIterC(const IndexRange2dC &nrect)
    : at(nrect.Origin()),
      rect(nrect),
      ok(true)
  { 
    //cerr << "Init=" << at << " \n";
    if(rect.Area() == 0)
      ok = false; 
  }
  
  //: Goto first index in sequence.
  
  bool ZigZagIterC::First() {
    if(rect.Area() == 0)
      return false;
    at = rect.Origin();
    return true;
  }

  //: Goto next pixel in scan pattern.
  
  bool ZigZagIterC::Next() {
    //cerr << "Current=" << at << "\n";
    int row = at[0].V() + at[1].V();
    if((row % 2) == 0) { // Wprk out direction of row.
      if(at[1] == rect.Range2().Max()) {
	if(at[0] == rect.Range1().Max()) {
	  ok = false;
	  return false;
	}
	at[0]++;
	return true;
      }
      if(at[0] == rect.Range1().Min()) {
	at[1]++;
	return true;
      }
      at[0]--;
      at[1]++;
    } else {
      if(at[0] == rect.Range1().Max()) {
	if(at[1] == rect.Range2().Max()) {
	  ok = false;
	  return false;
	}
	at[1]++;
	return true;
      }
      if(at[1] == rect.Range2().Min()) {
	at[0]++;
	return true;
      }
      at[0]++;
      at[1]--;
    }
    
    return true;
  }
}
