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

    //! Triangulate a 3-D point from its image in two or more cameras.
    //! @param pt3D Set to the point.
    //! @param points (camera index, image point) pairs.
    //! @return false for fewer than 2 points, a bad camera index, or (nearly) parallel first two rays.
    //! The first two rays give a midpoint estimate, refined over all points by Gauss-Newton on image error.
    //! (This was RAVL1 code that didn't compile, and its singularity test rejected perpendicular rays.)
    bool Triangulate(Vector<RealT, 3> &pt3D, const std::vector<std::tuple<unsigned, Vector<RealT, 2>>> &points) const
    {
      const auto numPoints = points.size();
      if(numPoints < 2)
        return false;
      for(const auto &point : points)
        if(std::get<0>(point) >= m_cameras.size())
          return false;

      // Midpoint of the closest approach of the first two rays.
      const auto &cam1 = m_cameras[std::get<0>(points[0])];
      const auto &cam2 = m_cameras[std::get<0>(points[1])];
      const Vector<RealT, 3> orig1 = cam1.origin();
      const Vector<RealT, 3> orig2 = cam2.origin();
      Vector<RealT, 3> dir1, dir2;
      cam1.projectInverseDirection(dir1, std::get<1>(points[0]));
      cam2.projectInverseDirection(dir2, std::get<1>(points[1]));
      dir1.normalize();
      dir2.normalize();
      const RealT cosAngle = dir1.dot(dir2);
      const RealT denom = RealT(1) - cosAngle * cosAngle;
      if(denom < RealT(1e-12))
        return false;
      const Vector<RealT, 3> t = orig2 - orig1;
      const RealT d1 = t.dot(dir1 - dir2 * cosAngle) / denom;
      const RealT d2 = t.dot(dir1 * cosAngle - dir2) / denom;
      pt3D = ((orig1 + dir1 * d1) + (orig2 + dir2 * d2)) * RealT(0.5);
      if(numPoints == 2)
        return true;

      // Minimise the image error over all points.
      Eigen::Matrix<RealT, Eigen::Dynamic, 3> J(2 * numPoints, 3);
      Eigen::Matrix<RealT, Eigen::Dynamic, 1> residual(2 * numPoints);
      for(int iter = 0; iter < 10; ++iter) {
        for(size_t i = 0; i < numPoints; ++i) {
          const auto &camera = m_cameras[std::get<0>(points[i])];
          Matrix<RealT, 2, 3> Jz;
          camera.projectJacobian(Jz, pt3D);
          J.template block<2, 3>(Eigen::Index(2 * i), 0) = Jz;
          Vector<RealT, 2> projected;
          camera.project(projected, pt3D);
          residual.template segment<2>(Eigen::Index(2 * i)) = projected - std::get<1>(points[i]);
        }
        const Vector<RealT, 3> step = (J.transpose() * J).ldlt().solve(J.transpose() * residual);
        if(!step.allFinite())
          break;
        pt3D -= step;
        if(step.norm() < RealT(1e-9) * (RealT(1) + pt3D.norm()))
          break;
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
