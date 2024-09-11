// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/Optimise.cc"

#include "Ravl/PatternRec/Optimise.hh"
#include "Ravl/PatternRec/CostInvert.hh"
#include "Ravl/VirtualConstructor.hh"
#include "Ravl/BinStream.hh"

namespace RavlN {

  //: Constructor from xml factory.

  OptimiseBodyC::OptimiseBodyC (const XMLFactoryContextC & factory)
   : _name(factory.AttributeString("name",factory.Name()))
  {}


  OptimiseBodyC::OptimiseBodyC (const StringC &name)
    :_name(name)
  {}
  
  OptimiseBodyC::OptimiseBodyC (const StringC &name, std::istream &)
    : _name(name)
  {}

  //: Load from stream.
  OptimiseBodyC::OptimiseBodyC (std::istream &strm)
    : RCBodyVC(strm)
  {
    strm >> _name;
  }
    
  //: Load from stream.
  OptimiseBodyC::OptimiseBodyC (BinIStreamC &strm) 
    :  RCBodyVC(strm)
  {
    strm >> _name;
  }
  
  VectorC OptimiseBodyC::MinimalX (const CostC &domain, RealT &minimumCost) const
  {
    RealT startCost = domain.Cost(domain.StartX());
    return MinimalX (domain,startCost,minimumCost);
  }
  
  VectorC OptimiseBodyC::MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const
  {
    return MinimalX (domain,minimumCost);
  }
  
  VectorC OptimiseBodyC::MaximalX (const CostC &domain, RealT &maximumCost) const
  {
    CostInvertC inverse (domain);
    VectorC minimumX = MinimalX (inverse,maximumCost);
    maximumCost = -maximumCost;
    return minimumX;
  }
  
  VectorC OptimiseBodyC::MaximalX (const CostC &domain, RealT startCost, RealT &maximumCost) const
  {
    CostInvertC inverse (domain);
    VectorC minimumX = MinimalX (inverse,startCost,maximumCost);
    maximumCost = -maximumCost;
    return minimumX;
  }
  
  const StringC OptimiseBodyC::GetInfo () const
  {
    //StrOStreamC stream;
    //stream << "Numerical Optimization Base Class Type: " << _name;
    //stream.String();
    return StringC("Numerical Optimization Base Class Type: ") + _name;
  }
  
  const StringC OptimiseBodyC::GetName () const
  { return _name; }
  
  
  bool OptimiseBodyC::Save (std::ostream &out) const
  { 
    if(!RCBodyVC::Save (out))
      return false;
    out << _name;
    return true;
  }
  
  //: Writes object to stream, can be loaded using constructor
  
  bool OptimiseBodyC::Save(BinOStreamC &out) const 
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





