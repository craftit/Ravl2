// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include "Ravl2/Image/RGBValue.hh"

namespace Ravl2
{

  //! BGR Pixel.
  
  template<class CompT>
  class BGRValueC
    : public Vector<CompT,3>
  {
  public:
    BGRValueC() = default;

    //! Constructs a colour from three colour components.
    // Note: the arguments are given in the order r,g,b.
    inline BGRValueC(CompT r ,
		     CompT g = 0,
		     CompT b = 0) {
      this->data[2] = r;
      this->data[1] = g;
      this->data[0] = b;
    }

    //! Constructs the colour from the triple RGB colour 'rgb'
    BGRValueC(const RGBValueC<CompT> & rgb) {
      this->data[2] = rgb.Red();
      this->data[1] = rgb.Green();
      this->data[0] = rgb.Blue();
    }

    //! Get an RGB pixel value.
    inline
    RGBValueC<CompT> RGB() const
    { return RGBValueC<CompT>(Red(),Green(),Blue()); }

    //! Returns the level of the red component.
    inline const CompT & Red() const
    { return this->data[2]; }

    //! Returns the level of the green component.
    inline const CompT & Green() const
    { return this->data[1]; }

    //! Returns the level of the blue component.
    inline const CompT & Blue() const
    { return this->data[0]; }

    //! Access to the red component.
    inline CompT & Red()
    { return this->data[2]; }

    //! Access to the green component.
    inline CompT & Green()
    { return this->data[1]; }

    //! Access to the blue component.
    inline CompT & Blue()
    { return this->data[0]; }

  };
}


#endif
