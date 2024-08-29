// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLGUI_DUTIL3D_HEADER
#define RAVLGUI_DUTIL3D_HEADER 1
/////////////////////////////////////////////////
//! docentry="Ravl.API.Graphics.3D"
//! author="Charles Galambos"
//! date="12/04/1999"

#include "Ravl2/GUI/DObject3D.hh"
#include "Ravl2/Image/RealRGBValue.hh"

#define DTransform3D_RESET       0x0001
#define DTransform3D_ROTATION    0x0002
#define DTransform3D_TRANSLATION 0x0004

namespace Ravl2 {

  ///////////////////////////////////////////////////////
  //: Rotate objects
  class DTransform3DBodyC : public DObjectSet3DBodyC
  {
  public:
    DTransform3DBodyC()
      : mode(DTransform3D_RESET)
      {}
    //: Rotation Constructor.

    DTransform3DBodyC(RealT nAngle, const Vector<RealT,3> &nAxis)
      : mode(DTransform3D_ROTATION),
        angle(nAngle),
        axis(nAxis)
      {}
    //: Rotation Constructor.

    DTransform3DBodyC(RealT nAngle, const Vector<RealT,3> &nAxis,
                      const Vector<RealT,3> &nTrans)
      : mode(DTransform3D_ROTATION | DTransform3D_TRANSLATION),
        angle(nAngle),
        axis(nAxis),
        trans(nTrans)
      {}
    //: Rotation/Translation Constructor.

    DTransform3DBodyC(const Vector<RealT,3> &nTrans)
      : mode(DTransform3D_TRANSLATION),
        trans(nTrans)
      {}
    //: Translation Constructor.

    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

  protected:
    IntT mode;
    RealT angle;
    Vector<RealT,3> axis;
    Vector<RealT,3> trans;
  };

  //: Transform objects
  class DTransform3DC : public DObjectSet3DC
  {
  public:
    DTransform3DC()
      : DObjectSet3DC(*new DTransform3DBodyC())
      {}
    //: Default constructor resets rotation to Identity

    DTransform3DC(RealT nAngle, const Vector<RealT,3> &nAxis)
      : DObjectSet3DC(*new DTransform3DBodyC(nAngle, nAxis))
      {}
    //: Rotation Constructor.

    DTransform3DC(RealT nAngle, const Vector<RealT,3> &nAxis, const Vector<RealT,3> &nTrans)
      : DObjectSet3DC(*new DTransform3DBodyC(nAngle, nAxis, nTrans))
      {}
    //: Rotation/Translation Constructor.

    DTransform3DC(RealT nAngle, const Vector<RealT,3> &nAxis, const Vector<RealT,3> &nTrans,
                  const DObject3DC &obj)
      : DObjectSet3DC(*new DTransform3DBodyC(nAngle, nAxis, nTrans))
      { (*this) += obj;  }
    //: Rotation/Translation Constructor.

    DTransform3DC(const Vector<RealT,3> &nTrans)
      : DObjectSet3DC(*new DTransform3DBodyC(nTrans))
      {}
    //: Translation Constructor.

    DTransform3DC(RealT nAngle, const Vector<RealT,3> &nAxis, const DObject3DC &obj)
      : DObjectSet3DC(*new DTransform3DBodyC(nAngle, nAxis))
      { (*this) += obj; }
    //: Constructor.

  protected:
    DTransform3DBodyC &Body()
      { return dynamic_cast<DTransform3DBodyC &>(DObject3DC::Body()); }

    const DTransform3DBodyC &Body() const
      { return dynamic_cast<const DTransform3DBodyC &>(DObject3DC::Body()); }
  };

}

#endif
