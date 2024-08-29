// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////////

#include <GL/gl.h>
#include <GL/glu.h>
#include "Ravl2/OpenGL/DViewPoint3D.hh"
#include "Ravl2/OpenGL/Canvas3D.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  //: Render object.
  bool DViewPoint3DBodyC::GUIRender(Canvas3DC &canvas) const
  {
    //cerr << "DViewPoint3DBodyC::Render(), Called. \n";
    // Setup GL stuff.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    RealT dAspect = 1;//canvas.size()[0]/canvas.size()[1];
    ONDEBUG(std::cerr << "gluPerspective(), FOV=" << fov <<" Aspect:" << dAspect << " Near=" << m_dNear << " Far=" << m_dFar << "\n");
    gluPerspective(fov, dAspect, m_dNear, m_dFar);
    gluLookAt(eye[0],    eye[1],    eye.Z(),
              centre[0], centre[1], centre.Z(),
              up[0],     up[1],     up.Z());

    //glDepthRange(1,5);

    //we dont need to touch model matrix here
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();

    //glTranslatef(0,0,-15);

    return true;
  }

}
