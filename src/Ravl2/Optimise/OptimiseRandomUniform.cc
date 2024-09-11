// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/OptimiseRandomUniform.cc"

#include "Ravl/PatternRec/OptimiseRandomUniform.hh"
#include "Ravl/Random.hh"
#include "Ravl/StrStream.hh"

namespace RavlN {

  OptimiseRandomUniformBodyC::OptimiseRandomUniformBodyC (UIntT numSamples)
    :OptimiseBodyC("OptimiseRandomUniformBodyC"),
     _numSamples(numSamples)
  {
  }
  
  OptimiseRandomUniformBodyC::OptimiseRandomUniformBodyC (std::istream &in)
    :OptimiseBodyC("OptimiseRandomUniformBodyC",in)
  {
    in >> _numSamples;
  }
  
  // ------------------------------------------------------------------------
  // **********  OptimalX    ************************************************
  // ------------------------------------------------------------------------
  //
  // Random optimizer with uniform density. Randomly samples the parameter
  // space to find the minimum cost position.
  
  VectorC OptimiseRandomUniformBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    VectorC X0 = domain.StartX();
    VectorC minX = domain.MinX();
    VectorC maxX = domain.MaxX();
    int Xdim = minX.Size();
    VectorC X (Xdim);
    RealT currentCost = domain.Cost (X0);    // Cost of starting point
    VectorC currentX = X0;                   // Best point begins as start point
    for (UIntT i = 0; i < _numSamples; i++) {// For all the samples
      for (IndexC index = 0; index < Xdim; index++)   // Generate random param vector
	X[index] = minX[index] + Random1() * (maxX[index]-minX[index]);
      RealT stepCost = domain.Cost (X);      // Evaluate cost at that point
      if (currentCost > stepCost) {          // If best then remember it
	currentCost = stepCost;
	currentX = X.Copy();
      }
    }
    minimumCost = currentCost;
    return domain.ConvertX2P (currentX);     // Return final estimate
  }
  
  const StringC OptimiseRandomUniformBodyC::GetInfo () const
  {
    StrOStreamC stream;
    stream << OptimiseBodyC::GetInfo () << "\n";
    stream << "Uniform random parameter space search optimisation using " << _numSamples << "samples.";
    return stream.String();
  }
  
  bool OptimiseRandomUniformBodyC::Save (std::ostream &out) const
  {
    OptimiseBodyC::Save (out);
    out << _numSamples << "\n";
    return true;
  }
  
}
