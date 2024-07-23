// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Sequence/CircleIter.cc"

#include "Ravl2/Geometry/CircleIter.hh"

namespace Ravl2 
{
  
  void CircleIterC::First() {
    x = 0;
    y = radius;
    d = 1 - radius;
    deltaE = 3;
    deltaSE = -2 * radius + 5;
    data = Index<2>(x,y) + offset;
    switch(radius)
      {
      case 0:
        x = y;
        octant = 7;
        break;
            case 1:
        octant = 4;
        d += deltaSE;
        deltaE += 2;
        deltaSE += 4;
        y--;
        x++;
        data = Index<2>(x,y) + offset;
        break;
      default:
        octant = 0;
        break;
      }
  }
    
  bool CircleIterC::Next() {
    octant++;
    switch(octant)
      {
      case 8: // Calculate next. octant 0
	octant = 0;
	if(y <= x) {
	  octant = -1;
	  return false; // Finished !
	}
	if(d < 0) {
	  d += deltaE;
	  deltaE += 2;
	  deltaSE += 2;
	} else {
	  d += deltaSE;
	  deltaE += 2;
	  deltaSE += 4;
	  y--;
	}
	x++;
	data = Index<2>(x,y) + offset;
	break;
	
      case 1:
	if(x != 0) {
	  data = Index<2>(-x,-y) + offset;
	  break;
	}
	octant++;
	/* no break */
        FMT_FALLTHROUGH;
      case 2:
	data = Index<2>(y,-x) + offset;
	break;
      case 3:
	data = Index<2>(-y,x) + offset;
	if(x == y)
	  octant = 7; // Skip redundant cases.
	break;
	
	// Only done if x != y
      case 4:
	data = Index<2>(x,-y) + offset;
	if(x == 0)
	  octant = 7;
	break;
      case 5:
	data = Index<2>(y,x) + offset;
	break;
      case 6:
	data = Index<2>(-y,-x) + offset;
	break;      
      case 7:
	data = Index<2>(-x,y) + offset;
	break;
	
      default:
	assert(false && "Bad state.");
	break;
      }
    return true;
  }
}

