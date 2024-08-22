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

  template<typename RealT>
  class PinholeCamera3
    : public PinholeCamera0<RealT>
  {
  public: 
   //: default constructor
   PinholeCamera3() = default;

   //! construct from an undistorted pinhole camera
   PinholeCamera3(const PinholeCamera0<RealT>& c0)
    : PinholeCamera0<RealT>(c0)
   {}

   //! data constructor
   PinholeCamera3(const RealT& cx, const RealT& cy, const RealT& fx, const RealT& fy, const RealT& k1, const RealT& k2, const Matrix<RealT,3,3>& R, const Vector<RealT,3>& t, const IndexRange<2>& frame)
    : PinholeCamera0<RealT>(cx,cy,fx,fy,R,t,frame), m_k1(k1), m_k2(k2)
   {}

  public:
    const RealT& k1() const 
    {
      return m_k1; 
    };
    //: First radial distortion coefficient
    
    const RealT& k2() const 
    {
      return m_k2; 
    };
    //: Second radial distortion coefficient

  public:
  
    bool Load(std::istream &strm);
    //: Load from stream.
    

    bool Save(std::ostream &out) const;
    //: Writes object to stream
    
  public:
  
    void Project(Vector<RealT,2>& z, const Vector<RealT,3>& x) const
    {
       Vector<RealT,3> Rx = m_R*x + m_t;
       Vector<RealT,2> zd = Distort0(Vector<RealT,2>(Rx[0]/Rx[2], Rx[1]/Rx[2]));
       z[0] = m_cx + m_fx*zd[0];
       z[1] = m_cy + m_fy*zd[1];
    }
    //: Project 3D point in space to 2D image point
    //  Can result in a divide-by-zero for degenerate points.
    //  See ProjectCheck if this is to be avoided.

    bool ProjectCheck(Vector<RealT,2>& z, const Vector<RealT,3>& x) const
    {
       // Distortion-free projection
       Vector<RealT,3> Rx = (m_R * x) + m_t;
       if (Rx[2] > -1E-3 && Rx[2] < 1E-3)
          return false;
       Vector<RealT,2> zd = Distort0(Vector<RealT,2>(Rx[0]/Rx[2], Rx[1]/Rx[2]));
       z[0] = m_cx + m_fx*zd[0];
       z[1] = m_cy + m_fy*zd[1];
       return true;
    }
    //: Project 3D point in space to 2D image point
    // The same as Project(...) but checks that the point
    // is not degenerate.

    void ProjectInverseDirection(Vector<RealT,3>& x, const Vector<RealT,2>& z) const
    {
      Vector<RealT,2> zd;
      zd[0] = (z[0]-m_cx)/m_fx;
      zd[1] = (z[1]-m_cy)/m_fy;
      Vector<RealT,2> uz = Undistort0(zd);
      Vector<RealT,3> Rx;
      Rx[0] = uz[0];
      Rx[1] = uz[1];
      Rx[2] = 1.0;
      TMul(m_R,Rx,x);
    } 
    //:Inverse projection up to a scale factor
    // Origin + lambda*ProjectInverseDirection is the camera ray
    // corresponding to image point z.

    Vector<RealT,2> Distort(const Vector<RealT,2>& z) const
    {
       RealT dx = (z[0]-m_cx)/m_fx;
       RealT dy = (z[1]-m_cy)/m_fy;
       Vector<RealT,2> zd = Distort0(Vector<RealT,2>(dx, dy));
       Vector<RealT,2> ret;
       ret[0] = m_cx + m_fx*zd[0];
       ret[1] = m_cy + m_fy*zd[1];
       return ret;
    }
    //: Transform from a simple pinhole model point to a distorted image point

    Vector<RealT,2> Undistort(const Vector<RealT,2>& z) const
    {
       Vector<RealT,2> zd;
       zd[0] = (z[0]-m_cx)/m_fx;
       zd[1] = (z[1]-m_cy)/m_fy;
       Vector<RealT,2> uz = Undistort0(zd);
       Vector<RealT,2> ret;
       ret[0] = m_cx + m_fx*uz[0];
       ret[1] = m_cy + m_fy*uz[1];
       return ret;
    }
    //: Return an undistorted image point for a PinholeCamera0C model

  protected:

    Vector<RealT,2> Distort0(const Vector<RealT,2>& z) const
    {
       Vector<RealT,2> ret = z;
       if (m_k1 != 0.0 || m_k2 != 0.0)
       {
          const RealT& xu = z[0];
          const RealT& yu = z[1];
          RealT rd = xu*xu + yu*yu;
          RealT scale = 1 + m_k1*rd + m_k2*rd*rd;
          ret[0] = xu*scale;
          ret[1] = yu*scale;
       }
       return ret;
    }
    //: Apply radial distortion

    Vector<RealT,2> Undistort0(const Vector<RealT,2>& z) const
    {
       Vector<RealT,2> ret = z;
       // NOTE: do not undistort a point greater than one image width/height outside the image as this may not converge
       if ((m_k1 != 0.0 || m_k2 != 0.0) && 
           z[0]>(m_frame.min(1)-m_frame.range(1).size()-m_cx)/m_fx && z[0]<(m_frame.max(1)+m_frame.range(1).size()-m_cx)/m_fx &&
           z[1]>(m_frame.min(0)-m_frame.range(0).size()-m_cy)/m_fy && z[1]<(m_frame.max(0)+m_frame.range(0).size()-m_cy)/m_fy)
       {
          const RealT& xd =  z[0];
          const RealT& yd =  z[1];
          RealT dl = std::sqrt(xd*xd + yd*yd);
          // Calculate distance from the centre to the distorted point
          dl = undist2dist(dl, m_k1, m_k2);
          if (dl >= 0)   // if distorting process converged......
          {
             RealT sqdl = dl*dl;
             RealT cudl = sqdl * sqdl;
             RealT scale = 1 + m_k1*sqdl + m_k2 * cudl;
             ret[0] = xd/scale;
             ret[1] = yd/scale;
          }
       }
       return ret;
    }
    //: Remove radial distortion

     /*
     * Function undist2dist: 
     *    ... given the distance between optical center and ideal projected point
     *    Pu, this function calculates the distance to a Point Pd (distorted),
     *    which describes a (real) projection with radial lensdistortion (set by
     *    parameters k1 & k2). The function performs an iterative search ...
     * PROVIDED BY BBC UNDER IVIEW PROJECT
     */
     static const int max_distiter = 50;
     static double undist2dist(double ru, double k1, double k2)
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
        rd = ru;   // initial estimate for distorted radius = undistorted radius
        do {
           delta = (ru - rd - k1 * rd * rd * rd - k2 * rd * rd * rd * rd * rd) /
              (1 + 3.0 * k1 * rd * rd + 5.0 * k2 * rd * rd * rd * rd);
           rd = rd + delta;
        } while ((fabs(delta) > 1e-8) && i--);

        if (i<0)
        {
           rd = -1.0;    // if iteration fails, return a negative radius
        }
        return rd;
     }

  protected:
    RealT m_k1; // First radial distortion coefficient
    RealT m_k2; // Second radial distortion coefficient
  };
  

};

#endif
