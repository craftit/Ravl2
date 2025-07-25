// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Optimise/OptimisePowell.hh"
#include "Ravl2/Optimise/BracketMinimum.hh"

namespace Ravl2 {

  void initOptimisePowell()
  {}


  OptimisePowell::OptimisePowell (unsigned iterations, RealT tolerance, bool useBracketMinimum,bool verbose)
    : Optimise(verbose),
     _iterations(iterations),
     _tolerance(tolerance),
     _brentIterations(_iterations),
     _brentTolerance(_tolerance),
     _brent(iterations,tolerance),
     _useBracketMinimum(useBracketMinimum)
  {}

  //! Construct from a configuration
  OptimisePowell::OptimisePowell(Configuration &config)
   : Optimise(config),
     _iterations(config.getNumber<unsigned>("iterations","Limit on the number of optimisation iterations",100,1,10000)),
     _tolerance(config.getNumber<RealT>("tolerance","Tolerance for stopping optimisation",RealT(1e-4),1e-10,10.0)),
     _brentIterations(config.getNumber<unsigned>("brentIterations","Limit on the number of Brent iterations",_iterations,1u,10000u)),
     _brentTolerance(config.getNumber<RealT,float>("brentTolerance","Tolerance for stopping Brent optimisation",_tolerance,1e-10f,10.0f)),
     _brent(_brentIterations,_brentTolerance),
     _useBracketMinimum(config.getBool("useBracketMinimum","Use bracket minimum in Brent",true))
  {}

  //! Setup limits, returns min,max
  std::tuple<OptimisePowell::RealT,OptimisePowell::RealT> OptimisePowell::SetupLimits(
    const VectorT<RealT> &dir,
    const VectorT<RealT> &P,
    const CostDomain<RealT> &domain)
  {
    // Find the domain limits along the direction vector.
    assert(dir.size() == P.size());
    if(P.size() != domain.min().size() || P.size() != domain.max().size()) {
      SPDLOG_ERROR("Parameter vector and domain must be the same size ({} and {})",P.size(),domain.min().size());
      throw std::runtime_error("Parameter vector and domain must be the same size");
    }
    RealT min = -std::numeric_limits<RealT>::min();
    RealT max = std::numeric_limits<RealT>::max();
    for(IndexT i = 0;i < P.size();i++) {
      if(dir[i] == 0)
	continue; // Avoid division by zero.
      RealT maxv = (domain.min()[i] - P[i]) / dir[i]; // Limit for MinX
      RealT minv = (domain.max()[i] - P[i]) / dir[i]; // Limit for MaxX
      if(minv > maxv) // The direction vector could have a negative value, so invert if needed.
	std::swap(minv,maxv);
      if(max > maxv) // Pull down maximum if limited
	max = maxv;
      if(minv > min) // Pull up minimum if limited
	min = minv;
    }

    //Point in full space to evaluate is given by: _point + _direction * X[0];  Where X[0] is the paramiter we're optimising.
    return std::make_tuple(min,max);
  }

  // Powell optimiser. Keeps a set of orthogonal directions and searches along
  // each one in turn for the minimum. The final point is then used to create
  // a new direction which replaces one of the existing ones and the process is
  // repeated.
  //
  //VectorT<RealT> OptimisePowell::MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const
  std::tuple<VectorT<OptimisePowell::RealT>,OptimisePowell::RealT> OptimisePowell::minimise (
    const CostDomain<RealT> &domain,
    const std::function<RealT(const VectorT<RealT> &)> &func,
    const VectorT<RealT> &start
  ) const

  {
    VectorT<RealT> P = start;
    RealT minimumCost = func(P);
    auto numDim = P.size();
    std::vector<VectorT<RealT>> Di((std::size_t(numDim)));
    if(mVerbose) {
      SPDLOG_INFO("MinimalX bracketMin={} Iterations={} Tolerance={} ", _useBracketMinimum, _iterations,_tolerance);
    }
    // Initialise directions to basis unit vectors
    for(IndexT i = 0;i < IndexT(Di.size());i++) {
      Di[std::size_t(i)] = VectorT<RealT>::Zero(numDim);
      Di[std::size_t(i)][i] = 1.0;
    }
    VectorT<RealT> Plast;
    VectorT<RealT> Psameagain;
    VectorT<RealT> Pdiff;
    RealT fP = minimumCost;              // Value of cost function at the start of the last iteration

    for (unsigned iter = 0; iter < _iterations; iter++) {
      Plast = P;       // Save the current position.
      size_t indexOfBiggest = 0; // Index of biggest reduction in cost
      RealT valueOfBiggest = 0.0;// Value of cost function after biggest reduction
      for (size_t id = 0;id < Di.size();++id) { // Go through direction vectors.
	auto &it = Di[id];
	auto [minP,maxP ] = SetupLimits(it,P,domain);

	// Minimise along line.
	auto linearFunc = [&func,&P,&it](RealT x) {
	  return func(P + it * x);
	};
	RealT startAt = 0.0;
	RealT fPlast = minimumCost;
	if(_useBracketMinimum) {
	  auto [minB,maxB,midB] = bracketMinimum (RealT(0),maxP,linearFunc);
	  minP = minB;
	  maxP = maxB;
	  startAt = midB;
	}
	auto [bestVal,bestCost] = _brent.minimise(minP,maxP,startAt,minimumCost,linearFunc);
	P += it * bestVal;
	RealT diff = fPlast - bestCost; // Compute the size of the reduction in cost.
	minimumCost = bestCost;
	if (diff > valueOfBiggest) {
	  valueOfBiggest = diff;
	  indexOfBiggest = id;
	}
	if(mVerbose) {
	  SPDLOG_INFO("Iter {} D={} Cost={} ", iter, id, minimumCost);
	}
      }

      // Compute the reduction in the cost function.
      RealT fPdiff = fP-minimumCost;

      // Check if we're stopped converging.
      if (_tolerance > 0 && (2*std::abs(fPdiff) <= _tolerance*(std::abs(fP)+std::abs(minimumCost)))) {
	break;
      }
      // check if we should continue in the same direction
      Pdiff = P - Plast;      // How far did we move ?
      Psameagain = P + Pdiff; // Try the same again movement again.
      RealT fPsameagain = func(Psameagain); // Evaluate the new move.
      // Include any cost benefit we get from brent along the new direction vector in the benefit
      fP = minimumCost;
      // if it has still improved in the same direction
      if (fPsameagain <= fP) {
        RealT t =
          2 * ((fP+fPsameagain)-2*minimumCost)*sqr(fPdiff-valueOfBiggest)
          - valueOfBiggest*sqr(fP-fPsameagain);

        if (t < 0) {
	  auto [minP,maxP ] = SetupLimits(Pdiff,P,domain);
	  auto linearFunc = [&func,&P,&Pdiff](RealT x) {
	    return func(P + Pdiff * x);
	  };
	  RealT startAt = 0;
	  if(_useBracketMinimum) {
	    auto [minB,maxB,midB] = bracketMinimum (minP,maxP,linearFunc);
	    minP = minB;
	    maxP = maxB;
	    startAt = midB;
	  }
	  auto [bestVal,bestCost] = _brent.minimise(minP,maxP,startAt,minimumCost,linearFunc);
	  P += Pdiff * bestVal;
	  //minimumCost = bestCost;
	  Di[indexOfBiggest] = Di[size_t(numDim-1)]; // Replace vector yielding largest cost
	  Di[size_t(numDim-1)] = Pdiff;              // Put in new direction vector.
        }
      }
      if(mVerbose) {
				SPDLOG_INFO("Iter {} Cost={}  ", iter, minimumCost);
      }
    }
    return std::make_tuple(P,minimumCost);
  }

  namespace {
    [[maybe_unused]] bool g_register = defaultConfigFactory().registerNamedType<Optimise, OptimisePowell>("OptimisePowell");

  }

}
