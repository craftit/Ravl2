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

  //! @brief Pinhole camera model with 3rd and 5th order radial lens distortion
  //! It uses the polynomial model to undistort the image during back-projection and a numerical approximation to distort the image during projection.
  //! This is the reverse of <a href="Ravl3DN.PinholeCamera3C.html">PinholeCamera2C</a>.

  template <typename RealT>
  class PinholeCamera2 : public PinholeCamera0<RealT>
  {
  public:
    //! default constructor
    PinholeCamera2() = default;

    //! construct from an undistorted pinhole camera
    explicit PinholeCamera2(const PinholeCamera0<RealT> &c0)
        : PinholeCamera0<RealT>(c0)
    {}
    
    //! Construct from a configuration
    explicit PinholeCamera2(Configuration &config)
      : PinholeCamera0<RealT>(config),
        m_sizex(config.getNumber<RealT>("sx","Pixel size in x",1.0,0.0,1e4)),
        m_sizey(config.getNumber<RealT>("sy","Pixel size in y",1.0,0.0,1e4)),
        m_k3(config.getNumber<RealT>("k3","Radial distortion coefficient 3",0.0,-1e4,1e4)),
        m_k5(config.getNumber<RealT>("k5","Radial distortion coefficient 3",0.0,-1e4,1e4))
    {}
    
    //! data constructor
    PinholeCamera2(const RealT &cx, const RealT &cy, const RealT &fx, const RealT &fy, const RealT &sizex, const RealT &sizey, const RealT &k3, const RealT &k5, const Matrix<RealT, 3, 3> &R, const Vector<RealT, 3> &t, const IndexRange<2> &frame)
      : PinholeCamera0<RealT>(cx, cy, fx, fy, R, t, frame),
        m_sizex(sizex),
        m_sizey(sizey),
        m_k3(k3),
        m_k5(k5)
    {}

  public:
    //: 3rd order radial distortion
    const RealT &k3() const
    {
      return this->m_k3;
    };

    //: 5th order radial distortion
    const RealT &k5() const
    {
      return this->m_k5;
    };

    //: Effective pixel size
    const RealT &sizeX() const
    {
      return this->m_sizex;
    };

    //: Effective pixel size
    const RealT &sizeY() const
    {
      return this->m_sizey;
    };

  public:
    //: project 3D point in space to 2D image point
    //  Can result in a divide-by-zero for degenerate points.
    //  See projectCheck if this is to be avoided.
    void project(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      // Distortion-free projection
      Vector<RealT, 3> Rx = (this->m_R * x) + this->m_t;
      RealT dx = this->m_fx * Rx[0] / Rx[2];
      RealT dy = this->m_fy * Rx[1] / Rx[2];
      z = distort(toVector<RealT>(this->m_cx + dx, this->m_cy + dy));
    }

    //: project 3D point in space to 2D image point
    // The same as project(...) but checks that the point
    // is not degenerate.
    bool projectCheck(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      // Distortion-free projection
      Vector<RealT, 3> Rx = (this->m_R * x) + this->m_t;
      if(isNearZero(Rx[2], RealT(1E-3)))
        return false;
      RealT dx = this->m_fx * Rx[0] / Rx[2];
      RealT dy = this->m_fy * Rx[1] / Rx[2];
      z = distort(toVector<RealT>(this->m_cx + dx, this->m_cy + dy));
      return true;
    }

    //:Inverse projection up to a scale factor
    // origin + lambda*projectInverseDirection is the camera ray
    // corresponding to image point z.
    void projectInverseDirection(Vector<RealT, 3> &x, const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 3> Rx;
      Vector<RealT, 2> uz = undistort(z);
      Rx[0] = (uz[0] - this->m_cx) / this->m_fx;
      Rx[1] = (uz[1] - this->m_cy) / this->m_fy;
      Rx[2] = 1.0;
      // TMul(this->m_R, Rx, x);
      x = xt::linalg::dot(xt::transpose(this->m_R), Rx);
    }

    //: Return an undistorted image point for a PinholeCamera0C model
    Vector<RealT, 2> undistort(const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 2> ret = z;
      if(this->m_k3 != 0 || this->m_k5 != 0) {
        RealT xh = this->m_sizex * (z[0] - this->m_cx);
        RealT yh = this->m_sizey * (z[1] - this->m_cy);
        RealT rd = xh * xh + yh * yh;
        RealT scale = 1 + this->m_k3 * rd + this->m_k5 * rd * rd;
        ret[0] = (xh * scale) / this->m_sizex + this->m_cx;
        ret[1] = (yh * scale) / this->m_sizey + this->m_cy;
      }
      return ret;
    }

    //: Transform from a simple pinhole model point to a distorted image point
    Vector<RealT, 2> distort(const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 2> ret = z;
      // NOTE: do not undistort a point greater than one image width/height outside the image as this may not converge
      if((this->m_k3 != 0 || this->m_k5 != 0) && z[0] > RealT(this->m_frame.min(1) - this->m_frame.range(1).size()) && z[0] < RealT(this->m_frame.max(1) + this->m_frame.range(1).size()) && z[1] > RealT(this->m_frame.min(0) - this->m_frame.range(0).size()) && z[1] < RealT(this->m_frame.max(0) + this->m_frame.range(0).size())) {
        RealT xu = this->m_sizex * (z[0] - this->m_cx);
        RealT yu = this->m_sizey * (z[1] - this->m_cy);
        RealT dl = std::sqrt(xu * xu + yu * yu);
        // Calculate distance from the centre to the distorted point
        dl = RealT(undist2dist(double(dl), double(this->m_k3), double(this->m_k5)));
        if(dl >= 0)// if distorting process converged......
        {
          RealT sqdl = dl * dl;
          RealT cudl = sqdl * sqdl;
          RealT scale = 1 + this->m_k3 * sqdl + this->m_k5 * cudl;
          ret[0] = (xu / scale) / this->m_sizex + this->m_cx;
          ret[1] = (yu / scale) / this->m_sizey + this->m_cy;
        }
      }
      return ret;
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
      archive(cereal::base_class<PinholeCamera0<RealT>>(this),
              cereal::make_nvp("sizex", this->m_sizex),
              cereal::make_nvp("sizey", this->m_sizey),
              cereal::make_nvp("k3", this->m_k3),
              cereal::make_nvp("k5", this->m_k5));
    }

  protected:
    /*
     * Function undist2dist: 
     *    ... given the distance between optical center and ideal projected point
     *    Pu, this function calculates the distance to a Point Pd (distorted),
     *    which describes a (real) projection with radial lensdistortion (set by
     *    parameters K3 & K5). The function performs an iterative search ...
     * PROVIDED BY BBC UNDER IVIEW PROJECT
     */
    static const int max_distiter = 50;
    static double undist2dist(double ru, double k3, double k5)
    {
      int i = max_distiter;
      double rd;

      // new method: expand undistorted radius function (r * scale) as a 1st order Taylor series:
      // undistorted radius = U(distorted radius)
      // U(r) = r * scale = r + K3 * r^3 + K5 * r^5
      //  giving
      // U(r + d) = U(r) + d * dU/dr (+ higher order terms)
      // then write ru = U(r + d) where r is a first guess, and solve for d
      // d  =  ru - U(r) / (dU/dr)
      // and then update estimate r to r+d and iterate until d gets very small

      double delta;
      rd = ru;// initial estimate for distorted radius = undistorted radius
      do {
        delta = (ru - rd - k3 * rd * rd * rd - k5 * rd * rd * rd * rd * rd) / (1 + 3.0 * k3 * rd * rd + 5.0 * k5 * rd * rd * rd * rd);
        rd = rd + delta;
      } while((fabs(delta) > 1e-8) && i--);

      if(i < 0) {
        rd = -1.0;// if iteration fails, return a negative radius
      }
      return rd;
    }

  protected:
    RealT m_sizex = 1;// x pixel size
    RealT m_sizey = 1;// y pixel size
    RealT m_k3 = 0;   // 3rd order radial distortion
    RealT m_k5 = 0;   // 5th order radial distortion
  };

  extern template class PinholeCamera2<float>;

};// namespace Ravl2
