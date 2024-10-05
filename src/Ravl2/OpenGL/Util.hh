// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="Charles Galambos"
//! date="12/04/1999"

#pragma once

#include "Ravl2/OpenGL/DObject3D.hh"
#include "Ravl2/Pixel/Pixel.hh"

#define DTransform3D_RESET       0x0001
#define DTransform3D_ROTATION    0x0002
#define DTransform3D_TRANSLATION 0x0004

namespace Ravl2
{

  ///////////////////////////////////////////////////////
  //: Rotate objects
  class DTransform3DBodyC
    : public DObjectSet3DBodyC
  {
  public:
    DTransform3DBodyC()
      : mode(DTransform3D_RESET)
      {}
    //: Rotation Constructor.

    DTransform3DBodyC(float nAngle, const Vector<float,3> &nAxis)
      : mode(DTransform3D_ROTATION),
        angle(nAngle),
        axis(nAxis)
      {}
    //: Rotation Constructor.

    DTransform3DBodyC(float nAngle, const Vector<float,3> &nAxis,
                      const Vector<float,3> &nTrans)
      : mode(DTransform3D_ROTATION | DTransform3D_TRANSLATION),
        angle(nAngle),
        axis(nAxis),
        trans(nTrans)
      {}
    //: Rotation/Translation Constructor.

    DTransform3DBodyC(const Vector<float,3> &nTrans)
      : mode(DTransform3D_TRANSLATION),
        trans(nTrans)
      {}
    //: Translation Constructor.

    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

  protected:
    int mode = 0;
    float angle = 0;
    Vector<float,3> axis = toVector<float>(0,0,1);
    Vector<float,3> trans = toVector<float>(0,0,0);
  };


}

