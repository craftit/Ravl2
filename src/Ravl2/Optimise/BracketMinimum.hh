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

#include <algorithm>
#include <spdlog/spdlog.h>
#include "Ravl2/Optimise/Optimise.hh"

namespace Ravl2
{

  //! @brief The result of bracketMinimum: lo <= a <= x <= b <= hi with f(x) <= f(a), f(b), or x on
  //! a limit when the cost still falls there.
  template <typename RealT>
  struct Bracket {
    RealT a;  //!< one end of the bracket
    RealT b;  //!< the other end
    RealT x;  //!< the lowest point found, inside [a, b]
    RealT fx; //!< the cost at x
  };

  //! @brief Bracket a minimum of a 1-D function, without leaving [lo, hi].
  //! @details Starts at @p start (cost @p fStart), steps downhill in whichever direction the cost
  //! falls, and expands by the golden ratio until the cost rises again or a limit is reached. Every
  //! evaluated point lies in [lo, hi]: the bracket is for line searches inside a bounded domain.
  //! @param lo Lower limit (lo <= start).
  //! @param hi Upper limit (start <= hi).
  //! @param func The function to bracket.
  //! @param start Where to start; its cost is @p fStart.
  //! @param fStart func(start), already known.
  //! @param steps The first step is (hi - lo) / (2 * steps), at most 0.5.
  template <typename RealT, typename FuncT>
  Bracket<RealT> bracketMinimum(RealT lo, RealT hi, const FuncT &func, RealT start, RealT fStart, int steps = 3)
  {
    const RealT gold = RealT(1.618034);
    if(lo > hi) {
      std::swap(lo, hi);
    }
    start = std::clamp(start, lo, hi);
    if(hi - lo <= RealT(0)) {
      return {start, start, start, fStart};
    }
    steps = std::max(steps, 1);
    const RealT step = std::min(RealT(0.5), (hi - lo) / RealT(2 * steps));
    // Which way is downhill?
    RealT x0 = start, f0 = fStart;
    RealT x1 = std::min(start + step, hi);
    RealT f1 = (x1 != x0) ? func(x1) : f0;
    if(!(f1 < f0)) {
      const RealT xb = std::max(start - step, lo);
      const RealT fb = (xb != x0) ? func(xb) : f0;
      if(!(fb < f0)) {
        return {xb, x1, x0, f0};// the start is lowest: bracketed by its neighbours
      }
      x1 = xb;
      f1 = fb;
    }
    const RealT limit = (x1 > x0) ? hi : lo;
    // Expand downhill until the cost rises or the limit is reached.
    while(x1 != limit) {
      RealT x2 = x1 + gold * (x1 - x0);
      x2 = (limit == hi) ? std::min(x2, hi) : std::max(x2, lo);
      const RealT f2 = func(x2);
      if(f2 >= f1) {
        return {std::min(x0, x2), std::max(x0, x2), x1, f1};
      }
      x0 = x1;
      f0 = f1;
      x1 = x2;
      f1 = f2;
    }
    return {std::min(x0, x1), std::max(x0, x1), x1, f1};// still falling at the limit
  }

  extern template Bracket<double> bracketMinimum<double>(double lo, double hi, const std::function<double(double)> &func, double start, double fStart, int steps);
  extern template Bracket<float> bracketMinimum<float>(float lo, float hi, const std::function<float(float)> &func, float start, float fStart, int steps);

}

