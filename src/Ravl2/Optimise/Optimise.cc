//
// Created by charles galambos on 11/09/2024.
//

#include "Ravl2/Optimise/Optimise.hh"
#include "Ravl2/Optimise/OptimisePowell.hh"
#include "Ravl2/Optimise/OptimiseBobyqa.hh"

namespace Ravl2
{
  void initOptimise()
  {
    initOptimisePowell();
    initOptimiseBobyqa();
  }

  //! Construct from a configuration
  Optimise::Optimise(Configuration &config)
    : mVerbose(config.getBool("verbose", "Verbose logging", false))
  {}

}