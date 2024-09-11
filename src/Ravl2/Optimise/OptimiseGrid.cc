// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/OptimiseGrid.hh"
#include "Ravl2/Random.hh"
#include "Ravl2/StrStream.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

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
  VectorT<RealT> OptimiseGridBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    VectorT<RealT> X0 = domain.StartX();
    VectorT<RealT> minX = domain.MinX();
    VectorT<RealT> maxX = domain.MaxX();
    IntT totalSamples = 1;
    std::vector<IntT> steps = domain.GetParameters().Steps();
    
    for(SArray1dIterC<IntT> it(steps);it;it++) {
      RavlAssertMsg(*it >= 1,"OptimiseGridBodyC::MinimalX, WARNING: Need a positive step size for search. ");
      totalSamples *= *it;
    }
    int Xdim = minX.size();
    VectorT<RealT> X (Xdim);
    RealT currentCost = domain.Cost (X0);    // Cost of starting point
    VectorT<RealT> currentX = X0;                   // Best point begins as start point
    for (IntT i = 0; i <totalSamples; i++) {// For all the samples
      IntT temp=i;
      for (int index = 0; index < Xdim; index++) {  // Generate parameter vector
	X[index] = minX[index] + (temp % steps[index]) * (maxX[index]-minX[index]) / (steps[index]-1);
	temp /=steps[index];
      }
      
      RealT stepCost = domain.Cost (X);      // Evaluate cost at that point
      ONDEBUG(std::cerr << "At=" << X << " Cost=" << stepCost << "\n");
      if (currentCost > stepCost) {          // If best then remember it
	currentCost = stepCost;
	currentX = X.Copy();
      }
    }
    minimumCost = currentCost;
    return domain.ConvertX2P (currentX);     // Return final estimate
  }
  
  const std::string OptimiseGridBodyC::GetInfo () const
  {
    Strstd::unique_ptr<std::ostream> stream;
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
