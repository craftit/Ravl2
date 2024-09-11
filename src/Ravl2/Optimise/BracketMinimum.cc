// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/BracketMinimum.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  void BracketMinimum(CostFunction1dC &cost) {
    ParametersC parameters(1);
    const RealT gold = 1.618034;
    const RealT smallVal = 1.0e-20;
    const RealT glimit = 100.0;
    VectorT<RealT> vax(1);
    RealT &x0 = vax[0];
    x0 = 0.0;
    
    VectorT<RealT> vbx(1);
    RealT &x1 = vbx[0];
    RealT steps = static_cast<RealT>(cost.Steps ()[0]);;
    if(steps < 2)
      steps = 2;

#if 0
    x1 = cost.MaxX ()[0] / steps;
#else
    x1 = x0 + 0.5;
    if(x1 > (cost.MaxX()[0]/ steps))
      x1 = (cost.MaxX()[0]/ steps);
#endif
    
    ONDEBUG(SPDLOG_TRACE("BracketMinimum. Start: Min={} Max={} ",x0,x1));
    
    VectorT<RealT> vcx(1);
    RealT &x2 = vcx[0];
    
    VectorT<RealT> vux(1);
    RealT &xn = vux[0];
    
    RealT fxn;
    RealT fx0 = cost.Cost(vax);
    RealT fx1 = cost.Cost(vbx);
    if (fx1 > fx0) {
      std::swap(x0,x1);
      std::swap(fx0,fx1);
    }
    
    x2 = x1 + gold * (x1 - x0);
    ONDEBUG(SPDLOG_TRACE("X2={}",x2));
    
    RealT fx2 = cost.Cost(vcx); // Find cost at center
    
    while (fx1 > fx2) {
      RealT dx12 = x1 - x2;
      RealT dx10 = x1 - x0;
      
      RealT r = dx10 * (fx1 - fx2);
      RealT q = dx12 * (fx1 - fx0);
      xn = x1 - (dx12 * q - dx10 * r) / (2.0 * Sign(Max(fabs(q-r),smallVal),q-r));
      RealT xlim = x1 + glimit * (x2 - x1);
      if ((x1 - xn) * (xn - x2) > 0.0) {
        fxn = cost.Cost(vux);
        if (fxn < fx2) {
          x0 = x1;
          x1 = xn;
          ONDEBUG(SPDLOG_TRACE("BracketMinimum. Done: Min={} Max={} ",x0,x2));
          parameters.Setup(0, x0, x2, 1, x1);
          cost.SetParameters(parameters);
          return;
        }
        else
          if (fxn > fx1) {
            x2 = xn;
            ONDEBUG(SPDLOG_TRACE("BracketMinimum. Done: Min={} Max={} ",x0,x2));
            parameters.Setup(0, x0, x2, 1, x1); // Start, Min, Max, Steps
            cost.SetParameters(parameters);
            return;
          }
        xn = x2 + gold * (x2 - x1);
        fxn = cost.Cost(vux);
      }
      else
        if ((x2 - xn) * (xn - xlim) > 0.0) {
          fxn = cost.Cost(vux);
          if (fxn < fx2) {
            x1 = x2; x2 = xn; xn = xn+gold*(xn-x2);
            fx1 = fx2; fx2 = fxn; fxn = cost.Cost(vux);
          }
        }
        else
          if ((xn - xlim) * (xlim - x2) >= 0.0) {
            xn = xlim;
            fxn = cost.Cost(vux);
          }
          else {
            xn = x2 + gold * (x2 - x1);
            fxn = cost.Cost(vux);
          }
      x0 = x1; x1 = x2; x2 = xn;
      fx0 = fx1; fx1 = fx2; fx2 = fxn;
    }
#if 1
    ONDEBUG(SPDLOG_TRACE("BracketMinimum. Done: Min={} Max={} ",x0,x2));
    parameters.Setup(0, x0, x2, 1, x1); // Start, Min, Max, Steps
    cost.SetParameters(parameters);
#endif
  }
  
}
