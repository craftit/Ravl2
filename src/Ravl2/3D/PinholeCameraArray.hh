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

namespace Ravl3DN
{

  //: Container for an array of pinhole cameras
  // This wrapper provides IO to construct the appropriate camera models and return access via PinholeCameraC

  class PinholeCameraArrayC
  {
  public:
    PinholeCameraArrayC()
    {}
    // default constructor

    PinholeCameraArrayC(unsigned size, unsigned distortion);
    // Constructor

  public:
    std::vector<PinholeCameraC> &Array()
    {
      return m_cameras;
    }
    //: Access the array of cameras

    const std::vector<PinholeCameraC> &Array() const
    {
      return m_cameras;
    }
    //: Access the array of cameras

    size_t size() const
    {
      return m_cameras.size();
    }
    //: Return the number of cameras in the array

    const PinholeCameraC &operator[](IntT i) const
    {
      return m_cameras[i];
    }
    // Read-only access to the 'i'-th element of the array.

    inline PinholeCameraC &operator[](IntT i)
    {
      return m_cameras[i];
    }
    // Read-write access  to the 'i'-th element of the array.

    unsigned DistortionModel() const
    {
      return m_distortion;
    }
    //: Return the distortion model used
    // i.e 0 = PinholeCamera0C, 1 = PinholeCamera1C etc.
    // If you need to access the camera parameters you can construct a handle from the base handle as follows
    // PinholeCamera1C c(dynamic_cast<PinholeCameraBody1C&>(camera.Body()));

    bool Triangulate(Vector<RealT, 3> &pt3D, const std::vector<std::tuple<unsigned, Vector<RealT, 2>>> &points) const;
    //: Triangulate points in a set of camera images
    // Returns false if <2 points or singular

  protected:
    unsigned m_distortion;
    std::vector<PinholeCameraC> m_cameras;// The reference counted container
  };

  std::istream &operator>>(std::istream &s, PinholeCameraArrayC &camera);
  //:Read camera parameters from a text stream.

  std::ostream &operator<<(std::ostream &s, const PinholeCameraArrayC &camera);
  //:Write camera parameters to a text stream.

};// namespace Ravl3DN
