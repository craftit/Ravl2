// This file is part of RAVL, Recognition And Vision Library
//
// BOBYQA: bound-constrained optimisation by quadratic approximation, without derivatives.
//
// The algorithm and its original Fortran implementation are by M.J.D. Powell:
//   M.J.D. Powell, "The BOBYQA algorithm for bound constrained optimization without
//   derivatives", Report DAMTP 2009/NA06, University of Cambridge, 2009.
//
// This is a port of the C++ translation of Powell's code in dlib
// (dlib/optimization/optimization_bobyqa.h, Copyright (C) 2009 M.J.D. Powell, Davis E. King),
// which is distributed under the Boost Software License, Version 1.0; see the notice at the top of
// OptimiseBobyqa.cc. It has no dependency on dlib.

#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include "Ravl2/Optimise/Optimise.hh"

namespace Ravl2
{

  //! Why a BOBYQA run stopped.
  enum class BobyqaStatus
  {
    Converged,              //!< The trust region radius reached rhoEnd: the normal finish.
    MaxEvaluations,         //!< The evaluation budget ran out before convergence.
    DenominatorCancellation,//!< Rounding errors cancelled a denominator in the model update, even after a rescue.
    TrustRegionStepFailed   //!< A trust region step failed to reduce the quadratic model (usually rounding errors).
  };

  //! Human-readable name of a BOBYQA status.
  [[nodiscard]] std::string_view toString(BobyqaStatus status);

  //! Result of a BOBYQA run. Whatever the status, x is the best point evaluated and cost its value.
  template <typename RealT>
  struct BobyqaResult {
    VectorT<RealT> x;                             //!< Best point found.
    RealT cost = 0;                               //!< Cost at x.
    std::size_t evaluations = 0;                  //!< Number of calls of the cost function.
    BobyqaStatus status = BobyqaStatus::Converged;//!< Why the run stopped.
  };

  //! @brief Minimise a function subject to bounds on its variables, with Powell's BOBYQA.
  //! This is the double-precision core used by OptimiseBobyqa. It is reentrant.
  //! @param func The cost function.
  //! @param start Starting point; must lie within [lower, upper]. It is moved inwards to
  //!   a distance rhoBegin from any bound it is closer to than that (as Powell's code does).
  //! @param lower Lower bounds of the variables.
  //! @param upper Upper bounds of the variables; upper - lower must be at least 2*rhoBegin in every dimension.
  //! @param numInterpolationPoints Number of interpolation points (Powell's NPT), in [n+2, (n+1)(n+2)/2];
  //!   0 selects the recommended 2n+1.
  //! @param rhoBegin Initial trust region radius, typically about a tenth of the largest expected change to a variable.
  //! @param rhoEnd Final trust region radius, the accuracy required in the variables; 0 < rhoEnd <= rhoBegin.
  //! @param maxEvaluations Limit on the number of calls of func. Running out is not an error: the best point
  //!   so far is returned with status MaxEvaluations.
  //! @return The best point found, its cost, the number of evaluations and the reason for stopping.
  //! @throws std::invalid_argument on bad parameters (including fewer than 2 variables), and
  //!   std::runtime_error if func returns a non-finite value.
  [[nodiscard]] BobyqaResult<double> bobyqaMinimise(
    const std::function<double(const VectorT<double> &)> &func,
    const VectorT<double> &start,
    const VectorT<double> &lower,
    const VectorT<double> &upper,
    std::size_t numInterpolationPoints,
    double rhoBegin,
    double rhoEnd,
    std::size_t maxEvaluations);

  //! @brief Powell's BOBYQA: bound-constrained, derivative-free minimisation with quadratic models.
  //! Builds a quadratic model of the cost from numInterpolationPoints interpolation points and minimises
  //! it within a trust region and the domain bounds, shrinking the trust region from rhoBegin to rhoEnd.
  //! Typically needs far fewer cost evaluations than OptimisePowell on smooth problems.
  //!
  //! rhoBegin and rhoEnd are absolute distances in the units of the variables, applied to every
  //! dimension alike, so the variables should be scaled to have similar magnitudes. Every dimension of the
  //! domain must be at least 2*rhoBegin wide. At least two variables are required.
  //!
  //! Internally the computation is in double precision; the cost function is evaluated at the float
  //! rounding of each point and returns a float. When rhoEnd asks for more accuracy than a float
  //! cost can resolve near the minimum, the run usually ends with TrustRegionStepFailed or
  //! DenominatorCancellation instead of Converged; the result is still the best point evaluated.
  //!
  //! minimise() is const and keeps no state between calls, so one instance can be shared between threads.

  class OptimiseBobyqa : public Optimise
  {
  public:
    //! Default parameters: 1000 evaluations, rhoBegin 0.1, rhoEnd 1e-4, 2n+1 interpolation points.
    OptimiseBobyqa() = default;

    //! @param maxEvaluations Limit on the number of cost function evaluations.
    //! @param rhoBegin Initial trust region radius.
    //! @param rhoEnd Final trust region radius (the accuracy required in the variables).
    //! @param numInterpolationPoints Number of interpolation points; 0 selects 2n+1.
    //! @param verbose Log a summary of each run.
    explicit OptimiseBobyqa(std::size_t maxEvaluations, double rhoBegin = 0.1, double rhoEnd = 1e-4,
                            std::size_t numInterpolationPoints = 0, bool verbose = false);

    //! Construct from a configuration
    explicit OptimiseBobyqa(Configuration &config);

    //! Limit on the number of cost function evaluations.
    [[nodiscard]] std::size_t maxEvaluations() const { return mMaxEvaluations; }

    //! Initial trust region radius.
    [[nodiscard]] double rhoBegin() const { return mRhoBegin; }

    //! Final trust region radius.
    [[nodiscard]] double rhoEnd() const { return mRhoEnd; }

    //! Number of interpolation points; 0 means 2n+1.
    [[nodiscard]] std::size_t numInterpolationPoints() const { return mNumInterpolationPoints; }

    //! @brief Determines Xmin=arg min_{X} func(X) within the domain.
    //! @param domain The bounds of the variables.
    //! @param func The function to be minimised.
    //! @param start The starting point; if empty the midpoint of the domain is used.
    //! @return A tuple with the best X found and its cost. Running out of evaluations is not an error.
    [[nodiscard]] std::tuple<VectorT<RealT>, RealT> minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start = VectorT<RealT>()) const final;

    //! @brief As minimise(), also reporting the number of evaluations and why the run stopped.
    [[nodiscard]] BobyqaResult<RealT> minimiseDetailed(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start = VectorT<RealT>()) const;

  private:
    void validate() const;

    std::size_t mMaxEvaluations = 1000;
    double mRhoBegin = 0.1;
    double mRhoEnd = 1e-4;
    std::size_t mNumInterpolationPoints = 0;
  };

  void initOptimiseBobyqa();
}// namespace Ravl2
