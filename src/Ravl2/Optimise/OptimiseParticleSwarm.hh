// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OptimiseParticleSwarm_HH
#define RAVL_OptimiseParticleSwarm_HH
////////////////////////////////////////////////////////////////////////////
//! author="Kieron Messer"
//! lib=Optimisation
//! date="22/4/1998"
//! userlevel=Normal
//! example=testNumOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Implementation"
//! rcsid="$Id$"
//! file="Ravl/PatternRec/Optimise/OptimiseParticleSwarm.hh"

#include "Ravl/PatternRec/Optimise.hh"

namespace RavlN {

  // --------------------------------------------------------------------------
  // **********  OptimiseParticleSwarmBodyC  **********************************
  // --------------------------------------------------------------------------
  //: Particle swarm optimisation (aka PSO)
  //
  
  class OptimiseParticleSwarmBodyC : public OptimiseBodyC
  {

  public:
    OptimiseParticleSwarmBodyC(UIntT numberOfParticles,
        RealT omega,
        RealT phiPosition,
        RealT phiGlobal,
        UIntT numberOfIterations,
        RealT terminationCriterion,
        UIntT numberOfThreads);
    //: Default constructor
    //!param: numSamples - number of random samples to check
    
    OptimiseParticleSwarmBodyC(std::istream &in);
    //: Constructs from stream
    
  protected:
    VectorC MinimalX(const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} |f(X)-Yd|
    
    virtual const StringC GetInfo() const;
    //: Prints information about the optimizer
    
    virtual bool Save(std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor

    UIntT m_numberOfParticles;
    RealT m_omega;
    RealT m_phiP;
    RealT m_phiG;
    UIntT m_numberOfIterations;
    RealT m_terminationCriterion;
    UIntT m_numberOfThreads;

  };
  
  //: Uniform distribution random search optimisation.
  //
  // Class for performing a random search using a uniform distribution.
  
  class OptimiseParticleSwarmC : public OptimiseC
  {
  public:
    OptimiseParticleSwarmC(UIntT numberOfParticles,
        RealT omega,
        RealT phiParticle,
        RealT phiGlobal,
        UIntT numberOfIterations,
        RealT terminationCriterion = 1e-6,
        UIntT numberOfThreads = 1) :
        OptimiseC(*(new OptimiseParticleSwarmBodyC(numberOfParticles, omega, phiParticle, phiGlobal, numberOfIterations, terminationCriterion, numberOfThreads)))
    {
    }
    //: Constructor
    //!param: numberOfParticles The number of particles to start with
    //!param: numberOfThreads The number of threads to use
  };

}

#endif

