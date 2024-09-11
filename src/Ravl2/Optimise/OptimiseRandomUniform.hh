// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISERANDOMUNIFORM_HH
#define RAVL_OPTIMISERANDOMUNIFORM_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! lib=Optimisation
//! date="22/4/1998"
//! userlevel=Normal
//! example=testNumOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"
//! rcsid="$Id$"
//! file="Ravl/PatternRec/Optimise/OptimiseRandomUniform.hh"

#include "Ravl/PatternRec/Optimise.hh"

namespace RavlN {

  // --------------------------------------------------------------------------
  // **********  OptimiseRandomUniformBodyC  **********************************
  // --------------------------------------------------------------------------
  //: Uniform distribution random search optimiser implementation class.
  //
  // This is the implementation class of the uniform distribution random search
  // optimiser for the PatternRec toolbox. The NumOptimiseRandomUniformC handle
  // class should be used.
  
  class OptimiseRandomUniformBodyC: public OptimiseBodyC
  {
    UIntT _numSamples;

  public:
    OptimiseRandomUniformBodyC (UIntT numSamples);
    //: Default constructor
    //!param: numSamples - number of random samples to check
    
    OptimiseRandomUniformBodyC (std::istream &in);
    //: Constructs from stream
    
  protected:
    VectorC MinimalX (const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const StringC GetInfo () const;
    //: Prints information about the optimizer
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
  };
  
  //: Uniform distribution random search optimisation.
  //
  // Class for performing a random search using a uniform distribution.
  
  class OptimiseRandomUniformC: public OptimiseC
  {
  public:
    OptimiseRandomUniformC (UIntT numSamples)
      :OptimiseC(*(new OptimiseRandomUniformBodyC (numSamples))) {}
    //: Constructor
    //!param: numSamples - number of random samples to check
  };
  
}

#endif

