//
// Created by charles galambos on 11/09/2024.
//

#include "Ravl2/Optimise/Optimise.hh"

namespace Ravl2
{
  //! Construct from a configuration
  Optimise::Optimise(Configuration &config)
    : mVerbose(config.getBool("verbose", "Verbose logging", false))
  {}

}