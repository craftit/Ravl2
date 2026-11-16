// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/OpenGL/DPinholeCamera03D.hh"

namespace Ravl2
{

  bool DPinholeCamera03DBodyC::GUIRender([[maybe_unused]] Canvas3D &c3d) const
  {
    // get camera
    const PinholeCamera0<float>& cam = camera;

    // virtual image size
    auto sx = float(canvas_region.range(1).size());
    auto sy = float(canvas_region.range(0).size());

    // OpenCV-convention intrinsics (x = col, y = row) and the world-to-camera extrinsic in
    // the OpenCV frame: the camera's internal values are (row, col)-ordered (see PinholeCamera0).
    const Eigen::Matrix3f K = cam.intrinsicMatrix<CameraCoordinateSystemT::OpenCV>();
    const Eigen::Matrix<float, 3, 4> Rt = cam.extrinsicMatrix<CameraCoordinateSystemT::OpenCV>();

    // create camera projection matrix
    float matrixProject[16] =
    {
      2.0f*K(0,0)/sx, 0.0, 0.0, 0.0,
      0.0, -2.0f*K(1,1)/sy, 0.0,  0.0,
      (2.0f*K(0,2)/sx)-1.0f, 1.0f-(2.0f*K(1,2)/sy), 1.01f,  1.0f,
      0.0, 0.0, -1.0, 0.0
    };

    // create homogeneous transform matrix
    float matrixModelView[16] =
    {
      Rt(0,0),  Rt(1,0),  Rt(2,0), 0.0,
      Rt(0,1),  Rt(1,1),  Rt(2,1), 0.0,
      Rt(0,2),  Rt(1,2),  Rt(2,2), 0.0,
      Rt(0,3),  Rt(1,3),  Rt(2,3), 1.0
    };

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(matrixProject);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matrixModelView);

    return true;
  }

}
