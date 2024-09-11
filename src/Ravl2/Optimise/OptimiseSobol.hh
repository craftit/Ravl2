// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISESOBOL_HH
#define RAVL_OPTIMISESOBOL_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! lib=Optimisation
//! date="22/4/1998"
//! userlevel=Normal
//! example=testNumOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"
//! rcsid="$Id$"
//! file="Ravl/PatternRec/Optimise/OptimiseSobol.hh"

#include "Ravl/PatternRec/Optimise.hh"

namespace RavlN {

  //! userlevel=Develop
  //: Sobol distribution search optimiser implementation class.
  //
  // This is the implementation class of the sobol distribution search
  // optimiser for the PatternRec toolbox. The OptimiseSobolC handle
  // class should be used.
  
  class OptimiseSobolBodyC: public OptimiseBodyC
  {
    UIntT _numSamples;

  public:
    OptimiseSobolBodyC (UIntT numSamples);
    //: Default constructor
    //!param: numSamples - number of samples to check
    
    OptimiseSobolBodyC (std::istream &in);
    //: Constructs from stream
    
  protected:
    VectorC MinimalX (const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const StringC GetInfo () const;
    //: Prints information about the optimizer
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
  };

  //! userlevel=Normal
  //: Sobol distribution search optimisation.
  //
  // Class for performing a search using a sobol distribution.
  
  class OptimiseSobolC: public OptimiseC
  {
  public:
    OptimiseSobolC (UIntT numSamples)
      :OptimiseC(*(new OptimiseSobolBodyC (numSamples))) {}
    //: Constructor
    //!param: numSamples - number of samples to check
  };
  
}

#endif

