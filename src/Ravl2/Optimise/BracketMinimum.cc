// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Optimise/BracketMinimum.hh"

namespace Ravl2
{

  template Bracket<double> bracketMinimum<double>(double lo, double hi, const std::function<double(double)> &func, double start, double fStart, int steps);
  template Bracket<float> bracketMinimum<float>(float lo, float hi, const std::function<float(float)> &func, float start, float fStart, int steps);
}
