// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="Charles Galambos"
//! date="18/06/1999"

#pragma once

#include "Ravl2/OpenGL/DObject3D.hh"

namespace Ravl2 {

  //: Setup a light in a 3D world.

  class DLight3D
    : public DObject3D
  {
  public:
    //: Constructor.
    DLight3D(const PixelRGB32F &nCol, const Vector<float,3> &nPos, int nn = 0)
      : n(nn),
	colour(nCol),
	pos(nPos),
	spot(false)
    {}

    //: Render object.
    bool GUIRender(Canvas3D &c3d) const override;

    //: Get center of object.
    // defaults to 0,0,0
    [[nodiscard]] Vector<float,3> GUICenter() const override
      { return Vector<float,3>({0, 0, 0}); }

    //: Get extent of object.
    // defaults to 1
    [[nodiscard]] float GUIExtent() const override
      { return 1; }

  protected:
    //: Convert Light number.
    [[nodiscard]] GLenum LightNo(int no) const;

    int n = 0;           //!< Number of light. 0-7
    PixelRGB32F colour; //!< Colour of light.
    Vector<float,3> pos;    //!< Position of light.
    bool spot = false;        //!< Spot light ?
    Vector<float,3> dir;    //!< Direction of spot light.
    float  ang = 0;       //!< angle of light.
  };

}


