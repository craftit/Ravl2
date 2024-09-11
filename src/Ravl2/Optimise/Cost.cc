// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/Cost.hh"

//////////////////////////////////////////
//: Constructor
//! @param  name       - derived class type name

namespace Ravl2 {
  
  CostBodyC::CostBodyC (const ParametersC &parameters)
    : _parameters(parameters)
  {
    InputSize (_parameters.MinX().size());
    OutputSize (1);
  }
  
  CostBodyC::CostBodyC (std::istream &in)
    : Function1BodyC(in),
      _parameters(in)
  {}
  
  //: Apply function to 'data'
  
  VectorT<RealT> CostBodyC::Apply(const VectorT<RealT> &X) const {
    VectorT<RealT> Y (1);
    Y[0] = Cost (X);
    return Y;    
  }

  void CostBodyC::SetMask (const std::vector<IntT> &mask)
  {
    _parameters.SetMask (mask);
    InputSize (_parameters.MinX().size());
  }
  
  VectorT<RealT> CostBodyC::ClipX (const VectorT<RealT> &X) const
  {
    const VectorT<RealT> &minX = MinX();
    const VectorT<RealT> &maxX = MaxX();
    VectorT<RealT> outX = X.Copy();
    for (int i = 0; i < X.size(); i++) {
      if (X[i] < minX[i]) outX[i] = minX[i];
      if (X[i] > maxX[i]) outX[i] = maxX[i];
    }
    return outX;
  }
  
  void CostBodyC::SetParameters (const ParametersC &parameters)
  {
    InputSize (parameters.MinX().size());
    _parameters = parameters;
  }
  
  bool CostBodyC::Save (std::ostream &out) const
  {
    FunctionBodyC::Save (out);
    _parameters.Save (out);
    return true;
  }
  
  
}
