// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISEDESCENT_HH
#define RAVL_OPTIMISEDESCENT_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! date="22/4/1998"
//! example=testOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#include "Ravl2/PatternRec/Optimise.hh"

namespace Ravl2 {

  // --------------------------------------------------------------------------
  // **********  OptimiseDescentBodyC  ****************************************
  // --------------------------------------------------------------------------
  //: Gradient descent optimiser implementation class.
  //
  // This is the implementation class of the gradient descent optimiser for the
  // PatternRec toolbox. The OptimiseDescentC handle class should be used.
  
  class OptimiseDescentBodyC: public OptimiseBodyC
  {
  public:
    OptimiseDescentBodyC(const XMLFactoryContextC & factory);
    //: Constructor from xml factory.

    OptimiseDescentBodyC (unsigned iterations, RealT tolerance);
    //: Constructor requires the number of iterations to use
    
    OptimiseDescentBodyC (std::istream &in);
    //: Constructs from stream
    
  protected:
    VectorT<RealT> MinimalX (const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const std::string GetInfo () const;
    //: Prints information about the optimiser
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
    
  private:
    unsigned _iterations;
    RealT _tolerance;
    
  };
  
  //: Gradient descent optimisation algorithm.
  //
  // Optimisation algorithm based on gradient descent. The cost function provides
  // a measure of the Jacobian analytically if possible so the performance
  // should be reasonable.
  
  class OptimiseDescentC: public OptimiseC
  {
  public:
    OptimiseDescentC()
    {}
    //: Default constructor, creates an invalid handle

    OptimiseDescentC(const XMLFactoryContextC & factory)
     : OptimiseC(*(new OptimiseDescentBodyC (factory)))
    {}
    //: XML Factory constructor

    OptimiseDescentC (unsigned iterations, RealT tolerance = 1e-6)
      :OptimiseC(*(new OptimiseDescentBodyC (iterations, tolerance)))
    {}
    //: Constructor
    //! @param  iterations - maximum number of iterations to use
    // Searches along direction of Jacobian (steepest descent) with ever
    // decreasing steps until cost function decreases. This is one iteration.
    // Iterates until the Jacobian is very small or number of iterations is met.
  };
}

#endif
