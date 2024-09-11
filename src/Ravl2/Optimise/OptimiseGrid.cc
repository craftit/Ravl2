// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/OptimiseGrid.cc"

#include "Ravl/PatternRec/OptimiseGrid.hh"
#include "Ravl/Random.hh"
#include "Ravl/StrStream.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlN {

  OptimiseGridBodyC::OptimiseGridBodyC ()
    :OptimiseBodyC("OptimiseGridBodyC")
  {
  }
  
  OptimiseGridBodyC::OptimiseGridBodyC (std::istream &in)
    :OptimiseBodyC("OptimiseGridBodyC",in)
  {
  }
  
  // ------------------------------------------------------------------------
  // **********  OptimalX    ************************************************
  // ------------------------------------------------------------------------
  //
  // Grid optimiser.  Uniformly sample search space in each direction
  // Can quickly get out of hand
  //
  VectorC OptimiseGridBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    VectorC X0 = domain.StartX();
    VectorC minX = domain.MinX();
    VectorC maxX = domain.MaxX();
    IntT totalSamples = 1;
    SArray1dC<IntT> steps = domain.GetParameters().Steps();
    
    for(SArray1dIterC<IntT> it(steps);it;it++) {
      RavlAssertMsg(*it >= 1,"OptimiseGridBodyC::MinimalX, WARNING: Need a positive step size for search. ");
      totalSamples *= *it;
    }
    int Xdim = minX.Size();
    VectorC X (Xdim);
    RealT currentCost = domain.Cost (X0);    // Cost of starting point
    VectorC currentX = X0;                   // Best point begins as start point
    for (IntT i = 0; i <totalSamples; i++) {// For all the samples
      IntT temp=i;
      for (IndexC index = 0; index < Xdim; index++) {  // Generate parameter vector
	X[index] = minX[index] + (temp % steps[index]) * (maxX[index]-minX[index]) / (steps[index]-1);
	temp /=steps[index];
      }
      
      RealT stepCost = domain.Cost (X);      // Evaluate cost at that point
      ONDEBUG(cerr << "At=" << X << " Cost=" << stepCost << "\n");
      if (currentCost > stepCost) {          // If best then remember it
	currentCost = stepCost;
	currentX = X.Copy();
      }
    }
    minimumCost = currentCost;
    return domain.ConvertX2P (currentX);     // Return final estimate
  }
  
  const StringC OptimiseGridBodyC::GetInfo () const
  {
    StrOStreamC stream;
    stream << OptimiseBodyC::GetInfo () << "\n";
    stream << "Grid optimiser.";
    return stream.String();
  }
  
  bool OptimiseGridBodyC::Save (std::ostream &out) const
  {
    OptimiseBodyC::Save (out);
    return true;
  }
  
}
