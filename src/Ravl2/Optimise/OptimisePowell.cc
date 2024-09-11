// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/OptimisePowell.hh"
#include "Ravl2/StrStream.hh"
#include "Ravl2/PatternRec/CostFunction1d.hh"
#include "Ravl2/PatternRec/BracketMinimum.hh"
#include "Ravl2/SArray1dIter5.hh"

namespace Ravl2 {

  OptimisePowellBodyC::OptimisePowellBodyC (unsigned iterations, RealT tolerance, bool useBracketMinimum,bool verbose)
    : OptimiseBodyC("OptimisePowellBodyC"),
     _iterations(iterations),
     _tolerance(tolerance),
     _brentIterations(_iterations),
     _brentTolerance(_tolerance),
     _brent(iterations,tolerance),
     _useBracketMinimum(useBracketMinimum),
     _verbose(verbose)
  {}

  //: Factory constructor
  OptimisePowellBodyC::OptimisePowellBodyC (const XMLFactoryContextC & factory)
   : OptimiseBodyC(factory),
     _iterations(factory.AttributeUInt("iterations",100)),
     _tolerance(factory.AttributeReal("tolerance",1e-4)),
     _brentIterations(factory.AttributeUInt("brentIterations",_iterations)),
     _brentTolerance(factory.AttributeReal("brentTolerance",_tolerance)),
     _brent(_brentIterations,_brentTolerance),
     _useBracketMinimum(factory.AttributeBool("useBracketMinimum",true)),
     _verbose(factory.AttributeBool("verbose",true))
  {}

  OptimisePowellBodyC::OptimisePowellBodyC (std::istream &in)
    :OptimiseBodyC("OptimisePowellBodyC",in)
  {
    in >> _iterations;
  }
  
  static void SetupLimits(const VectorT<RealT> &dir,const VectorT<RealT> &P,const CostC &domain,ParametersC &parameters1d) {
    // Find the domain limits along the direction vector.
    
    RealT min = -RavlConstN::maxReal;
    RealT max = RavlConstN::maxReal;
    IntT steps = 0;
    for(SArray1dIter5C<RealT,RealT,RealT,RealT,IntT> lit(dir,P,domain.MinX(),domain.MaxX(),domain.Steps());lit;lit++) {
      if(lit.data<0>() == 0.0)
        continue; // Avoid division by zero.
      RealT maxv = (lit.data<2>() - lit.data<1>()) / lit.data<0>(); // Limit for MinX
      RealT minv = (lit.Data4() - lit.data<1>()) / lit.data<0>(); // Limit for MaxX
      if(minv > maxv) // The direction vector could have a negative value, so invert if needed.
        std::swap(minv,maxv);
      if(max > maxv) // Pull down maximum if limited
        max = maxv;
      if(minv > min) // Pull up minimum if limited
        min = minv;
      steps += lit.Data5();
    }
    steps /= domain.Steps().size();
    if(steps < 3) steps = 3; // Check there;s actually some space to optimise in.
    
    //Point in full space to evaluate is given by: _point + _direction * X[0];  Where X[0] is the paramiter we're optimising.
    parameters1d.Setup(0,min,max,steps);
  }
  
  // ------------------------------------------------------------------------
  // **********  OptimalX    ************************************************
  // ------------------------------------------------------------------------
  //
  // Powell optimiser. Keeps a set of orthogonal directions and searches along
  // each one in turn for the minimum. The final point is then used to create
  // a new direction which replaces one of the existing ones and the process is
  // repeated.
  //
  VectorT<RealT> OptimisePowellBodyC::MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const
  {
    ParametersC parameters1d(1);
    
    VectorT<RealT> P = domain.StartX();
    IntT numDim = P.size();
    std::vector<VectorT<RealT>> Di(numDim);
    
    if(_verbose) {
      SPDLOG_TRACE("MinimalX bracketMin={} Iterations={} Tolerance={} ",(int) _useBracketMinimum, _iterations,_tolerance);
    }
    
    // initialise directions to basis unit vectors
    for (SArray1dIterC<VectorT<RealT>> it(Di); it; it++) {
      *it = VectorT<RealT>(numDim);
      it->Fill(0.0);
      it.Data()[it.index()] = 1.0;
    }
    
    int indexOfBiggest; // Index of biggest reduction in cost 
    RealT valueOfBiggest;  // Value of cost function after biggest reduction
    VectorT<RealT> Plast;
    VectorT<RealT> Psameagain;
    VectorT<RealT> Pdiff;
    minimumCost = startCost;
    RealT fP = minimumCost;              // Value of cost function at the start of the last iteration
    for (unsigned iter = 0; iter < _iterations; iter++) {
      Plast = P.Copy();       // Save the current position.
      indexOfBiggest = 0;
      valueOfBiggest = 0.0;
      for (SArray1dIterC<VectorT<RealT>> it(Di); it; it++) { // Go through direction vectors.
        
	SetupLimits(*it,P,domain,parameters1d);
        
	// Minimise along line.
	
        RealT fPlast = minimumCost;
        CostFunction1dC cost1d(parameters1d, // Limits for parameters.
                               domain,       // Cost function we're trying to minimise.
                               P,            // Current best position.
                               *it           // Direction we wish to optimise along.
                               );
        if (_useBracketMinimum) {
          BracketMinimum(cost1d);
          P = cost1d.Point(_brent.MinimalX(cost1d,minimumCost));
        } else
          P = cost1d.Point(_brent.MinimalX(cost1d,minimumCost,minimumCost));
        RealT diff = fPlast - minimumCost; // Compute the size of the reduction in cost.
        if (diff > valueOfBiggest) {
          valueOfBiggest = diff;
          indexOfBiggest = it.index();
        }
        
        if(_verbose && (it.index() % 20) == 19)
          std::cerr << "Iter " << iter << " D=" << it.index() << " Cost=" << minimumCost << "\n";
      }
      // Compute the reduction in the cost function.
      RealT fPdiff = fP-minimumCost;
      
      // Check if we're stopped converging.
      if (_tolerance > 0 && 2.0*std::abs(fPdiff) <= _tolerance*(std::abs(fP)+std::abs(minimumCost)))
        break;
      
      
      // check if we should continue in the same direction
      Pdiff = P - Plast;      // How far did we move ?
      Psameagain = P + Pdiff; // Try the same again movement again.
      RealT fPsameagain = domain.Cost(Psameagain); // Evaluate the new move.
      // Include any cost befinit we get from brent along the new direction vector in the benifit
      fP = minimumCost;
      
      // if it has still improved in the same direction
      if (fPsameagain <= fP) {
        RealT t = 
          2.0 * ((fP+fPsameagain)-2.0*minimumCost)*Sqr(fPdiff-valueOfBiggest)
          - valueOfBiggest*Sqr(fP-fPsameagain);
        
        if (t < 0.0) {
          SetupLimits(Pdiff,P,domain,parameters1d); // Setup limits for new direction.
          
          CostFunction1dC cost1d(parameters1d,domain,P,Pdiff);
          if (_useBracketMinimum) {
            BracketMinimum(cost1d);
            P = cost1d.Point(_brent.MinimalX(cost1d,minimumCost));
          } else
            P = cost1d.Point(_brent.MinimalX(cost1d,minimumCost,minimumCost));
          Di[indexOfBiggest] = Di[numDim-1]; // Replace vector yielding largest cost
          Di[numDim-1] = Pdiff.Copy();              // Put in new direction vector.
        }
      }
      if(_verbose)
        SPDLOG_TRACE("Iter {} Cost={}  ",iter,minimumCost);
    }
    return P;
  }
  
  const std::string OptimisePowellBodyC::GetInfo () const
  {
    Strstd::unique_ptr<std::ostream> stream;
    stream << OptimiseBodyC::GetInfo () << "\n";
    stream << "Powell optimization algorithm. Iterations = " << _iterations;
    return stream.String();
  }
  
  bool OptimisePowellBodyC::Save (std::ostream &out) const
  {
    OptimiseBodyC::Save (out);
    out << _iterations << "\n";
    return true;
  }
  
}
