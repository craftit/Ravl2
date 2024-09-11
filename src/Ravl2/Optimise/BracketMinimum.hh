// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Robert Crida"
//! date="8/8/2003"
//! example=testCost.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Cost Functions"

#pragma once

#include <spdlog/spdlog.h>
#include "Ravl2/Optimise/Optimise.hh"

namespace Ravl2
{

  //!  Updates the cost function parameter range to bracket a minimum.
  //! @param min - the minimum value of the domain
  //! @param max - the maximum value of the domain
  //! @param func - The function to be minimised
  //! @return A tuple with min, max and the center value
  template<typename RealT,typename FuncT>
  std::tuple<RealT,RealT,RealT> bracketMinimum (RealT min,RealT max, const FuncT &func,int steps = 3)
  {
    const RealT gold = 1.618034;
    const RealT smallVal = 1.0e-20;
    const RealT glimit = 100.0;

    RealT x0 = min;

    if(steps < 2)
      steps = 2;

    RealT x1 = x0 + 0.5;
    if(x1 > (max/ steps)) {
      x1 = (max / steps);
    }

    SPDLOG_TRACE("BracketMinimum. Start: Min={} Max={} ",x0,x1);

    RealT fxn;
    RealT fx0 = func(x0);
    RealT fx1 = func(x1);
    if (fx1 > fx0) {
      std::swap(x0,x1);
      std::swap(fx0,fx1);
    }

    RealT x2 = x1 + gold * (x1 - x0);
    SPDLOG_TRACE("X2={}",x2);

    RealT fx2 = func(x2); // Find cost at center

    while (fx1 > fx2) {
      RealT dx12 = x1 - x2;
      RealT dx10 = x1 - x0;

      RealT r = dx10 * (fx1 - fx2);
      RealT q = dx12 * (fx1 - fx0);
      RealT xn = x1 - (dx12 * q - dx10 * r) / (2.0 * sign(std::max(fabs(q-r),smallVal),q-r));
      RealT xlim = x1 + glimit * (x2 - x1);
      if ((x1 - xn) * (xn - x2) > 0.0) {
	fxn = func(xn);
	if (fxn < fx2) {
	  x0 = x1;
	  x1 = xn;
	  SPDLOG_TRACE("BracketMinimum. Done: Min={} Max={} ",x0,x2);
	  return std::make_tuple(x0,x2,x1);
	}
	else
	if (fxn > fx1) {
	  x2 = xn;
	  SPDLOG_TRACE("BracketMinimum. Done: Min={} Max={} ",x0,x2);
	  return std::make_tuple(x0,x2,x1);
	}
	xn = x2 + gold * (x2 - x1);
	fxn = func(xn);
      }
      else
      if ((x2 - xn) * (xn - xlim) > 0) {
	fxn = func(xn);
	if (fxn < fx2) {
	  x1 = x2; x2 = xn; xn = xn+gold*(xn-x2);
	  fx1 = fx2; fx2 = fxn; fxn = func(xn);
	}
      }
      else
      if ((xn - xlim) * (xlim - x2) >= 0) {
	xn = xlim;
	fxn = func(xn);
      }
      else {
	xn = x2 + gold * (x2 - x1);
	fxn = func(xn);
      }
      x0 = x1; x1 = x2; x2 = xn;
      fx0 = fx1; fx1 = fx2; fx2 = fxn;
    }

    return std::make_tuple(x0,x2,x1);
  }

  extern template std::tuple<double,double,double> bracketMinimum<double>(double min,double max, const std::function<double(double)> &func,int steps);


}

