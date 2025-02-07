// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Jean-Yves Guillemaut"
//! docentry="Ravl.API.3D.Camera Modelling"

#pragma once

#include "Ravl2/3D/PinholeCamera0.hh"

namespace Ravl2
{

  //: Pinhole camera model with 3rd and 5th order radial lens distortion
  // It uses the polynomial model to distort the image during projection and a numerical approximation to undistort the image during back-projection (similarly to the model used in OpenCV).  This is the reverse of <a href="Ravl3DN.PinholeCamera2C.html">PinholeCamera2C</a>.

  template <typename RealT>
  class PinholeCamera3 : public PinholeCamera0<RealT>
  {
  public:
    //: Default constructor
    PinholeCamera3() = default;

    //! Construct from an undistorted pinhole camera
    explicit PinholeCamera3(const PinholeCamera0<RealT> &c0, RealT k1 = 0, RealT k2 = 0)
      : PinholeCamera0<RealT>(c0),
        m_k1(k1),
        m_k2(k2)
    {}

    //! Construct from an image frame and focal length
    PinholeCamera3(const IndexRange<2> &frame, float f, RealT k1 = 0, RealT k2 = 0)
      : PinholeCamera0<RealT>(frame, f),
        m_k1(k1),
        m_k2(k2)
    {}

    //! Construct from a configuration
    explicit PinholeCamera3(Configuration &config)
      : PinholeCamera0<RealT>(config),
        m_k1(config.getNumber<RealT>("k1","Radial distortion coefficient 1",0.0,-1e4,1e4)),
        m_k2(config.getNumber<RealT>("k2","Radial distortion coefficient 2",0.0,-1e4,1e4))
    {}
    
    //! data constructor
    PinholeCamera3(const RealT &cx, const RealT &cy, const RealT &fx, const RealT &fy, const RealT &k1, const RealT &k2, const Matrix<RealT, 3, 3> &R, const Vector<RealT, 3> &t, const IndexRange<2> &frame)
        : PinholeCamera0<RealT>(cx, cy, fx, fy, R, t, frame),
          m_k1(k1),
          m_k2(k2)
    {}

    //! Construct camera from another coordinate system
    template <CameraCoordinateSystemT CoordSys>
    static PinholeCamera3<RealT> fromParameters(const RealT &cx, const RealT &cy,
                                                const RealT &fx, const RealT &fy,
                                                const Matrix<RealT, 3, 3> &R,
                                                const Vector<RealT, 3> &t,
                                                const IndexRange<2> &frame,
                                                const RealT &k1, const RealT &k2
                                                )
    {
      return PinholeCamera3<RealT>(PinholeCamera0<RealT>::template fromParameters<CoordSys>(cx, cy, fx, fy, R, t, frame), k1, k2);
    }

    //: First radial distortion coefficient
    [[nodiscard]] const RealT &k1() const
    {
      return this->m_k1;
    };

    //: Second radial distortion coefficient
    [[nodiscard]] const RealT &k2() const
    {
      return this->m_k2;
    };

    //! project 3D point in space to 2D image point
    //!  Can result in a divide-by-zero for degenerate points.
    //!  See projectCheck if this is to be avoided.
    void project(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx = this->m_R * x + this->m_t;
      Vector<RealT, 2> zd = distort0(toVector<RealT>(Rx[0] / Rx[2], Rx[1] / Rx[2]));
      z[0] = this->m_cx + this->m_fx * zd[0];
      z[1] = this->m_cy + this->m_fy * zd[1];
    }

    //! project a set of 3D point to a 2D image point
    void project(std::span<Vector<RealT, 2> > z,std::span<const Vector<RealT, 3> > x) const
    {
      assert(z.size() == x.size());
      if(z.size() != x.size()) {
        throw std::runtime_error("Size mismatch");
      }
      for(size_t i = 0; i < z.size(); i++)
      {
        project(z[i], x[i]);
      }
    }

    //! project 3D point in space to 2D image point
    //! The same as project(...) but checks that the point
    //! is not degenerate and in front of the camera.
    bool projectCheck(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      // Distortion-free projection
      Vector<RealT, 3> Rx = (this->m_R * x) + this->m_t;
      if(isNearZero(Rx[2],RealT(1e-4))) {
        return false;
      }
      Vector<RealT, 2> zd = distort0(toVector<RealT>(Rx[0] / Rx[2], Rx[1] / Rx[2]));
      z[0] = this->m_cx + this->m_fx * zd[0];
      z[1] = this->m_cy + this->m_fy * zd[1];
      return true;
    }

    //! Inverse projection up to a scale factor.
    //! origin + lambda*projectInverseDirection is the camera ray
    //! corresponding to image point z.
    void projectInverseDirection(Vector<RealT, 3> &x, const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 2> zd;
      zd[0] = (z[0] - this->m_cx) / this->m_fx;
      zd[1] = (z[1] - this->m_cy) / this->m_fy;
      Vector<RealT, 2> uz = undistort0(zd);
      Vector<RealT, 3> Rx;
      Rx[0] = uz[0];
      Rx[1] = uz[1];
      Rx[2] = 1.0;
      // TMul(this->m_R, Rx, x);
      x = this->m_R.transpose() * Rx;
    }

    //! Transform from a simple pinhole model point to a distorted image point
    [[nodiscard]] Vector<RealT, 2> distort(const Vector<RealT, 2> &z) const
    {
      RealT dx = (z[0] - this->m_cx) / this->m_fx;
      RealT dy = (z[1] - this->m_cy) / this->m_fy;
      Vector<RealT, 2> zd = distort0(toVector<RealT>(dx, dy));
      Vector<RealT, 2> ret;
      ret[0] = this->m_cx + this->m_fx * zd[0];
      ret[1] = this->m_cy + this->m_fy * zd[1];
      return ret;
    }

    //! Return an undistorted image point for a PinholeCamera0C model
    [[nodiscard]] Vector<RealT, 2> undistort(const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 2> zd;
      zd[0] = (z[0] - this->m_cx) / this->m_fx;
      zd[1] = (z[1] - this->m_cy) / this->m_fy;
      Vector<RealT, 2> uz = undistort0(zd);
      Vector<RealT, 2> ret;
      ret[0] = this->m_cx + this->m_fx * uz[0];
      ret[1] = this->m_cy + this->m_fy * uz[1];
      return ret;
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
      archive(cereal::base_class<PinholeCamera0<RealT>>(this),
              cereal::make_nvp("k1", this->m_k1),
              cereal::make_nvp("k2", this->m_k2));
    }

  protected:
    //! Apply radial distortion
    [[nodiscard]] Vector<RealT, 2> distort0(const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 2> ret = z;
      if(!isNearZero(this->m_k1) || !isNearZero(this->m_k2)) {
        const RealT &xu = z[0];
        const RealT &yu = z[1];
        RealT rd = xu * xu + yu * yu;
        RealT scale = 1 + this->m_k1 * rd + this->m_k2 * rd * rd;
        ret[0] = xu * scale;
        ret[1] = yu * scale;
      }
      return ret;
    }

    //! Remove radial distortion
    [[nodiscard]] Vector<RealT, 2> undistort0(const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 2> ret = z;
      // NOTE: do not undistort a point greater than one image width/height outside the image as this may not converge
      if((this->m_k1 != 0 || this->m_k2 != 0) && z[0] > (RealT(this->m_frame.min(1) - this->m_frame.range(1).size()) - this->m_cx) / this->m_fx && z[0] < (RealT(this->m_frame.max(1) + this->m_frame.range(1).size()) - this->m_cx) / this->m_fx && z[1] > (RealT(this->m_frame.min(0) - this->m_frame.range(0).size()) - this->m_cy) / this->m_fy && z[1] < (RealT(this->m_frame.max(0) + this->m_frame.range(0).size()) - this->m_cy) / this->m_fy) {
        const RealT &xd = z[0];
        const RealT &yd = z[1];
        RealT dl = std::sqrt(xd * xd + yd * yd);
        // Calculate distance from the centre to the distorted point
        dl = RealT(undist2dist(double(dl), double(this->m_k1), double(this->m_k2)));
        if(dl >= 0)// if distorting process converged......
        {
          RealT sqdl = dl * dl;
          RealT cudl = sqdl * sqdl;
          RealT scale = 1 + this->m_k1 * sqdl + this->m_k2 * cudl;
          ret[0] = xd / scale;
          ret[1] = yd / scale;
        }
      }
      return ret;
    }

    /*
     * Function undist2dist: 
     *    ... given the distance between optical center and ideal projected point
     *    Pu, this function calculates the distance to a Point Pd (distorted),
     *    which describes a (real) projection with radial lensdistortion (set by
     *    parameters k1 & k2). The function performs an iterative search ...
     * PROVIDED BY BBC UNDER IVIEW PROJECT
     */
    static const int max_distiter = 50;
    [[nodiscard]] static double undist2dist(double ru, double k1, double k2)
    {
      int i = max_distiter;
      double rd;

      // new method: expand undistorted radius function (r * scale) as a 1st order Taylor series:
      // undistorted radius = U(distorted radius)
      // U(r) = r * scale = r + k1 * r^3 + k2 * r^5
      //  giving
      // U(r + d) = U(r) + d * dU/dr (+ higher order terms)
      // then write ru = U(r + d) where r is a first guess, and solve for d
      // d  =  ru - U(r) / (dU/dr)
      // and then update estimate r to r+d and iterate until d gets very small

      double delta;
      rd = ru;// initial estimate for distorted radius = undistorted radius
      do {
        delta = (ru - rd - k1 * rd * rd * rd - k2 * rd * rd * rd * rd * rd) / (1 + 3.0 * k1 * rd * rd + 5.0 * k2 * rd * rd * rd * rd);
        rd = rd + delta;
      } while((fabs(delta) > 1e-8) && i--);

      if(i < 0) {
        rd = -1.0;// if iteration fails, return a negative radius
      }
      return rd;
    }

  private:
    RealT m_k1 = 0;//!< First radial distortion coefficient
    RealT m_k2 = 0;//!< Second radial distortion coefficient
  };

  template<class RealT>
  std::ostream &operator<<(std::ostream &s,const PinholeCamera3<RealT> &v) {
    s << static_cast<const PinholeCamera0<RealT>&>(v);
    s << " k:"  << v.k1() << ' ' << v.k2();
    return s;
  }

  extern template class PinholeCamera3<float>;

};// namespace Ravl2

template <typename RealT>
struct fmt::formatter<Ravl2::PinholeCamera3<RealT>> : fmt::ostream_formatter {};
