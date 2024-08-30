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

  //////////////////////////////////////////////
  //: Draw some lines.

  class DLight3DBodyC
    : public DObject3DBodyC
  {
  public:
    DLight3DBodyC(const RealRGBValueC &nCol, const Vector<RealT,3> &nPos, int nn)
      : n(nn),
	colour(nCol),
	pos(nPos),
	spot(false)
    {}
    //: Constructor.

    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

    virtual Vector<RealT,3> GUICenter() const
      { return Vector<RealT,3>(0, 0, 0); }
    //: Get center of object.
    // defaults to 0,0,0

    virtual RealT GUIExtent() const
      { return 1; }
    //: Get extent of object.
    // defaults to 1

  protected:
    GLenum LightNo(IntT no) const;
    //: Convert Light number.

    IntT n;           // Number of light. 0-7
    RealRGBValueC colour; // Colour of light.
    Vector<RealT,3> pos;    // Position of light.
    bool spot;        // Spot light ?
    Vector<RealT,3> dir;    // Direction of spot light.
    RealT  ang;       // angle of light.
  };

  //: Draw some lines.
  class DLight3DC : public DObject3DC
  {
  public:
    DLight3DC(const RealRGBValueC &col, const Vector<RealT,3> &nPos, int nn = 0)
      : DObject3DC(*new DLight3DBodyC(col, nPos, nn))
    {}
    //: Constructor.
  };
}


