// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
/////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! file="Ravl/Math/Sequence/CircleIter.hh"
//! lib=RavlMath
//! userlevel=Normal
//! author="Charles Galambos"
//! docentry="Ravl.API.Math.Sequences"
//! date="08/02/1999"

#pragma once

#include "Ravl2/Index.hh"

namespace Ravl2
{
  //: Iterate around a circle with integer coordinates.
  // NB. This does NOT move linearly around the circle,
  // but through octants.
  
  class CircleIterC
  {
  public:
    //: Constructor.
    CircleIterC(int nradius = 1,Index<2> nOffset = Index<2>(0,0))
      : radius(nradius),
	offset(nOffset)
    { First(); }

    void First();
    //: Goto first point on circle.
    
    [[nodiscard]] inline bool IsElm() const { return (octant > -1); }
    //: At valid position ?

    [[nodiscard]] operator bool() const
    { return IsElm(); }
    //: At a valid position ?
    
    [[nodiscard]] inline const Index<2> &Data() const { return data; }
    //: Get point.
    // Largest error from radius should be less than 0.5

    [[nodiscard]] const Index<2> &operator*() const
    { return data; }
    //: Get current point.
    
    bool Next();
    //: Goto next point.
    // Returns true if we're now at a valid point.
    
    bool operator++(int)
    { return Next(); }
    //: Goto next point.
    // Returns true if we're now at a valid point.
  private:
    //:
    int octant = 0; // Current octant.
    int radius = 1;
    int d = 0,deltaE = 0,deltaSE = 0;
    int x= 0;
    int y = 0;
    Index<2> offset {};
    Index<2> data {};
  };
}  
