// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////////

#include <GL/gl.h>
#include <GL/glut.h>
#include "Ravl2/OpenGL/DViewPoint3D.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  //: Render object.
  bool DViewPoint3DBodyC::GUIRender(Canvas3D &canvas) const
  {
    (void) canvas;
    // Setup GL stuff.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    RealT dAspect = 1;//canvas.size()[0]/canvas.size()[1];
    ONDEBUG(std::cerr << "gluPerspective(), FOV=" << fov <<" Aspect:" << dAspect << " Near=" << m_dNear << " Far=" << m_dFar << "\n");
    gluPerspective(double(fov), double(dAspect), double(m_dNear), double(m_dFar));
    gluLookAt(double(eye[0]),    double(eye[1]),    double(eye[2]),
	      double(centre[0]), double(centre[1]), double(centre[2]),
		     double(up[0]),     double(up[1]),     double(up[2]));
    
    return true;
  }

}
