// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_BRACKETMINIMUM_HH
#define RAVL_BRACKETMINIMUM_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! lib=Optimisation
//! date="8/8/2003"
//! userlevel=Normal
//! example=testCost.cc
//! file="Ravl/PatternRec/Optimise/BracketMinimum.hh"
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Cost Functions"
//! rcsid="$Id$"

#include "Ravl/PatternRec/CostFunction1d.hh"

namespace RavlN {

  void BracketMinimum (CostFunction1dC &cost);
    //: Updates the cost function parameter range to bracket a minimum
      
}

#endif
