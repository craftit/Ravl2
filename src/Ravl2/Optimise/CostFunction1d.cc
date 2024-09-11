// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/CostFunction1d.hh"
#include "Ravl2/SArray1dIter3.hh"

namespace Ravl2 {

  CostFunction1dBodyC::CostFunction1dBodyC (const ParametersC parameters,
                                            const CostC &cost,
                                            const VectorT<RealT> &point,
                                            const VectorT<RealT> &direction)
    : CostBodyC(parameters),
      _cost(cost),
      _point(point),
      _direction(direction)
  {
    RavlAssert (_point.size() == _direction.size());
    RavlAssert (_cost.OutputSize() == 1);
  }
  
  CostFunction1dBodyC::CostFunction1dBodyC (std::istream &in)
    :CostBodyC(in),
     _cost(in)
  {
    in >> _point >> _direction;
  }
  
  RealT CostFunction1dBodyC::Cost (const VectorT<RealT> &X) const
  {
    return _cost.Cost (Point(X));
  }
  
  VectorT<RealT> CostFunction1dBodyC::Point (const VectorT<RealT> &X) const
  {
    VectorT<RealT> ret(_point.size());
    for(SArray1dIter3C<RealT,RealT,RealT> it(ret,_point,_direction);it;it++)
      it.data<0>() = it.data<1>() + it.data<2>() * X[0];
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
  
  RealT CostFunction1dBodyC::Apply1(const VectorT<RealT> &X) const {
    return Cost (X);
  }

  
}
