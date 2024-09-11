// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISECONJUGATEGRADIENT_HH
#define RAVL_OPTIMISECONJUGATEGRADIENT_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida and Charles Galambos"
//! lib=Optimisation
//! date="28/7/2006"
//! userlevel=Normal
//! example=testOptimise.cc
//! file="Ravl/PatternRec/Optimise/OptimiseConjugateGradientDescent.hh"
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"

#include "Ravl/PatternRec/Optimise.hh"

namespace RavlN {

  // --------------------------------------------------------------------------
  // **********  OptimiseConjugateGradientDescentBodyC  ****************************************
  // --------------------------------------------------------------------------
  //! userlevel=Develop
  //: Gradient descent optimiser implementation class.
  //
  // This is the implementation class of the gradient descent optimiser for the
  // PatternRec toolbox. The OptimiseConjugateGradientC handle class should be used.
  
  class OptimiseConjugateGradientBodyC
    : public OptimiseBodyC
  {
  public:
    OptimiseConjugateGradientBodyC(const XMLFactoryContextC & factory);
    //: Constructor from xml factory.

    OptimiseConjugateGradientBodyC (UIntT iterations, RealT tolerance,bool useBacketMinimum = true,bool useAbsoluteCostForTolerance = false,UIntT brentIterations = 0, RealT brentTolerance = 0);
    //: Constructor requires the number of iterations to use
    
    OptimiseConjugateGradientBodyC (std::istream &in);
    //: Constructs from stream
    
    virtual RCBodyVC &Copy() const;
    //: Create copy of the optimiser

  protected:
    VectorC MinimalX (const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const StringC GetInfo () const;
    //: Prints information about the optimiser
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
    
  private:
    UIntT _iterations;
    RealT _tolerance;
    UIntT _brentIterations;
    RealT _brentTolerance;
    bool _useBracketMinimum;
    bool m_useAbsoluteCostForTolerance;
  };
  
  //! userlevel=Normal
  //: Gradient descent optimisation algorithm.
  //
  // Optimisation algorithm based on gradient descent. The cost function provides
  // a measure of the Jacobian analytically if possible so the performance
  // should be reasonable.
  
  class OptimiseConjugateGradientC: public OptimiseC
  {
  public:
    OptimiseConjugateGradientC()
    {}
    //: Create invalid handle.

    OptimiseConjugateGradientC(const XMLFactoryContextC & factory)
     : OptimiseC(*(new OptimiseConjugateGradientBodyC (factory)))
    {}
    //: XML Factory constructor

    OptimiseConjugateGradientC(UIntT iterations,RealT tolerance = 1e-6,
        bool useBacketMinimum = true,bool useAbsoluteCostForTolerance = false,
        UIntT brentIterations = 0, RealT brentTolerance = 0)
      : OptimiseC(*(new OptimiseConjugateGradientBodyC (iterations, tolerance,useBacketMinimum,useAbsoluteCostForTolerance,brentIterations,brentTolerance)))
    {}
    //: Constructor
    //!param: iterations - maximum number of iterations to use
    // Searches along direction of Jacobian (steepest descent) with ever
    // decreasing steps until cost function decreases. This is one iteration.
    // Iterates until the Jacobian is very small or number of iterations is met.
  };

  void LinkOptimiseConjugateGradient();

}

#endif
