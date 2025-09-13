// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////

#include "Ravl2/OpenGL/DTransform.hh"

namespace Ravl2 {

  //: Render object.
  bool DTransform3DBodyC::GUIRender(Canvas3D &c3d) const
  {
    (void)c3d;
    //glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    //glMatrixMode(GL_PROJECTION);
    if(mode & DTransform3D_RESET)
      glLoadIdentity();

    if(mode & DTransform3D_ROTATION)
      glRotatef(angle, axis[0], axis[1], axis[2]);

    if(mode & DTransform3D_TRANSLATION)
      glTranslatef(trans[0], trans[1], trans[2]);
    
    DObjectSet3D::GUIRender(c3d);

    //glPopMatrix();
    return true;
  }
}
