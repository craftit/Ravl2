//
// Created by charles galambos on 11/09/2024.
//

#pragma once

#include <functional>
#include "Ravl2/Types.hh"
#include "Ravl2/Configuration.hh"
#include "CostDomain.hh"

namespace Ravl2
{

  //! Base class for optimisation algorithms that do not require gradients

  class Optimise
  {
  public:
    using RealT = float; //!< The type of the real numbers used in the optimisation

    Optimise() = default;

    //! Setup
    explicit Optimise(bool verbose)
      : mVerbose(verbose)
    {}

    //! Construct from a configuration
    Optimise(Configuration &config);

    virtual ~Optimise() = default;

    //! @brief Determines Xmin=arg min_{X} domain(X)
    //! @param  domain      - the cost function that will be minimised
    //! @param  func - The function to be minimised
    //! @param  start - The starting point for the optimisation, if empty then the midpoint of the domain is used
    //! @return  A tuple with the X value which gives the minimum cost, and the minimum cost value

    [[nodiscard]] virtual std::tuple<VectorT<RealT>,RealT> minimise (
      				const CostDomain<RealT> &domain,
				const std::function<RealT(const VectorT<RealT> &)> &func,
				const VectorT<RealT> &start = VectorT<RealT>()
				) const = 0;

    //! Set the verbose flag
    void setVerbose(bool verbose)
    { mVerbose = verbose; }

    //! Get the verbose flag
    [[nodiscard]] bool verbose() const
    { return mVerbose; }
  protected:
    bool mVerbose = false;
  };



}