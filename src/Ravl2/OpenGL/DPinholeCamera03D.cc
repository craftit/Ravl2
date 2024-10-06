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

  bool DPinholeCamera03DBodyC::GUIRender([[maybe_unused]] Canvas3DC &c3d) const
  {
    // get camera
    const PinholeCamera0<float>& cam = camera;

    // virtual image size
    auto sx = float(canvas_region.range(1).size());
    auto sy = float(canvas_region.range(0).size());

    // create camera projection matrix
    float matrixProject[16] =
    {
      2.0f*cam.fx()/sx, 0.0, 0.0, 0.0,
      0.0, -2.0f*cam.fy()/sy, 0.0,  0.0,
      (2.0f*cam.cx()/sx)-1.0f, 1.0f-(2.0f*cam.cy()/sy), 1.01f,  1.0f,
      0.0, 0.0, -1.0, 0.0
    };

    // create homogeneous transform matrix
    float matrixModelView[16] =
    {
      cam.R()(0,0),  cam.R()(1,0),  cam.R()(2,0), 0.0,
      cam.R()(0,1),  cam.R()(1,1),  cam.R()(2,1), 0.0,
      cam.R()(0,2),  cam.R()(1,2),  cam.R()(2,2), 0.0,
      cam.t()[0],     cam.t()[1],     cam.t()[2],    1.0
    };

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(matrixProject);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matrixModelView);

    return true;
  }

}
