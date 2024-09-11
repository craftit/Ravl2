// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISEBRENT_HH
#define RAVL_OPTIMISEBRENT_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! lib=Optimisation
//! date="06/8/2003"
//! userlevel=Normal
//! example=testBrent.cc
//! file="Ravl/PatternRec/Optimise/OptimiseBrent.hh"
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"
//! rcsid="$Id$"

#include "Ravl/PatternRec/Optimise.hh"

namespace RavlN {

  // --------------------------------------------------------------------------
  // **********  OptimiseBrentBodyC  ****************************************
  // --------------------------------------------------------------------------
  //: Parabolic Interpolation and Brent's method in One Dimension implementation class.
  //
  // This is the implementation class of the Parabolic Interpolation and Brent's
  // method in One Dimension optimiser for the PatternRec toolbox. The 
  // OptimiseBrentC handle class should be used.
  
  class OptimiseBrentBodyC: public OptimiseBodyC
  {
    UIntT _iterations;
    RealT _tolerance;
    
  public:
    OptimiseBrentBodyC (UIntT iterations, RealT tolerance);
    //: Constructor requires the number of iterations to use
    
    OptimiseBrentBodyC (std::istream &in);
    //: Constructs from stream
    
  protected:
    VectorC MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|

    virtual const StringC GetInfo () const;
    //: Prints information about the optimiser
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
  };
  
  //: Parabolic Interpolation and Brent's method in One Dimension.
  //
  // Optimisation algorithm based on Parabolic Interpolation and Brent's method 
  // in One Dimension. It does not use gradient and therefore does not need to call
  // Jacobian on the cost function. Note also that this optimiser only works for
  // cost functions of one dimention.
  
  class OptimiseBrentC: public OptimiseC
  {
  public:
    OptimiseBrentC ()
    {}
    //: Default constructor makes invalid handle

    OptimiseBrentC (UIntT iterations, RealT tolerance = 1e-6)
      :OptimiseC(*(new OptimiseBrentBodyC (iterations, tolerance))) {}
    //: Constructor
    //!param: iterations - maximum number of iterations to use
    //!param: tolerance  - ending tolerance
  };
}

#endif
