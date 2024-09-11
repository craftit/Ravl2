// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/OptimiseSobol.cc"

#include "Ravl/PatternRec/OptimiseSobol.hh"
#include "Ravl/SobolSequence.hh"
#include "Ravl/StrStream.hh"

namespace RavlN {

  OptimiseSobolBodyC::OptimiseSobolBodyC (UIntT numSamples)
    :OptimiseBodyC("OptimiseSobolBodyC"),
     _numSamples(numSamples)
  {
  }
  
  OptimiseSobolBodyC::OptimiseSobolBodyC (std::istream &in)
    :OptimiseBodyC("OptimiseSobolBodyC",in)
  {
    in >> _numSamples;
  }
  
  // Random optimizer with uniform density. Randomly samples the parameter
  // space to find the minimum cost position.
  //
  VectorC OptimiseSobolBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    VectorC X0 = domain.StartX().Copy();
    VectorC minX = domain.MinX();
    VectorC maxX = domain.MaxX();
    int Xdim = minX.Size();
    SobolSequenceC sobolSequence (Xdim);
    VectorC X (Xdim);
    RealT currentCost = domain.Cost (X0);    // Cost of starting point
    VectorC currentX = X0.Copy();            // Best point begins as start point
    for (UIntT i = 0; i < _numSamples; i++, sobolSequence++) {// For all the samples
      SArray1dC<RealT> nextValue = sobolSequence.Data();
      for (IndexC index = 0; index < Xdim; index++)   // Generate random param vector
	X[index] = minX[index] + nextValue[index] * (maxX[index]-minX[index]);
      RealT stepCost = domain.Cost (X);      // Evaluate cost at that point
      if (currentCost > stepCost) {          // If best then remember it
	currentCost = stepCost;
	currentX = X.Copy();
      }
    }
    minimumCost = currentCost;
    return domain.ConvertX2P (currentX);     // Return final estimate
  }
  
  const StringC OptimiseSobolBodyC::GetInfo () const
  {
    StrOStreamC stream;
    stream << OptimiseBodyC::GetInfo () << "\n";
    stream << "Sobol parameter space search optimisation using " << _numSamples << "samples.";
    return stream.String();
  }
  
  bool OptimiseSobolBodyC::Save (std::ostream &out) const
  {
    OptimiseBodyC::Save (out);
    out << _numSamples << "\n";
    return true;
  }
  
}
