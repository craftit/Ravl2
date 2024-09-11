// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISEPOWELL_HH
#define RAVL_OPTIMISEPOWELL_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! date="6/8/2003"
//! example=testOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#include "Ravl2/PatternRec/Optimise.hh"
#include "Ravl2/PatternRec/OptimiseBrent.hh"

namespace Ravl2 {

  // --------------------------------------------------------------------------
  // **********  OptimisePowellBodyC  *****************************************
  // --------------------------------------------------------------------------
  //: Powell's (roughly) quadratically convergent non-gradient optimiser
  //: implementation class.
  //
  // This is the implementation class of the non-gradient Powell optimiser for the
  // PatternRec toolbox. The OptimisePowellC handle class should be used.
  
  class OptimisePowellBodyC
   : public OptimiseBodyC
  {    
  public:
    OptimisePowellBodyC (unsigned iterations, RealT tolerance, bool useBracketMinimum,bool verbose = false);
    //: Constructor requires the number of iterations to use
    
    OptimisePowellBodyC (const XMLFactoryContextC & factory);
    //: Factory constructor

    OptimisePowellBodyC (std::istream &in);
    //: Constructs from stream

    //ACCESSOR METHODS:
    unsigned GetNumIterations(void) const {return this->_iterations;}
    //: Get iterations

    RealT GetTolerance(void) const {return this->_tolerance;}
    //: Get tolerance

    void SetVerbose(bool verbose)
    { _verbose = verbose; }
    //: Set the verbose flag

  protected:
    VectorT<RealT> MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const std::string GetInfo () const;
    //: Prints information about the optimiser
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
   
  private:
    unsigned _iterations;
    RealT _tolerance;
    unsigned _brentIterations;
    RealT _brentTolerance;
    OptimiseBrentC _brent;
    bool _useBracketMinimum;
    bool _verbose; 
  };
  
  //: Powell's (roughly) quadratically convergent non-gradient optimisation
  //: algorithm
  //
  // Optimisation algorithm that does not use gradient but has roughly quadratic
  // convergence.
  
  class OptimisePowellC: public OptimiseC
  {
  public:
    OptimisePowellC (unsigned iterations, RealT tolerance = 1e-6, bool useBracketMinimum = true,bool verbose = false)
      : OptimiseC(new OptimisePowellBodyC (iterations, tolerance, useBracketMinimum,verbose))
    {}
    //: Constructor
    //! @param  iterations - maximum number of iterations to use

    OptimisePowellC (const XMLFactoryContextC & factory)
      : OptimiseC(new OptimisePowellBodyC (factory))
    {}

    //ACCESSOR METHODS:

    unsigned GetNumIterations(void)
    {return Body().GetNumIterations();}
    //: Get iterations

    RealT GetTolerance(void)
    {return Body().GetTolerance();}
    //: Get tolerance

    void SetVerbose(bool verbose)
    { return Body().SetVerbose(verbose); }
    //: Set the verbose flag.

  protected:
    OptimisePowellBodyC &Body()
    { return static_cast<OptimisePowellBodyC &>(OptimiseC::Body()); }
    //: Access body.

    const OptimisePowellBodyC &Body() const
    { return static_cast<const OptimisePowellBodyC &>(OptimiseC::Body()); }
    //: Access body.

  };
}

#endif
