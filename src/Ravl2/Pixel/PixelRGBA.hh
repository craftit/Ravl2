// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="24/01/2001"

#pragma once

#include <array>
#include "Ravl2/Pixel/PixelRGB.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  //: RGBA Pixel.

  template <class CompT>
  class PixelRGBA : public Vector<CompT, 4>
  {
  public:
    //! Default constructor.
    // Creates an undefined value.
    PixelRGBA() = default;

    //! Construct from component values.
    PixelRGBA(const CompT &r, const CompT &g, const CompT &b, const CompT &a = CompT())
    {
      (*this)[0] = r;
      (*this)[1] = g;
      (*this)[2] = b;
      (*this)[3] = a;
    }

    //! Construct from another component type.
    template <class OCompT>
    PixelRGBA(const PixelRGBA<OCompT> &oth)
    {
      (*this)[0] = oth.Red();
      (*this)[1] = oth.Green();
      (*this)[2] = oth.Blue();
      (*this)[3] = oth.Alpha();
    }

    //! Constructor from base class.
    PixelRGBA(const Vector<CompT, 4> &v)
        : Vector<CompT, 4>(v)
    {}

    //! Construct from another 3 rgb pixel.
    template <class OCompT>
    PixelRGBA(PixelRGB<OCompT> &oth, const CompT &alpha = {})
    {
      (*this)[0] = oth.Red();
      (*this)[1] = oth.Green();
      (*this)[2] = oth.Blue();
      (*this)[3] = alpha;
    }

    //! Returns the level of the red component.
    inline const CompT &Red() const
    {
      return (*this)[0];
    }

    //! Returns the level of the green component.
    inline const CompT &Green() const
    {
      return (*this)[1];
    }

    //! Returns the level of the blue component.
    inline const CompT &Blue() const
    {
      return (*this)[2];
    }

    //! Returns the level of the alpha component.
    inline const CompT &Alpha() const
    {
      return (*this)[3];
    }

    //! Returns the level of the red component.
    inline CompT &Red()
    {
      return (*this)[0];
    }

    //! Returns the level of the green component.
    inline CompT &Green()
    {
      return (*this)[1];
    }

    //! Returns the level of the blue component.
    inline CompT &Blue()
    {
      return (*this)[2];
    }

    //! Returns the level of the alpha component.
    inline CompT &Alpha()
    {
      return (*this)[3];
    }
  };

}// namespace Ravl2

