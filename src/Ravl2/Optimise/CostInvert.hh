// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_COSTINVERT_HH
#define RAVL_COSTINVERT_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! lib=Optimisation
//! date="22/4/1998"
//! userlevel=Normal
//! example=testCost.cc
//! file="Ravl/PatternRec/Optimise/CostInvert.hh"
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Cost Functions"
//! rcsid="$Id$"

#include "Ravl/PatternRec/Cost.hh"

namespace RavlN {
  // --------------------------------------------------------------------------
  // **********  CostInvertBodyC  **********************************************
  // --------------------------------------------------------------------------
  //: Implementation class for inverting cost functions.
  //
  // This is the implementation class for cost functions which invert another
  // cost function. The NumCostInvertC handle class should be used.
  
  class CostInvertBodyC: public CostBodyC
  {
    CostC _cost;
  public:
    CostInvertBodyC (const CostC &cost);
    //: Constructor
    //!param: cost - cost function that will be inverted
    
    CostInvertBodyC (std::istream &in);
    //: Contructs from stream
    
  protected:
    virtual RealT Cost (const VectorC &X) const;
    //: Evaluate cost function at X
    
    virtual MatrixC Jacobian (const VectorC &X) const;
    //: Calculate Jacobian matrix at X
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
  };
  
  //: Cost function which inverts another cost function.
  //
  // Contains another cost function. When a cost estimate is required, the
  // internal cost function is evaluated and the cost is negated. It is used
  // to implement the maximiser using the minimisation optimisers by inverting
  // the cost function. Can be useful when creating combined cost functions
  // where some should decrease the overall cost.
  
  class CostInvertC: public CostC
  {
  public:
    CostInvertC (const CostC &cost)
      :CostC(*(new CostInvertBodyC (cost))) {}
    //: Constructor
    //!param: cost - cost function that will be inverted
  };

}

#endif
