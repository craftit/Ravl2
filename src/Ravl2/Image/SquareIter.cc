// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#include "Ravl2/Image/SquareIter.hh"
#include "Ravl2/Assert.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
  
  bool SquareIterC::Next() {
    ONDEBUG(std::cerr << "SquareIterC::Next(), State:" << state << "\n");
    switch(state) {
    case 1:  // From Center.
      if(maxSize <= 1) 
	break;
      at = centre + toIndex(-1,0); // UpN
      state++; 
      size = 1;
      end = at[1] + 1;
      return true;
    case 2: // Along top to the right.
      at[1]++;
      if(at[1] < end) 
	return true;
      state++;
      end = at[0] + (size * 2);
      return true;
    case 3: // Down the left side.
      at[0]++;
      if(at[0] < end)
	return true;
      state++;
      end = at[1] - (size * 2);
      return true;
    case 4: // Back along the bottom.
      at[1]--;
      if(at[1] > end)
	return true;
      state++;
      end = at[0] - ((size * 2)+1);
      return true;
    case 5: // Up the righ hand side.
      at[0]--;
      if(at[0] > end)
	return true;
      size++;
      if(size >= maxSize)
	break; // Done!
      at = centre;
      at[0] -= size;
      at[1] -= size;
      end = at[1] + (size * 2);
      state = 2;
      return true;
    default:
      RavlAlwaysAssertMsg(false,"SquareIterC::Next(), ERROR: Illegal state. ");
      break;
    }
    state = 0;
    return false;
  }
  
}
