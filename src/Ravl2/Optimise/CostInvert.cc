// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#include "Ravl/PatternRec/CostInvert.hh"
#include "Ravl/StrStream.hh"
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/CostInvert.cc"

namespace RavlN {

  CostInvertBodyC::CostInvertBodyC (const CostC &cost)
    :CostBodyC(cost.GetParameters()),
     _cost(cost)
  {
  }
  
  CostInvertBodyC::CostInvertBodyC (std::istream &in)
    :CostBodyC(in),
     _cost(in)
  {
  }
  
  RealT CostInvertBodyC::Cost (const VectorC &X) const
  {
    return - _cost.Cost (X);
  }
  
  MatrixC CostInvertBodyC::Jacobian (const VectorC &X) const
  {
    return _cost.Jacobian (X) * -1.0;
  }
  
  bool CostInvertBodyC::Save (std::ostream &out) const
  {
    CostBodyC::Save (out);
    _cost.Save (out);
    return true;
  }
  
}
