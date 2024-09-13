// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Robert Crida"
//! date="6/8/2003"
//! example=testOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#pragma once

#include "Ravl2/Optimise/Optimise.hh"
#include "Ravl2/Optimise/OptimiseBrent.hh"

namespace Ravl2
{

  //! @brief Powell's (roughly) quadratically convergent non-gradient optimiser. This is also know as 'Bobyqa'
  //! Optimisation algorithm that does not use gradient but has roughly quadratic
  //! convergence.

  class OptimisePowell
   : public Optimise
  {    
  public:
    //! Constructor requires the number of iterations to use
    OptimisePowell (unsigned iterations, RealT tolerance, bool useBracketMinimum = true,bool verbose = false);

    //! Construct from a configuration
    explicit OptimisePowell(Configuration &config);

    //: Get iterations
    [[nodiscard]] unsigned numIterations() const {return this->_iterations;}

    //: Get tolerance
    [[nodiscard]] RealT tolerance() const {return this->_tolerance;}

    //! @brief Determines Xmin=arg min_{X} domain(X)
    //! Powell optimiser. Keeps a set of orthogonal directions and searches along
    //! each one in turn for the minimum. The final point is then used to create
    //! a new direction which replaces one of the existing ones and the process is
    //! repeated.
    //! @param  domain      - the cost function that will be minimised
    //! @param  func - The function to be minimised
    //! @param  start - The starting point for the optimisation, if empty then the midpoint of the domain is used
    //! @return  A tuple with the X value which gives the minimum cost, and the minimum cost value
    //!

    [[nodiscard]] std::tuple<VectorT<RealT>,RealT> minimise (
      const CostDomain &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start
    ) const final;

  private:

    static std::tuple<RealT,RealT> SetupLimits(const VectorT<RealT> &dir,const VectorT<RealT> &P,const CostDomain &domain);

      unsigned _iterations = 100;
    RealT _tolerance = RealT(1e-4);
    unsigned _brentIterations = 100;
    RealT _brentTolerance = RealT(1e-4);
    OptimiseBrent _brent;
    bool _useBracketMinimum = true;
  };
  
}
