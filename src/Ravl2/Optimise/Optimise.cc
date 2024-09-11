// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/Optimise.hh"
#include "Ravl2/PatternRec/CostInvert.hh"
#include "Ravl2/VirtualConstructor.hh"
#include "Ravl2/BinStream.hh"

namespace Ravl2 {

  //: Constructor from xml factory.

  OptimiseBodyC::OptimiseBodyC (const XMLFactoryContextC & factory)
   : _name(factory.AttributeString("name",factory.Name()))
  {}


  OptimiseBodyC::OptimiseBodyC (const std::string &name)
    :_name(name)
  {}
  
  OptimiseBodyC::OptimiseBodyC (const std::string &name, std::istream &)
    : _name(name)
  {}

  //: Load from stream.
  OptimiseBodyC::OptimiseBodyC (std::istream &strm)
    : RCBodyVC(strm)
  {
    strm >> _name;
  }
    
  //: Load from stream.
  OptimiseBodyC::OptimiseBodyC (Binstd::unique_ptr<std::istream> &strm) 
    :  RCBodyVC(strm)
  {
    strm >> _name;
  }
  
  VectorT<RealT> OptimiseBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    RealT startCost = domain.Cost(domain.StartX());
    return MinimalX (domain,startCost,minimumCost);
  }
  
  VectorT<RealT> OptimiseBodyC::MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const
  {
    return MinimalX (domain,minimumCost);
  }
  
  VectorT<RealT> OptimiseBodyC::MaximalX (const CostC &domain, RealT &maximumCost) const
  {
    CostInvertC inverse (domain);
    VectorT<RealT> minimumX = MinimalX (inverse,maximumCost);
    maximumCost = -maximumCost;
    return minimumX;
  }
  
  VectorT<RealT> OptimiseBodyC::MaximalX (const CostC &domain, RealT startCost, RealT &maximumCost) const
  {
    CostInvertC inverse (domain);
    VectorT<RealT> minimumX = MinimalX (inverse,startCost,maximumCost);
    maximumCost = -maximumCost;
    return minimumX;
  }
  
  const std::string OptimiseBodyC::GetInfo () const
  {
    //Strstd::unique_ptr<std::ostream> stream;
    //stream << "Numerical Optimization Base Class Type: " << _name;
    //stream.String();
    return std::string("Numerical Optimization Base Class Type: ") + _name;
  }
  
  const std::string OptimiseBodyC::GetName () const
  { return _name; }
  
  
  bool OptimiseBodyC::Save (std::ostream &out) const
  { 
    if(!RCBodyVC::Save (out))
      return false;
    out << _name;
    return true;
  }
  
  //: Writes object to stream, can be loaded using constructor
  
  bool OptimiseBodyC::Save(Binstd::unique_ptr<std::ostream> &out) const 
  { 
    if(!RCBodyVC::Save (out)) 
      return false;
    out << _name;
    return true;
  }

  // ------------------------------------------------------------------------
  
  OptimiseC::OptimiseC ()
  {
  }
  
  OptimiseC::OptimiseC (std::istream &in)
    : RCHandleVC<OptimiseBodyC>(RAVL_VIRTUALCONSTRUCTOR(in,OptimiseBodyC))
  {
    CheckHandleType(Body());
  }
  
  OptimiseC::OptimiseC (OptimiseBodyC &oth)
    :RCHandleVC<OptimiseBodyC> (oth)
  {}

  OptimiseC::OptimiseC (OptimiseBodyC *oth)
    :RCHandleVC<OptimiseBodyC> (oth)
  {}

}





