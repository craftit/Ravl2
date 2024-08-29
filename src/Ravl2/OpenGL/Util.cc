// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////

#include "Ravl2/GUI/Util.hh"
#include "Ravl2/GUI/Canvas3D.hh"

namespace Ravl2 {

  //: Render object.
  bool DTransform3DBodyC::GUIRender(Canvas3DC &c3d) const
  {
    //glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    //glMatrixMode(GL_PROJECTION);
    if(mode & DTransform3D_RESET)
      glLoadIdentity();

    if(mode & DTransform3D_ROTATION)
      glRotated(angle, axis[0], axis[1], axis.Z());

    if(mode & DTransform3D_TRANSLATION)
      glTranslated(trans[0], trans[1], trans.Z());

    for(DLIterC<DObject3DC> it(parts); it.valid(); it.next())
      it.Data().GUIRenderDL(c3d);

    //glPopMatrix();
    return true;
  }
}
