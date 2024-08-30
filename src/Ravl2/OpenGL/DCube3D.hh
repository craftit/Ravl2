// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="Charles Galambos"
//! date="12/04/99"

#pragma once

#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/OpenGL/DObject3D.hh"

namespace Ravl2 {

  //: Body of a  object set in a 3D world.
  class DCube3DBodyC
    : public DObject3DBodyC
  {
  public:
    DCube3DBodyC(
      const Vector<RealT,3> &nDiag = Vector<RealT,3>(1, 1, 1),
      const RealRGBValueC &col = RealRGBValueC(1, 0, 0))
      : diag(nDiag),
      colour(col)
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
    Vector<RealT,3> diag;
    RealRGBValueC colour;
  };


}

