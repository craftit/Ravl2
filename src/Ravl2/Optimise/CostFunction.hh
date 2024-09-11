// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_COSTFUNCTION_HH
#define RAVL_COSTFUNCTION_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! lib=Optimisation
//! date="22/4/1998"
//! userlevel=Normal
//! example=testCost.cc
//! file="Ravl/PatternRec/Optimise/CostFunction.hh"
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Cost Functions"

#include "Ravl/PatternRec/Cost.hh"
#include "Ravl/PatternRec/Distance.hh"

namespace RavlN {

  // --------------------------------------------------------------------------
  // **********  CostFunctionBodyC  *******************************************
  // --------------------------------------------------------------------------
  //: Implementation class for cost functions interfacing to NumFuncC.
  //
  // This is the implementation class for cost functions which interface to
  // functions derived from NumFuncC. The NumCostFunctionC handle class should
  // be used.
  
  class CostFunctionBodyC: public CostBodyC
  {
  public:
    CostFunctionBodyC (const ParametersC &parameters,
		       const VectorC &Yd, 
		       const FunctionC &function,
		       const DistanceC &metric);
    //: Constructor
    //!param: parameters - describes which parameters to use for optimisation
    //!param: Yd         - the desired output from the function
    //!param: function   - the function to be optimised
    //!param: metric     - a method for calculating the size of |f(X)-Yd|
    
    CostFunctionBodyC (std::istream &in);
    //: Contructs from stream
    
    virtual RealT Cost (const VectorC &X) const;
    //: Evaluate cost function at X
    
    virtual MatrixC Jacobian (const VectorC &X) const;
    //: Calculate Jacobian matrix at X
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
    
  protected:
    VectorC _Yd;
    FunctionC _function;
    DistanceC _metric;
  };
  
  
  //: Optimisation cost function interface to functions derived from NumFuncC.
  //
  // Provides an interface to allow numerical optimisation with any class that
  // is derived from NumFuncC. Some degree of constrained optimisation can be
  // achieved in that the NumParametersC object specifies which inputs to the
  // function can be varied and which are fixed. The value of the fixed
  // parameters can be specified.
  
  class CostFunctionC: public CostC
  {
  public:
    CostFunctionC (const ParametersC &parameters,
		   const VectorC &Yd, 
		   const FunctionC &function,
		   const DistanceC &metric)
      :CostC(*(new CostFunctionBodyC (parameters,Yd,function,metric))) {}
    //: Constructor
    //!param: parameters - describes which parameters to use for optimisation
    //!param: Yd         - the desired output from the function
    //!param: function   - the function to be optimised
    //!param: metric     - a method for calculating the size of |f(X)-Yd|
  };

}

#endif
