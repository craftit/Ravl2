// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/Cost.cc"

#include "Ravl/PatternRec/Cost.hh"

//////////////////////////////////////////
//: Constructor
//!param: name       - derived class type name

namespace RavlN {
  
  CostBodyC::CostBodyC (const ParametersC &parameters)
    : _parameters(parameters)
  {
    InputSize (_parameters.MinX().Size());
    OutputSize (1);
  }
  
  CostBodyC::CostBodyC (std::istream &in)
    : Function1BodyC(in),
      _parameters(in)
  {}
  
  //: Apply function to 'data'
  
  VectorC CostBodyC::Apply(const VectorC &X) const {
    VectorC Y (1);
    Y[0] = Cost (X);
    return Y;    
  }

  void CostBodyC::SetMask (const SArray1dC<IntT> &mask)
  {
    _parameters.SetMask (mask);
    InputSize (_parameters.MinX().Size());
  }
  
  VectorC CostBodyC::ClipX (const VectorC &X) const
  {
    const VectorC &minX = MinX();
    const VectorC &maxX = MaxX();
    VectorC outX = X.Copy();
    for (IndexC i = 0; i < X.Size(); i++) {
      if (X[i] < minX[i]) outX[i] = minX[i];
      if (X[i] > maxX[i]) outX[i] = maxX[i];
    }
    return outX;
  }
  
  void CostBodyC::SetParameters (const ParametersC &parameters)
  {
    InputSize (parameters.MinX().Size());
    _parameters = parameters;
  }
  
  bool CostBodyC::Save (std::ostream &out) const
  {
    FunctionBodyC::Save (out);
    _parameters.Save (out);
    return true;
  }
  
  
}
