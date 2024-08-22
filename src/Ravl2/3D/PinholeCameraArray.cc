// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001-12, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#include "Ravl2/Matrix.hh"
#include "Ravl2/Vector.hh"
#include "Ravl2/3D/PinholeCamera0.hh"
#include "Ravl2/3D/PinholeCamera1.hh"
#include "Ravl2/3D/PinholeCamera2.hh"
#include "Ravl2/3D/PinholeCamera3.hh"
#include "Ravl2/3D/PinholeCameraArray.hh"

namespace Ravl3DN
{

  PinholeCameraArrayC::PinholeCameraArrayC(unsigned size, unsigned dist)
      : m_distortion(dist), m_cameras(size)
  {
    SArray1dIterC<PinholeCameraC> it(m_cameras);
    // Initialise the correct camera model in the array
    for(; it; it++) {
      switch(dist) {
        case 3:
          it.Data() = PinholeCamera3C();
          break;
        case 2:
          it.Data() = PinholeCamera2C();
          break;
        case 1:
          it.Data() = PinholeCamera1C();
          break;
        default:
          it.Data() = PinholeCamera0C();
          break;
      }
    }
  }

  // Triangulate points in a set of camera images
  bool
  PinholeCameraArrayC::Triangulate(Vector<RealT, 3> &pt3D, const std::vector<std::tuple<unsigned, Vector<RealT, 2>>> &points) const
  {
    IntT iNum = points.size();
    if(iNum < 2) {
      return false;
    }

    // Triangulate the first 2 points
    std::tuple<unsigned, Vector<RealT, 2>> imgPt1 = points.Nth(0);
    std::tuple<unsigned, Vector<RealT, 2>> imgPt2 = points.Nth(1);
    Vector<RealT, 3> orig1, orig2;
    m_cameras[imgPt1.data<0>()].Origin(orig1);
    m_cameras[imgPt2.data<0>()].Origin(orig2);
    Vector<RealT, 3> dir1, dir2;
    m_cameras[imgPt1.data<0>()].ProjectInverseDirection(dir1, imgPt1.data<1>());
    m_cameras[imgPt2.data<0>()].ProjectInverseDirection(dir2, imgPt2.data<1>());
    dir1.MakeUnit();
    dir2.MakeUnit();
    double dN1N2 = dir1.Dot(dir2);
    // Check not singular
    if(std::abs(dN1N2) < 1e-12) {
      return false;
    }
    Vector<RealT, 3> t = orig2 - orig1;
    double d1 = t.Dot(dir1 - dir2 * dN1N2) / (1 - dN1N2 * dN1N2);
    double d2 = t.Dot(dir1 * dN1N2 - dir2) / (1 - dN1N2 * dN1N2);
    pt3D = (orig1 + dir1 * d1 + orig2 + dir2 * d2) * 0.5;
    if(iNum == 2) {
      return true;
    }

    // Now minimise the image plane error to all points
    Tensor<RealT, 2> J(2 * iNum, 3);
    VectorT<RealT> Z(2 * iNum);
    IntT it, iCam;
    for(it = 0; it < 10; it++) {
      // Construct residual vector and jacobian matrix
      for(iCam = 0; iCam < iNum; iCam++) {
        std::tuple<unsigned, Vector<RealT, 2>> imgPt1 = points.Nth(iCam);
        // Get the jacobian
        FTensor<RealT, 2><2, 3> Jz;
        m_cameras[imgPt1.data<0>()].ProjectJacobian(Jz, pt3D);
        IntT i, j;
        for(i = 0; i < 2; i++)
          for(j = 0; j < 3; j++) J[2 * iCam + i][j] = Jz[i][j];
        // Get the residual
        Vector<RealT, 2> imgPt;
        m_cameras[imgPt1.data<0>()].Project(imgPt, pt3D);
        Z[2 * iCam] = imgPt[0] - imgPt1.data<1>()[0];
        Z[2 * iCam + 1] = imgPt[1] - imgPt1.data<1>()[1];
      }
      // Find the update to solve the least squares problem J*d=Z
      Tensor<RealT, 2> JTJ = J.T() * J;
      VectorT<RealT> JTZ = J.T() * Z;
      JTJ.InverseIP();
      VectorT<RealT> d = JTJ * JTZ;
      pt3D -= Vector<RealT, 3>(d[0], d[1], d[2]);
    }

    return true;
  }

  std::istream &operator>>(std::istream &s, PinholeCameraArrayC &cameras)
  {
    unsigned count, dist;
    s >> count >> dist;
    cameras = PinholeCameraArrayC(count, dist);
    SArray1dIterC<PinholeCameraC> it(cameras.Array());
    for(; it; it++) {
      s >> it.Data();
    }
    return s;
  }

  std::ostream &operator<<(std::ostream &s, const PinholeCameraArrayC &camera)
  {
    s << camera.Array().size() << " " << camera.DistortionModel() << endl;
    SArray1dIterC<PinholeCameraC> it(camera.Array());
    for(; it; it++) {
      s << it.Data();
    }
    return s;
  }

  BinIStreamC &operator>>(BinIStreamC &s, PinholeCameraArrayC &cameras)
  {
    unsigned count, dist;
    s >> count >> dist;
    cameras = PinholeCameraArrayC(count, dist);
    SArray1dIterC<PinholeCameraC> it(cameras.Array());
    for(; it; it++) {
      s >> it.Data();
    }
    return s;
  }

  BinOStreamC &operator<<(BinOStreamC &s, const PinholeCameraArrayC &camera)
  {
    s << camera.Array().size() << camera.DistortionModel();
    SArray1dIterC<PinholeCameraC> it(camera.Array());
    for(; it; it++) {
      s << it.Data();
    }
    return s;
  }
};// namespace Ravl3DN
