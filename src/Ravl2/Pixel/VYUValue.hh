// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="24/01/2001"

#pragma once
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

  //! VYU Pixel base class.
  
  template<class CompT>
  class VYUValueC 
    : public Vector<CompT,3>
  {
  public:
    VYUValueC()
    {}
    //: Default constructor.
    // Creates an undefined value.

    VYUValueC(const CompT &v,const CompT &y,const CompT &u) {
      this->data[0] = v;
      this->data[1] = y;
      this->data[2] = u;
    }
    //: Construct from component values.
    
    VYUValueC(const Vector<CompT,3> &v)
      : Vector<CompT,3>(v)
    {}
    //: Constructor from base class.
    
    template<class OCompT>
    VYUValueC(VYUValueC<OCompT> &oth) {
      this->data[0] = oth;
      this->data[1] = oth.Y();
      this->data[2] = oth.U();
    }
    //: Construct from another component type.

    void Set(const CompT &v,const CompT &y,const CompT &u) {
      this->data[0] =v;
      this->data[1] =y;
      this->data[2] =u;
    }
    //: Set the values.
    
    inline const CompT & V() const
    { return this->data[0]; }
    //: Returns the level of the V component.
    
    inline const CompT & Y() const
    { return this->data[1]; }
    //: Returns the level of the Y component.
    
    inline const CompT & U() const
    { return this->data[2]; }
    //: Returns the level of the U component.
    
    inline CompT & V()
    { return this->data[0]; }
    //: Returns the level of the V component.
    
    inline CompT & Y() 
    { return this->data[1]; }
    //: Returns the level of the Y component.
    
    inline CompT & U()
    { return this->data[2]; }
    //: Returns the level of the U component.
    
  };

  template<class CompT>
  inline
  std::istream &operator>>(std::istream &strm,VYUValueC<CompT> &val) 
  { return strm >> ((Vector<CompT,3> &)(val)); }
  //: Stream input.
  
  template<class CompT>
  inline
  std::ostream &operator<<(std::ostream &strm,const VYUValueC<CompT> &val) 
  { return strm << ((const Vector<CompT,3> &)(val)); }
  //: Stream output.

}


#endif
