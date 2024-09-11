// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISELEVENBERGMARQUARDT_HH
#define RAVL_OPTIMISELEVENBERGMARQUARDT_HH
////////////////////////////////////////////////////////////////////////////
//! author="Charles Galambos"
//! date="28/7/2006"
//! example=testOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#include "Ravl2/PatternRec/Optimise.hh"

namespace Ravl2 {

  // --------------------------------------------------------------------------
  // **********  OptimiseLevenbergMarquardtDescentBodyC  ****************************************
  // --------------------------------------------------------------------------
  //: Levenberg-Marquardt optimiser implementation class.
  //
  // This is the implementation class of the gradient descent optimiser for the
  // PatternRec toolbox. The OptimiseLevenbergMarquardtC handle class should be used.
  
  class OptimiseLevenbergMarquardtBodyC: public OptimiseBodyC
  {
  public:
    OptimiseLevenbergMarquardtBodyC (unsigned iterations, RealT tolerance);
    //: Constructor requires the number of iterations to use
    
    OptimiseLevenbergMarquardtBodyC (std::istream &in);
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
  
  //: Levenberg-Marquardt optimisation algorithm.
  //
  // This method requires the cost function is being reducted towards zero, and that
  // a reasonable estimation of the hessian is available. 
  
  class OptimiseLevenbergMarquardtC: public OptimiseC
  {
  public:
    OptimiseLevenbergMarquardtC (unsigned iterations, RealT tolerance = 1e-6)
      : OptimiseC(*(new OptimiseLevenbergMarquardtBodyC (iterations, tolerance))) 
    {}
    //: Constructor
    //! @param  iterations - maximum number of iterations to use
    // Searches along direction of Jacobian (steepest descent) with ever
    // decreasing steps until cost function decreases. This is one iteration.
    // Iterates until the Jacobian is very small or number of iterations is met.
  };
}

#endif
