// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Joel Mitchelson"
//! docentry="Ravl.API.3D.Camera Modelling"

#pragma once
#include "Ravl2/3D/PinholeCamera0.hh"

namespace Ravl2
{

  //! Pinhole camera model with first order radial lens distortion
  //! This class adds distortion (1 + k1 * radius) to the pinhole camera model
  //!  Projects 2D image points z from 3D points x according to:<br>
  //!    z[0] = cx + fx*( (R*x + t)[0] / (R*x + t)[2] )* (1 + k1 * r) <br>
  //!    z[1] = cy + fy*( (R*x + t)[1] / (R*x + t)[2] )* (1 + k1 * r) <br>

  template <typename RealT>
  class PinholeCamera1 : public PinholeCamera0<RealT>
  {
  public:
    //! default constructor
    PinholeCamera1() = default;

    //! construct from an undistorted pinhole camera
    explicit PinholeCamera1(const PinholeCamera0<RealT> &c0)
        : PinholeCamera0<RealT>(c0)
    {}
    
    //! Construct from a configuration
    explicit PinholeCamera1(Configuration &config)
    : PinholeCamera0<RealT>(config),
      m_k1(config.getNumber<RealT>("k1","Radial distortion coefficient 1",0.0,-1e4,1e4))
    {}
    
    //! data constructor
    PinholeCamera1(const RealT &cx, const RealT &cy, const RealT &fx, const RealT &fy, const RealT &k1, const Matrix<RealT, 3, 3> &R, const Vector<RealT, 3> &t, const IndexRange<2> &frame)
        : PinholeCamera0<RealT>(cx, cy, fx, fy, R, t, frame), m_k1(k1)
    {}

  public:
    //: first order radial distortion
    [[nodiscard]] RealT &k1()
    {
      return m_k1;
    };

    //: first order radial distortion
    [[nodiscard]] const RealT &k1() const
    {
      return m_k1;
    };

  public:
    //: project 3D point in space to 2D image point
    //  Projects according to:<br>
    //    z[0] = cx + fx*( (R*x + t)[0] / (R*x + t)[2] )* (1 + k1 * r) <br>
    //    z[1] = cy + fy*( (R*x + t)[1] / (R*x + t)[2] )* (1 + k1 * r) <br>
    //  Can result in a divide-by-zero for degenerate points.
    //  See projectCheck if this is to be avoided.
    void project(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx = (this->m_R * x) + this->m_t;
      RealT dx = this->m_fx * Rx[0] / Rx[2];
      RealT dy = this->m_fy * Rx[1] / Rx[2];
      RealT r = std::sqrt(dx * dx + dy * dy);
      // Map from undistorted to distorted point
      z[0] = this->m_cx + dx * (1 + this->m_k1 * r);
      z[1] = this->m_cy + dy * (1 + this->m_k1 * r);
    }

    //: project 3D point in space to 2D image point
    // The same as project(...) but checks that the point
    // is not degenerate.
    bool projectCheck(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx = (this->m_R * x) + this->m_t;
      if(isNearZero(Rx[2], RealT(1E-3)))
        return false;
      RealT dx = this->m_fx * Rx[0] / Rx[2];
      RealT dy = this->m_fy * Rx[1] / Rx[2];
      RealT r = std::sqrt(dx * dx + dy * dy);
      z[0] = this->m_cx + dx * (1 + this->m_k1 * r);
      z[1] = this->m_cy + dy * (1 + this->m_k1 * r);
      return true;
    }

    //! Inverse projection up to a scale factor
    //! origin + lambda*projectInverseDirection is the camera ray
    //! corresponding to image point z.
    void projectInverseDirection(Vector<RealT, 3> &x, const Vector<RealT, 2> &z) const
    {
      // Map from distorted to undistorted point
      RealT dx = z[0] - this->m_cx;
      RealT dy = z[1] - this->m_cy;
      RealT r = std::sqrt(dx * dx + dy * dy);
      RealT rescale(1.0);
      if(r > 1 && this->m_k1 > 0 && 1 + 4 * this->m_k1 * r >= 0) {
        rescale = (-1 + std::sqrt(1 + 4 * this->m_k1 * r)) / (2 * this->m_k1 * r);
      }
      Vector<RealT, 3> Rx;
      Rx[0] = dx * rescale / this->m_fx;
      Rx[1] = dy * rescale / this->m_fy;
      Rx[2] = 1;
      // TMul(this->m_R,Rx,x);
      x = this->m_R.transpose() * Rx;
    }

    //! The Jacobian matrix of the projection function.
    void projectJacobian(Matrix<RealT, 2, 3> &Jz, const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx = (this->m_R * x) + this->m_t;
      RealT r_Rx2_2 = 1 / (Rx[2] * Rx[2]);
      RealT dz0dx0 = this->m_fx * (this->m_R(0, 0) * Rx[2] - this->m_R(2, 0) * Rx[0]) * r_Rx2_2;
      RealT dz0dx1 = this->m_fx * (this->m_R(0, 1) * Rx[2] - this->m_R(2, 1) * Rx[0]) * r_Rx2_2;
      RealT dz0dx2 = this->m_fx * (this->m_R(0, 2) * Rx[2] - this->m_R(2, 2) * Rx[0]) * r_Rx2_2;
      RealT dz1dx0 = this->m_fy * (this->m_R(1, 0) * Rx[2] - this->m_R(2, 0) * Rx[1]) * r_Rx2_2;
      RealT dz1dx1 = this->m_fy * (this->m_R(1, 1) * Rx[2] - this->m_R(2, 1) * Rx[1]) * r_Rx2_2;
      RealT dz1dx2 = this->m_fy * (this->m_R(1, 2) * Rx[2] - this->m_R(2, 2) * Rx[1]) * r_Rx2_2;
      RealT dx = this->m_fx * Rx[0] / Rx[2];
      RealT dy = this->m_fy * Rx[1] / Rx[2];
      RealT r = std::sqrt(dx * dx + dy * dy);
      RealT a = 1 + this->m_k1 * r;
      Jz(0, 0) = dz0dx0 * a;
      Jz(0, 1) = dz0dx1 * a;
      Jz(0, 2) = dz0dx2 * a;
      Jz(1, 0) = dz1dx0 * a;
      Jz(1, 1) = dz1dx1 * a;
      Jz(1, 2) = dz1dx2 * a;
      if(r > 1 && this->m_k1 > 0) {
        Jz(0, 0) += dx * this->m_k1 * (dz0dx0 + dz1dx0) / r;
        Jz(0, 1) += dx * this->m_k1 * (dz0dx1 + dz1dx1) / r;
        Jz(0, 2) += dx * this->m_k1 * (dz0dx2 + dz1dx2) / r;
        Jz(1, 0) += dy * this->m_k1 * (dz0dx0 + dz1dx0) / r;
        Jz(1, 1) += dy * this->m_k1 * (dz0dx1 + dz1dx1) / r;
        Jz(1, 2) += dy * this->m_k1 * (dz0dx2 + dz1dx2) / r;
      }
    }

    //: Return an undistorted image point for a PinholeCamera0C model
    Vector<RealT, 2> undistort(const Vector<RealT, 2> &z) const
    {
      // Map from distorted to undistorted image
      RealT dx = z[0] - this->m_cx;
      RealT dy = z[1] - this->m_cy;
      RealT d = std::sqrt(dx * dx + dy * dy);
      RealT rescale(1.0);
      if(d > 1 && this->m_k1 > 0 && 1 + 4 * this->m_k1 * d >= 0) {
        rescale = (-1 + std::sqrt(1 + 4 * this->m_k1 * d)) / (2 * this->m_k1 * d);
      }
      return toVector<RealT>(this->m_cx + rescale * dx, this->m_cy + rescale * dy);
    }

    //: Transform from a simple pinhole model point to a distorted image point
    Vector<RealT, 2> distort(const Vector<RealT, 2> &z) const
    {
      RealT dx = z[0] - this->m_cx;
      RealT dy = z[1] - this->m_cy;
      RealT d = std::sqrt(dx * dx + dy * dy);
      RealT rescale(1.0);
      if(d > 1) {
        rescale = (1 + this->m_k1 * d);
      }
      return toVector<RealT>(this->m_cx + rescale * dx, this->m_cy + rescale * dy);
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
      archive(cereal::base_class<PinholeCamera0<RealT>>(this),
              cereal::make_nvp("k1", this->m_k1));
    }

  protected:
    RealT m_k1 = 0;
  };

  extern template class PinholeCamera1<float>;

};// namespace Ravl2
