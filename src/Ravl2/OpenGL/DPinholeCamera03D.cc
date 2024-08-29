// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/OpenGL/DPinholeCamera03D.hh"

namespace Ravl2
{
  using namespace Ravl2;

  bool DPinholeCamera03DBodyC::GUIRender(Canvas3DC &c3d) const
  {
    // get camera
    const PinholeCamera0C& cam = camera;

    // virtual image size
    RealT sx = canvas_region.range(1).size();
    RealT sy = canvas_region.range(0).size();

    // create camera projection matrix
    double matrixProject[16] =
    {
      2.0*cam.fx()/sx, 0.0, 0.0, 0.0,
      0.0, -2.0*cam.fy()/sy, 0.0,  0.0,
      (2.0*cam.cx()/sx)-1.0, 1.0-(2.0*cam.cy()/sy), 1.01,  1.0,
      0.0, 0.0, -1.0, 0.0
    };

    // create homogeneous transform matrix
    double matrixModelView[16] =
    {
      cam.R()(0,0),  cam.R()(1,0),  cam.R()(2,0), 0.0,
      cam.R()(0,1),  cam.R()(1,1),  cam.R()(2,1), 0.0,
      cam.R()(0,2),  cam.R()(1,2),  cam.R()(2,2), 0.0,
      cam.t()[0],     cam.t()[1],     cam.t()[2],    1.0
    };

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(matrixProject);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(matrixModelView);

    return true;
  }

}
