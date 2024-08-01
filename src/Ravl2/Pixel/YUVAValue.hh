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

  //: YUVA Pixel base class.

  template <class CompT>
  class YUVAValueC : public Vector<CompT, 4>
  {
  public:
    YUVAValueC()
    {}
    //: Default constructor.
    // Creates an undefined value.

    YUVAValueC(const CompT &y, const CompT &u, const CompT &v, const CompT &a)
    {
      this->data[0] = y;
      this->data[1] = u;
      this->data[2] = v;
      this->data[3] = a;
    }
    //: Construct from component values.

    YUVAValueC(const Vector<CompT, 4> &v)
        : Vector<CompT, 4>(v)
    {}
    //: Constructor from base class.

    template <class OCompT>
    YUVAValueC(YUVAValueC<OCompT> &oth)
    {
      this->data[0] = oth.Y();
      this->data[1] = oth.U();
      this->data[2] = oth;
    }
    //: Construct from another component type.

    inline const CompT &Y() const
    {
      return this->data[0];
    }
    // Returns the level of the Y component.

    inline const CompT &U() const
    {
      return this->data[1];
    }
    // Returns the level of the U component.

    inline const CompT &V() const
    {
      return this->data[2];
    }
    // Returns the level of the V component.

    inline const CompT &A() const
    {
      return this->data[3];
    }
    // Returns the level of the V component.

    inline CompT &Y()
    {
      return this->data[0];
    }
    // Returns the level of the Y component.

    inline CompT &U()
    {
      return this->data[1];
    }
    // Returns the level of the U component.

    inline CompT &V()
    {
      return this->data[2];
    }
    // Returns the level of the V component.

    inline CompT &A()
    {
      return this->data[3];
    }
    // Returns the level of the V component.
  };

  template <class CompT>
  inline std::istream &operator>>(std::istream &strm, YUVAValueC<CompT> &val)
  {
    return strm >> ((Vector<CompT, 4> &)(val));
  }
  //: Stream input.

  template <class CompT>
  inline std::ostream &operator<<(std::ostream &strm, const YUVAValueC<CompT> &val)
  {
    return strm << ((const Vector<CompT, 4> &)(val));
  }
  //: Stream output.

}// namespace Ravl2
