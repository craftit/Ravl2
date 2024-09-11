// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Robert Crida"
//! date="06/8/2003"
//! example=testBrent.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#pragma once

#include "Ravl2/Optimise/Optimise.hh"

namespace Ravl2 {

  //! @brief Parabolic Interpolation and Brent's method in One Dimension implementation class.
  //! Optimisation algorithm based on Parabolic Interpolation and Brent's method
  //! in One Dimension. It does not use gradient and therefore does not need to call
  //! Jacobian on the cost function. Note also that this optimiser only works for
  //! cost functions of one dimension.

  class OptimiseBrent
  {
  public:
    using RealT = double;

    //! Constructor
    //! @param  iterations - maximum number of iterations to use
    //! @param  tolerance  - ending tolerance
    OptimiseBrent (unsigned iterations, RealT tolerance);

    //! This optimisation algorithm only works for cost functions of one dimension.
    //! @param  minP - the minimum value of the domain
    //! @param  maxP - the maximum value of the domain
    //! @param  func - The function to be minimised
    //! @return  A tuple with the X value which gives the minimum cost, and the minimum cost value
    std::tuple<RealT,RealT> minimise(
      RealT minP,
      RealT maxP,
      RealT startAt,
      RealT initialCost,
      const std::function<RealT(RealT val)> &func
    ) const;

  private:
    unsigned _iterations = 100;
    RealT _tolerance = 1.0e-4;
  };
  
}
