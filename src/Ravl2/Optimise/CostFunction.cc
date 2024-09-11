// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/CostFunction.hh"

namespace Ravl2 {

  CostFunctionBodyC::CostFunctionBodyC (const ParametersC &parameters,
					const VectorT<RealT> &Yd,
					const FunctionC &function,
					const DistanceC &metric)
    :CostBodyC(parameters),
     _Yd(Yd),
     _function(function),
     _metric(metric)
  {
    RavlAssert (_Yd.size() == _function.OutputSize());
  }
  
  CostFunctionBodyC::CostFunctionBodyC (std::istream &in)
    :CostBodyC(in)
  {
    in >> _Yd;
    _function = FunctionC (in);
    _metric = DistanceC (in);
  }
  
  RealT CostFunctionBodyC::Cost (const VectorT<RealT> &X) const
  {
    VectorT<RealT> P = _parameters.TransX2P(X);
    VectorT<RealT> diff = _function.Apply (P) - _Yd;
    return _metric.Magnitude (diff);
  }
  
  Tensor<RealT,2> CostFunctionBodyC::Jacobian (const VectorT<RealT> &X) const
  {
    VectorT<RealT> P = _parameters.TransX2P(X);
    VectorT<RealT> diff = _function.Apply (P) - _Yd;
    Tensor<RealT,2> dSdY = _metric.Jacobian (diff);
    Tensor<RealT,2> dYdP = _function.Jacobian (P);
    // This was:
    //Tensor<RealT,2> dPdX = TransX2P();
    //return dSdY*dYdP*dPdX;
    return _parameters.TransP2X(dSdY*dYdP);
  }
  
  bool CostFunctionBodyC::Save (std::ostream &out) const
  {
    CostBodyC::Save (out);
    out << _Yd << "\n";
    _function.Save (out);
    _metric.Save (out);
    return true;
  }
  
}
