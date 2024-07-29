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

  //! YUV Pixel base class.
  
  template<class CompT>
  class YUVValueC 
    : public Vector<CompT,3>
  {
  public:
    //! Default constructor.
    // Creates an undefined value.
    YUVValueC() = default;

    //! Construct from component values.
    YUVValueC(const CompT &y,const CompT &u,const CompT &v) {
      this->data[0] =y;
      this->data[1] =u;
      this->data[2] =v;
    }

    //! Constructor from base class.
    YUVValueC(const Vector<CompT,3> &v)
      : Vector<CompT,3>(v)
    {}

    template<class OCompT>
    YUVValueC(YUVValueC<OCompT> &oth) {
      this->data[0] = oth.Y();
      this->data[1] = oth.U();
      this->data[2] = oth;
    }
    //: Construct from another component type.

    void Set(const CompT &y,const CompT &u,const CompT &v) {
      this->data[0] =y;
      this->data[1] =u;
      this->data[2] =v;
    }
    //: Set the values.
    
    void LimitYUV(const CompT &minY,const CompT &maxY,const CompT &minUV,const CompT &maxUV) {
      if(this->data[0] < minY)
        this->data[0] = minY;
      if(this->data[0] > maxY)
        this->data[0] = maxY;
      
      if(this->data[1] < minUV)
        this->data[1] = minUV;
      if(this->data[1] > maxUV)
        this->data[1] = maxUV;
      
      if(this->data[2] < minUV)
        this->data[2] = minUV;
      if(this->data[2] > maxUV)
        this->data[2] = maxUV;
    }
    //: Limit colour values.
    
    inline const CompT & Y() const
    { return this->data[0]; }
    //: Returns the level of the Y component.
    
    inline const CompT & U() const
    { return this->data[1]; }
    //: Returns the level of the U component.
    
    inline const CompT & V() const
    { return this->data[2]; }
    //: Returns the level of the V component.
    
    inline CompT & Y() 
    { return this->data[0]; }
    //: Returns the level of the Y component.
    
    inline CompT & U()
    { return this->data[1]; }
    //: Returns the level of the U component.
    
    inline CompT & V()
    { return this->data[2]; }
    //: Returns the level of the V component.

  };

  template<class CompT>
  inline
  std::istream &operator>>(std::istream &strm,YUVValueC<CompT> &val) 
  { return strm >> ((Vector<CompT,3> &)(val)); }
  //: Stream input.
  
  template<class CompT>
  inline
  std::ostream &operator<<(std::ostream &strm,const YUVValueC<CompT> &val) 
  { return strm << ((const Vector<CompT,3> &)(val)); }
  //: Stream output.

}


#endif
