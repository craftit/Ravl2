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
//! date="22/4/1998"
//! example=testNumOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#include "Ravl2/PatternRec/Optimise.hh"

namespace Ravl2 {

  //: Sobol distribution search optimiser implementation class.
  //
  // This is the implementation class of the sobol distribution search
  // optimiser for the PatternRec toolbox. The OptimiseSobolC handle
  // class should be used.
  
  class OptimiseSobolBodyC: public OptimiseBodyC
  {
    unsigned _numSamples;

  public:
    OptimiseSobolBodyC (unsigned numSamples);
    //: Default constructor
    //! @param  numSamples - number of samples to check
    
    OptimiseSobolBodyC (std::istream &in);
    //: Constructs from stream
    
  protected:
    VectorT<RealT> MinimalX (const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const std::string GetInfo () const;
    //: Prints information about the optimizer
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
  };

  //: Sobol distribution search optimisation.
  //
  // Class for performing a search using a sobol distribution.
  
  class OptimiseSobolC: public OptimiseC
  {
  public:
    OptimiseSobolC (unsigned numSamples)
      :OptimiseC(*(new OptimiseSobolBodyC (numSamples))) {}
    //: Constructor
    //! @param  numSamples - number of samples to check
  };
  
}

#endif

