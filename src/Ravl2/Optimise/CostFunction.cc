// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/CostFunction.cc"

#include "Ravl/PatternRec/CostFunction.hh"

namespace RavlN {

  CostFunctionBodyC::CostFunctionBodyC (const ParametersC &parameters,
					const VectorC &Yd,
					const FunctionC &function,
					const DistanceC &metric)
    :CostBodyC(parameters),
     _Yd(Yd),
     _function(function),
     _metric(metric)
  {
    RavlAssert (_Yd.Size() == _function.OutputSize());
  }
  
  CostFunctionBodyC::CostFunctionBodyC (std::istream &in)
    :CostBodyC(in)
  {
    in >> _Yd;
    _function = FunctionC (in);
    _metric = DistanceC (in);
  }
  
  RealT CostFunctionBodyC::Cost (const VectorC &X) const
  {
    VectorC P = _parameters.TransX2P(X);
    VectorC diff = _function.Apply (P) - _Yd;
    return _metric.Magnitude (diff);
  }
  
  MatrixC CostFunctionBodyC::Jacobian (const VectorC &X) const
  {
    VectorC P = _parameters.TransX2P(X);
    VectorC diff = _function.Apply (P) - _Yd;
    MatrixC dSdY = _metric.Jacobian (diff);
    MatrixC dYdP = _function.Jacobian (P);
    // This was:
    //MatrixC dPdX = TransX2P();
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
