// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISEGRID_HH
#define RAVL_OPTIMISEGRID_HH
////////////////////////////////////////////////////////////////////////////
//! author="Kieron Messer"
//! date="6/9/2003"
//! example=testNumOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#include "Ravl2/PatternRec/Optimise.hh"

namespace Ravl2 {

  // --------------------------------------------------------------------------
  // **********  OptimiseGridBodyC  **********************************
  // --------------------------------------------------------------------------
  //: Uniform distribution random search optimiser implementation class.
  //
  // This is the implementation class of the uniform distribution random search
  // optimiser for the PatternRec toolbox. The NumOptimiseGridC handle
  // class should be used.
  
  class OptimiseGridBodyC: public OptimiseBodyC
  {
  public:
    OptimiseGridBodyC ();
    //: Default constructor
    //! @param  numSamples - number of random samples to check
    
    OptimiseGridBodyC (std::istream &in);
    //: Constructs from stream
    
  protected:
    VectorT<RealT> MinimalX (const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const std::string GetInfo () const;
    //: Prints information about the optimizer
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
  };
  
  //: Uniform distribution random search optimisation.
  //
  // Class for performing a random search using a uniform distribution.
  
  class OptimiseGridC: public OptimiseC
  {
  public:
    OptimiseGridC ()
      :OptimiseC(*(new OptimiseGridBodyC ())) {}
    //: Constructor
    //! @param  numSamples - number of random samples to check
  };
  
}

#endif

