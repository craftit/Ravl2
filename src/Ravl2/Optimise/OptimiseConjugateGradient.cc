// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/OptimiseConjugateGradient.cc"

#include "Ravl/PatternRec/OptimiseConjugateGradient.hh"
#include "Ravl/StrStream.hh"
#include "Ravl/SArray1dIter5.hh"
#include "Ravl/SArray1dIter2.hh"
#include "Ravl/SArray1dIter3.hh"
#include "Ravl/PatternRec/OptimisePowell.hh"
#include "Ravl/PatternRec/CostFunction1d.hh"
#include "Ravl/PatternRec/BracketMinimum.hh"
#include "Ravl/XMLFactoryRegister.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlN {

  //: Constructor from xml factory.

  OptimiseConjugateGradientBodyC::OptimiseConjugateGradientBodyC(const XMLFactoryContextC & factory)
   : OptimiseBodyC(factory),
     _iterations(factory.AttributeUInt("iterations",1000)),
     _tolerance(factory.AttributeReal("tolerance",1e-6)),
     _brentIterations(factory.AttributeUInt("brentIterations",_iterations)),
     _brentTolerance(factory.AttributeReal("brentTolerance",_tolerance)),
     _useBracketMinimum(factory.AttributeBool("useBracketMinimum",true)),
      m_useAbsoluteCostForTolerance(factory.AttributeBool("useAbsoluteCostForTolerance",false))
  {

  }

  OptimiseConjugateGradientBodyC::OptimiseConjugateGradientBodyC (UIntT iterations, RealT tolerance,
                                                                  bool useBacketMinimum,
                                                                  bool useAbsoluteCostForTolerance,
                                                                  UIntT brentIterations, RealT brentTolerance)
    : OptimiseBodyC("OptimiseConjugateGradientBodyC"),
      _iterations(iterations),
      _tolerance(tolerance),
      _brentIterations(brentIterations),
      _brentTolerance(brentTolerance),
      _useBracketMinimum(useBacketMinimum),
      m_useAbsoluteCostForTolerance(useAbsoluteCostForTolerance)
  {
    if(_brentIterations == 0)
      _brentIterations = _iterations;
    if(_brentTolerance == 0)
      _brentTolerance = _tolerance;
  }
  
  OptimiseConjugateGradientBodyC::OptimiseConjugateGradientBodyC (std::istream &in)
    : OptimiseBodyC("OptimiseConjugateGradientBodyC",in),
      _iterations(0),
      _tolerance(0),
      _brentIterations(0),
      _brentTolerance(0),
      _useBracketMinimum(false),
      m_useAbsoluteCostForTolerance(0)
  {
    in >> _iterations;
  }

  RavlN::RCBodyVC &OptimiseConjugateGradientBodyC::Copy() const
  { return *new OptimiseConjugateGradientBodyC(*this); }
  //: Create copy of the optimiser

  
  static void SetupLimits(const VectorC &dir,const VectorC &P,const CostC &domain,ParametersC &parameters1d) {
    // Find the domain limits along the direction vector.
    
    //RealT min = -RavlConstN::maxReal;
    RealT min = 0; // We only ever want to go down hill.
    RealT max = RavlConstN::maxReal;
    IntT steps = 0;
    for(SArray1dIter5C<RealT,RealT,RealT,RealT,IntT> lit(dir,P,domain.MinX(),domain.MaxX(),domain.Steps());lit;lit++) {
      if(lit.Data1() == 0.0)
        continue; // Avoid division by zero.
      RealT maxv = (lit.Data3() - lit.Data2()) / lit.Data1(); // Limit for MinX
      RealT minv = (lit.Data4() - lit.Data2()) / lit.Data1(); // Limit for MaxX
      if(minv > maxv) // The direction vector could have a negative value, so invert if needed.
        Swap(minv,maxv);
      if(max > maxv) // Pull down maximum if limited
        max = maxv;
      if(minv > min) // Pull up minimum if limited
        min = minv;
      steps += lit.Data5();
    }
    steps /= domain.Steps().Size();
    if(steps < 3) steps = 3; // Check there;s actually some space to optimise in.
    
    //Point in full space to evaluate is given by: _point + _direction * X[0];  Where X[0] is the parameter we're optimising.
    parameters1d.Setup(0,min,max,steps);
  }
  
  
  // ------------------------------------------------------------------------
  // **********  OptimalX    ************************************************
  // ------------------------------------------------------------------------
  
  VectorC OptimiseConjugateGradientBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {

    RavlAssertMsg(domain.GetParameters().IsValid(),"Cost function has no parameters setup.");

    UIntT counter = 0;
    VectorC iterX = domain.StartX();         // Copy start into temporary var;
    
#if 0
    std::cerr << "ClipX=" << domain.ClipX (iterX) << "\n";
    std::cerr << "    X=" << iterX << "\n";
#endif
    
    ParametersC parameters1d(1);
    OptimiseBrentC _brent(_brentIterations,_brentTolerance);
    RealT currentCost = domain.Cost (iterX);      // Evaluate current cost
    //RealT firstCost = currentCost;

    VectorC dYdX = domain.Jacobian1(iterX) * -1.0; // Determine current Jacobian
    VectorC gdYdX = dYdX.Copy();
    VectorC hdYdX = dYdX.Copy();
    
    do {
      SetupLimits(dYdX,iterX,domain,parameters1d);
      
      // Setup minimisation along line.
      
      CostFunction1dC cost1d(parameters1d, // Limits for parameters.
                             domain,       // Cost function we're trying to minimise.
                             iterX,        // Current best position.
                             dYdX          // Direction we wish to optimise along.
                             );
      
      if (_useBracketMinimum) {
        BracketMinimum(cost1d);
        iterX = cost1d.Point(_brent.MinimalX(cost1d,minimumCost));
      } else
        iterX = cost1d.Point(_brent.MinimalX(cost1d,currentCost,minimumCost));
      
      // Check termination condition.
      
      // Check if we're stopped converging.
      if(m_useAbsoluteCostForTolerance) {
        if(minimumCost < _tolerance ) {
          RavlDebug("Tolerance requirement met. %f ",minimumCost);
          break;
        }
        if(minimumCost == currentCost) {
          RavlDebug("Done on equal costs.");
          break;
        }
      } else {
        // Compute the reduction in the cost function.
        RealT costdiff = currentCost-minimumCost;
        // If tolerance is zero, just execute the requested number of iterations.
        if (_tolerance > 0 && 2.0*Abs(costdiff) <= _tolerance*(Abs(currentCost)+Abs(minimumCost))) {
          //ONDEBUG(cerr << "CostDiff=" << costdiff << " Tolerance=" << _tolerance*(Abs(currentCost)+Abs(minimumCost)) << "\n");
          //RavlDebug("Improvement below minimum. ");
          break;
        }
      }
      currentCost = minimumCost; // Reset for next iteration.
      
      // Determine current Jacobian
      dYdX = domain.Jacobian1(iterX);
      
      // Compute conjugate direction.
      RealT gg = 0;
      RealT dgg =0;
      for(SArray1dIter2C<RealT,RealT> it(dYdX,gdYdX);it;it++) {
        gg += Sqr(it.Data2());
        dgg += (it.Data1() + it.Data2()) * it.Data1();
      }
      //RavlDebug("gg=%f dgg=%f ",gg,dgg);
      if(gg == 0) {
        ONDEBUG(std::cerr << "Terminated on gg == 0\n");
        break;
      }
      RealT gama = dgg/gg;
#if 1
      if(Abs(gama) < 1e-9) {
#if 1
        gdYdX = dYdX.Copy();
        hdYdX = dYdX.Copy();

        RealT gg = 0;
        RealT dgg =0;
        for(SArray1dIter2C<RealT,RealT> it(dYdX,gdYdX);it;it++) {
          gg += Sqr(it.Data2());
          dgg += (it.Data1() + it.Data2()) * it.Data1();
        }
        //RavlDebug("gg=%f dgg=%f ",gg,dgg);
        if(gg == 0) {
          ONDEBUG(std::cerr << "Terminated on gg == 0\n");
          break;
        }
        gama = dgg/gg;
        RavlDebug("Direction reset gama:%f cost:%f ",gama,currentCost);
#else
        std::cerr << "Directions exhausted \n";
        break;
#endif
      }
#endif
      for(SArray1dIter3C<RealT,RealT,RealT> it(dYdX,gdYdX,hdYdX);it;it++) {
        it.Data2() = -it.Data1();
        it.Data1() = it.Data2() + gama * it.Data3();
        it.Data3() = it.Data1();
      }
      
    } while (counter++ < _iterations); 
    //RavlDebug("Terminated after %u  iterations. MinCost=%f",counter,currentCost);
    //if(currentCost < 0.1)
    //  RavlDebug("First cost: %f final: %f ",firstCost,currentCost);

    return domain.ConvertX2P (iterX);            // Return final estimate
  }
  
  const StringC OptimiseConjugateGradientBodyC::GetInfo () const
  {
    StrOStreamC stream;
    stream << OptimiseBodyC::GetInfo () << "\n";
    stream << "Gradient descent optimisation algorithm. Iterations = " << _iterations;
    return stream.String();
  }
  
  bool OptimiseConjugateGradientBodyC::Save (std::ostream &out) const
  {
    OptimiseBodyC::Save (out);
    out << _iterations << "\n";
    return true;
  }
  
  void LinkOptimiseConjugateGradient()
  {}

  static RavlN::XMLFactoryRegisterHandleConvertC<OptimiseConjugateGradientC, OptimiseC> g_registerXMLFactoryDesignClassifierGaussianMixture("RavlN::OptimiseConjugateGradientC");


}
