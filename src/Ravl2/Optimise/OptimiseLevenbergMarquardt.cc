// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id: OptimiseLevenbergMarquardt.cc 5398 2006-03-09 16:55:13Z craftit $"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/OptimiseLevenbergMarquardt.cc"

#include "Ravl/PatternRec/OptimiseLevenbergMarquardt.hh"
#include "Ravl/StrStream.hh"
#include "Ravl/SArray1dIter5.hh"
#include "Ravl/SArray1dIter2.hh"
#include "Ravl/SArray1dIter3.hh"
#include "Ravl/PatternRec/CostFunction1d.hh"


#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlN {

  OptimiseLevenbergMarquardtBodyC::OptimiseLevenbergMarquardtBodyC (UIntT iterations, RealT tolerance)
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
  
  VectorC OptimiseLevenbergMarquardtBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    
    VectorC mX = domain.StartX();              // Copy start into temporary var;
    MatrixC mI = MatrixC::Identity(mX.Size());
    RealT currentCost = domain.Cost (mX);      // Evaluate current cost
    RealT lambda = 0.0001;
    
    //mX[6]=0;mX[7]=0; //affine
    ONDEBUG(cerr << "Lambda=" << lambda << " Initial Cost=" << currentCost << "\n");
    UIntT i = 0;
    MatrixC mA;
    VectorC mJ;
    for(;i < _iterations;i++) {
      ONDEBUG(cerr <<endl<<"---- iter ="<<i <<" _iterations= " <<_iterations<<endl);
         // mX[6]=0;mX[7]=0; //affine
      domain.EvaluateValueJacobianHessian(mX,currentCost,mJ,mA);
      //VectorC mb = (mJ * currentCost *-2.0);
      VectorC mb = mJ;
 
      RealT newCost = 0;
      ONDEBUG(cerr << "--mX=" << mX << "\n");
      ONDEBUG(cerr << "--mb=" << mb << "\n");
      ONDEBUG(cerr << "--mA=" << mA << "\n");
      UIntT j = 0;
      for(;j < 16;j++) {
        //cout<<"mA= \n"<< mA<<endl;
        //cout<< "lambda"<<lambda<<endl;
        //cout<< "MatrixC(mA + mI * lambda).Inverse()\n"<<MatrixC(mA + mI * lambda).Inverse()<<endl;
        VectorC mdX = MatrixC(mA + mI * lambda).Inverse() * mb;
        
        VectorC mnX = mX + mdX;
          //  mnX[6]=0;mnX[7]=0; //affine
        newCost = domain.Cost (mnX);      // Evaluate current cost
        ONDEBUG(cerr << "  mdX=" << mdX << "   Cost=" << newCost << "\n");
        if(newCost < currentCost) {
          lambda /= 10;
          mX = mnX; // Store new estimate
          minimumCost = currentCost;
          ONDEBUG(cerr << " Cost decreased! start new iteration "<<"\n"); 
          break;
        }
        lambda *= 10;
        ONDEBUG(cerr << " Lambda increased =" << lambda <<  "\n"); 
      }
      if(j >= 16) {
        // Can't improve on current estimate.
        ONDEBUG(cerr << " Lambda increased 16 times no improvement....start new iteration"<<  "\n"); 
        break;
      }
      ONDEBUG(cerr << " Current Lambda=" << lambda << " Cost=" << newCost << "\n");
      
      // Check if we're stopped converging.
      RealT costdiff = currentCost - newCost;
      if (2.0*Abs(costdiff) <= _tolerance*(Abs(currentCost)+Abs(newCost))) {
        ONDEBUG(cerr << "CostDiff=" << costdiff << " Tolerance=" << _tolerance*(Abs(currentCost)+Abs(minimumCost)) << "\n");
        break;
      }
      
      // Update cost for another go.
       currentCost = newCost;
     }
    ONDEBUG(cerr << "Final Lambda=" << lambda << " Cost=" << minimumCost << " Iterations=" << i << "\n");
    return domain.ConvertX2P (mX);            // Return final estimate
  }
  
  const StringC OptimiseLevenbergMarquardtBodyC::GetInfo () const
  {
    StrOStreamC stream;
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
