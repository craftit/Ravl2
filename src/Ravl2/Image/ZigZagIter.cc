// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Image/ZigZagIter.hh"

namespace Ravl2 {
  
  //: Constuctor
  
  ZigZagIterC::ZigZagIterC(const IndexRange<2> &nrect)
    : at(nrect.min()),
      rect(nrect),
      ok(true)
  {
    if(nrect.range(0).size() != nrect.range(1).size()) {
      ok = false;
      throw std::runtime_error("ZigZagIterC: Rectangle must be square.");
    }
    //cerr << "Init=" << at << " \n";
    if(rect.area() == 0)
      ok = false; 
  }
  
  //: Goto first index in sequence.
  
  bool ZigZagIterC::First() {
    if(rect.area() == 0)
      return false;
    at = rect.min();
    return true;
  }

  //: Goto next pixel in scan pattern.
  
  bool ZigZagIterC::Next() {
    //cerr << "Current=" << at << "\n";
    int row = at[0] + at[1];
    if((row % 2) == 0) { // Wprk out direction of row.
      if(at[1] == rect.range(1).max()) {
	if(at[0] == rect.range(0).max()) {
	  ok = false;
	  return false;
	}
	at[0]++;
	return true;
      }
      if(at[0] == rect.range(0).min()) {
	at[1]++;
	return true;
      }
      at[0]--;
      at[1]++;
    } else {
      if(at[0] == rect.range(0).max()) {
	if(at[1] == rect.range(1).max()) {
	  ok = false;
	  return false;
	}
	at[1]++;
	return true;
      }
      if(at[1] == rect.range(1).min()) {
	at[0]++;
	return true;
      }
      at[0]++;
      at[1]--;
    }
    
    return true;
  }
}
