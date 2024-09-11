// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/CostFunction1d.cc"

#include "Ravl/PatternRec/CostFunction1d.hh"
#include "Ravl/SArray1dIter3.hh"

namespace RavlN {

  CostFunction1dBodyC::CostFunction1dBodyC (const ParametersC parameters,
                                            const CostC &cost,
                                            const VectorC &point,
                                            const VectorC &direction)
    : CostBodyC(parameters),
      _cost(cost),
      _point(point),
      _direction(direction)
  {
    RavlAssert (_point.Size() == _direction.Size());
    RavlAssert (_cost.OutputSize() == 1);
  }
  
  CostFunction1dBodyC::CostFunction1dBodyC (std::istream &in)
    :CostBodyC(in),
     _cost(in)
  {
    in >> _point >> _direction;
  }
  
  RealT CostFunction1dBodyC::Cost (const VectorC &X) const
  {
    return _cost.Cost (Point(X));
  }
  
  VectorC CostFunction1dBodyC::Point (const VectorC &X) const
  {
    VectorC ret(_point.Size());
    for(SArray1dIter3C<RealT,RealT,RealT> it(ret,_point,_direction);it;it++)
      it.Data1() = it.Data2() + it.Data3() * X[0];
    return ret;
    //return _point + _direction * X[0];
  }
    
  bool CostFunction1dBodyC::Save (std::ostream &out) const
  {
    CostBodyC::Save (out);
    _cost.Save (out);
    out << _point << "\n" << _direction << "\n";
    return true;
  }

  //: Apply function to 'data'
  
  RealT CostFunction1dBodyC::Apply1(const VectorC &X) const {
    return Cost (X);
  }

  
}
