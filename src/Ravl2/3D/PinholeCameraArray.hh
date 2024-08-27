// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001-12, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.3D.Camera Modelling"

#pragma once

#include <tuple>
#include <vector>
#include "Ravl2/3D/PinholeCamera.hh"

namespace Ravl2
{

  //: Container for an array of pinhole cameras
  // This wrapper provides IO to construct the appropriate camera models and return access via PinholeCameraT

  template <typename PinholeCameraT, typename RealT>
  class PinholeCameraArray
  {
  public:
    PinholeCameraArray()
    = default;
    // default constructor

    PinholeCameraArray(unsigned size, unsigned distortion);
    // Constructor

  public:
    //: Access the array of cameras
    std::vector<PinholeCameraT> &Array()
    {
      return m_cameras;
    }

    //: Access the array of cameras
    const std::vector<PinholeCameraT> &Array() const
    {
      return m_cameras;
    }

    //: Return the number of cameras in the array
    [[nodiscard]] size_t size() const
    {
      return m_cameras.size();
    }

    // Read-only access to the 'i'-th element of the array.
    const PinholeCameraT &operator[](int i) const
    {
      return m_cameras[i];
    }

    // Read-write access  to the 'i'-th element of the array.
    inline PinholeCameraT &operator[](int i)
    {
      return m_cameras[i];
    }

    //: Return the distortion model used
    // i.e 0 = PinholeCamera0C, 1 = PinholeCamera1C etc.
    // If you need to access the camera parameters you can construct a handle from the base handle as follows
    // PinholeCamera1C c(dynamic_cast<PinholeCameraBody1C&>(camera.Body()));
    [[nodiscard]] unsigned DistortionModel() const
    {
      return m_distortion;
    }

    bool Triangulate(Vector<RealT, 3> &pt3D, const std::vector<std::tuple<unsigned, Vector<RealT, 2>>> &points) const
    {
      int iNum = points.size();
      if(iNum < 2) {
        return false;
      }

      // Triangulate the first 2 points
      std::tuple<unsigned, Vector<RealT, 2>> imgPt1 = points[0];
      std::tuple<unsigned, Vector<RealT, 2>> imgPt2 = points[1];
      auto orig1 = m_cameras[imgPt1.template data<0>()].origin();
      auto orig2 = m_cameras[imgPt2.template data<0>()].origin();
      Vector<RealT, 3> dir1, dir2;
      m_cameras[imgPt1.template data<0>()].inverseProjectDirection(dir1, imgPt1.template data<1>());
      m_cameras[imgPt2.template data<0>()].inverseProjectDirection(dir2, imgPt2.template data<1>());
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
      int it, iCam;
      for(it = 0; it < 10; it++) {
        // Construct residual vector and jacobian matrix
        for(iCam = 0; iCam < iNum; iCam++) {
          std::tuple<unsigned, Vector<RealT, 2>> imgPt1a = points[iCam];
          // Get the jacobian
          Matrix<RealT, 2, 3> Jz;
          m_cameras[imgPt1a.template data<0>()].projectJacobian(Jz, pt3D);
          int i, j;
          for(i = 0; i < 2; i++)
            for(j = 0; j < 3; j++) J[2 * iCam + i][j] = Jz[i][j];
          // Get the residual
          Vector<RealT, 2> imgPt;
          m_cameras[imgPt1a.template data<0>()].project(imgPt, pt3D);
          Z[2 * iCam] = imgPt[0] - imgPt1a.template data<1>()[0];
          Z[2 * iCam + 1] = imgPt[1] - imgPt1a.template data<1>()[1];
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

    //: Triangulate points in a set of camera images
    // Returns false if <2 points or singular

  protected:
    unsigned m_distortion = 0;
    std::vector<PinholeCameraT> m_cameras;// The reference counted container
  };

};// namespace Ravl2
