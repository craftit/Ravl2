// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Optimise/BracketMinimum.hh"

namespace Ravl2
{

  template std::tuple<double,double,double> bracketMinimum<double>(double min,double max, const std::function<double(double)> &func,int steps);
  template std::tuple<float,float,float> bracketMinimum<float>(float min,float max, const std::function<float(float)> &func,int steps);
}
