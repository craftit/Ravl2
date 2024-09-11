// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/OptimiseLevenbergMarquardt.hh"
#include "Ravl2/StrStream.hh"
#include "Ravl2/SArray1dIter5.hh"
#include "Ravl2/SArray1dIter2.hh"
#include "Ravl2/SArray1dIter3.hh"
#include "Ravl2/PatternRec/CostFunction1d.hh"


#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  OptimiseLevenbergMarquardtBodyC::OptimiseLevenbergMarquardtBodyC (unsigned iterations, RealT tolerance)
    : OptimiseBodyC("OptimiseLevenbergMarquardtBodyC"),
      _iterations(iterations),
      _tolerance(tolerance)
  {
  }
  
  OptimiseLevenbergMarquardtBodyC::OptimiseLevenbergMarquardtBodyC (std::istream &in)
    : OptimiseBodyC("OptimiseLevenbergMarquardtBodyC",in)
  {
    in >> _iterations;
  }
  
  VectorT<RealT> OptimiseLevenbergMarquardtBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    
    VectorT<RealT> mX = domain.StartX();              // Copy start into temporary var;
    Tensor<RealT,2> mI = Tensor<RealT,2>::Identity(mX.size());
    RealT currentCost = domain.Cost (mX);      // Evaluate current cost
    RealT lambda = 0.0001;
    
    //mX[6]=0;mX[7]=0; //affine
    ONDEBUG(std::cerr << "Lambda=" << lambda << " Initial Cost=" << currentCost << "\n");
    unsigned i = 0;
    Tensor<RealT,2> mA;
    VectorT<RealT> mJ;
    for(;i < _iterations;i++) {
      ONDEBUG(std::cerr <<endl<<"---- iter ="<<i <<" _iterations= " <<_iterations<<endl);
         // mX[6]=0;mX[7]=0; //affine
      domain.EvaluateValueJacobianHessian(mX,currentCost,mJ,mA);
      //VectorT<RealT> mb = (mJ * currentCost *-2.0);
      VectorT<RealT> mb = mJ;
 
      RealT newCost = 0;
      ONDEBUG(std::cerr << "--mX=" << mX << "\n");
      ONDEBUG(std::cerr << "--mb=" << mb << "\n");
      ONDEBUG(std::cerr << "--mA=" << mA << "\n");
      unsigned j = 0;
      for(;j < 16;j++) {
        //cout<<"mA= \n"<< mA<<endl;
        //cout<< "lambda"<<lambda<<endl;
        //cout<< "Tensor<RealT,2>(mA + mI * lambda).Inverse()\n"<<Tensor<RealT,2>(mA + mI * lambda).Inverse()<<endl;
        VectorT<RealT> mdX = Tensor<RealT,2>(mA + mI * lambda).Inverse() * mb;
        
        VectorT<RealT> mnX = mX + mdX;
          //  mnX[6]=0;mnX[7]=0; //affine
        newCost = domain.Cost (mnX);      // Evaluate current cost
        ONDEBUG(std::cerr << "  mdX=" << mdX << "   Cost=" << newCost << "\n");
        if(newCost < currentCost) {
          lambda /= 10;
          mX = mnX; // Store new estimate
          minimumCost = currentCost;
          ONDEBUG(std::cerr << " Cost decreased! start new iteration "<<"\n"); 
          break;
        }
        lambda *= 10;
        ONDEBUG(std::cerr << " Lambda increased =" << lambda <<  "\n"); 
      }
      if(j >= 16) {
        // Can't improve on current estimate.
        ONDEBUG(std::cerr << " Lambda increased 16 times no improvement....start new iteration"<<  "\n"); 
        break;
      }
      ONDEBUG(std::cerr << " Current Lambda=" << lambda << " Cost=" << newCost << "\n");
      
      // Check if we're stopped converging.
      RealT costdiff = currentCost - newCost;
      if (2.0*std::abs(costdiff) <= _tolerance*(std::abs(currentCost)+std::abs(newCost))) {
        ONDEBUG(std::cerr << "CostDiff=" << costdiff << " Tolerance=" << _tolerance*(std::abs(currentCost)+std::abs(minimumCost)) << "\n");
        break;
      }
      
      // Update cost for another go.
       currentCost = newCost;
     }
    ONDEBUG(std::cerr << "Final Lambda=" << lambda << " Cost=" << minimumCost << " Iterations=" << i << "\n");
    return domain.ConvertX2P (mX);            // Return final estimate
  }
  
  const std::string OptimiseLevenbergMarquardtBodyC::GetInfo () const
  {
    Strstd::unique_ptr<std::ostream> stream;
    stream << OptimiseBodyC::GetInfo () << "\n";
    stream << "Levenberg Marguardt gradient descent optimization algorithm. Iterations = " << _iterations;
    return stream.String();
  }
  
  bool OptimiseLevenbergMarquardtBodyC::Save (std::ostream &out) const
  {
    OptimiseBodyC::Save (out);
    out << _iterations << "\n";
    return true;
  }


  
}
