// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Joel Mitchelson"
//! docentry="Ravl.API.3D.Camera Modelling"

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Configuration.hh"
#include "Ravl2/Geometry/Quaternion.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Isometry3.hh"
#include "Ravl2/Geometry/LinePV.hh"
#include "Ravl2/IndexRange.hh"
#include "Ravl2/3D/PinholeCamera.hh"
#include "Ravl2/Geometry/Plane3ABCD.hh"
#include "Ravl2/IO/Cereal.hh"

namespace Ravl2
{

  enum class CameraCoordinateSystemT
  {
    Native, //!< As OpenCV but image coordinates are swapped
    OpenCV,
    OpenGL
  };

  //! @brief Simple pinhole camera model with no lens distortion
  //!  Projects 2D image points z from 3D points x according to:<br>
  //!    z[0] = cx + fx*( (R*x + t)[0] / (R*x + t)[2] )<br>
  //!    z[1] = cy + fy*( (R*x + t)[1] / (R*x + t)[2] )<br>

  template <typename RealT>
  class PinholeCamera0
  {
  public:
    using ValueT = RealT;
    
    //! Default constructor
    PinholeCamera0() = default;

    //! Data constructor
    PinholeCamera0(const RealT &cx, const RealT &cy,
                   const RealT &fx, const RealT &fy,
                   const Matrix<RealT, 3, 3> &R,
                   const Vector<RealT, 3> &t,
                   const IndexRange<2> &frame)
        : m_cx(cx), m_cy(cy), m_fx(fx), m_fy(fy), m_R(R), m_t(t), m_frame(frame)
    {
    }
    
    //! Data constructor
    PinholeCamera0(RealT f,
                   const Point<RealT,2> &centre,
                   const IndexRange<2> &frame,
                   const Isometry3<RealT> &pose = Isometry3<RealT>()
                   )
      : m_cx(centre[0]),
        m_cy(centre[1]),
        m_fx(f),
        m_fy(f),
        m_R(pose.rotation().toMatrix()),
        m_t(pose.translation()),
        m_frame(frame)
    {}
    
    explicit PinholeCamera0(Configuration &config)
    {
      auto cx = config.getNumber<RealT>("cx","Center x",0.0,-1e4,1e4);
      auto cy = config.getNumber<RealT>("cy","Center y",0.0,-1e4,1e4);
      auto fx = config.getNumber<RealT>("fx","Focal length",1.0,0.0,1e4);
      auto fy = config.getNumber<RealT>("fy","Focal length",1.0,0.0,1e4);
      auto t = config.getPoint<RealT,3>("t","Translation",0.0,-1e5,1e5);
      Vector<float,3> angle = config.getPoint<RealT,3>("rotation","Rotation",0,-360,+360);
      for(unsigned i = 0; i < 3; i++)
        angle[i] = deg2rad(angle[i]);
      auto R = Quaternion<float>::fromEulerAnglesXYZ(angle).toMatrix();
      auto frame = config.template get<Ravl2::IndexRange<2>>("frame","Image frame",IndexRange<2>({{0,0},{0,0}}));

      auto coordSysStr = config.getString("coordinateSystem","Coordinate system","OpenCV");
      if(coordSysStr == "native" || coordSysStr == "Native") {
        *this = PinholeCamera0(cx, cy, fx, fy, R, t, frame);
      } else if(coordSysStr == "opencv" || coordSysStr == "OpenCV") {
        *this = PinholeCamera0<RealT>::fromParameters<CameraCoordinateSystemT::OpenCV>(cx, cy, fx, fy, R, t, frame);
      }
#if 0
      // Not supooorted yet
      else if(coordSysStr == "OpenGL" || coordSysStr == "opengl") {
        *this = PinholeCamera0<RealT>::fromParameters<CameraCoordinateSystemT::OpenGL>(cx, cy, fx, fy, R, t, frame);
      }
#endif
      else {
        SPDLOG_ERROR("Unknown coordinate system '{}'", coordSysStr);
        throw std::runtime_error("Unknown coordinate system");
      }
    }
    
    //! Construct a default camera with a given frame, with the axis at the centre of the frame
    //! @param frame - the image frame for the camera
    //! @param f - the focal length of the camera
    //! The camera is placed at the centre of the frame with the given focal length.
    explicit PinholeCamera0(const IndexRange<2> &frame, float f, const Isometry3<RealT> &pose = Isometry3<RealT>())
      : m_cx(RealT(frame.range(0).min())+RealT(frame.range(0).size()-1)/RealT(2.0)),
        m_cy(RealT(frame.range(1).min())+RealT(frame.range(1).size()-1)/RealT(2.0)),
        m_fx(f),
        m_fy(f),
        m_R(pose.rotation().toMatrix()),
        m_t(pose.translation()),
        m_frame(frame)
    {}

    //! Construct from another coordinate system
    template <CameraCoordinateSystemT CoordSys>
    static PinholeCamera0 fromParameters(RealT cx, RealT cy,
                                         RealT fx, RealT fy,
                                          const Matrix<RealT, 3, 3> &R,
                                          const Vector<RealT, 3> &t,
                                          const IndexRange<2> &frame)
    {
      if constexpr(CoordSys == CameraCoordinateSystemT::OpenGL) {
        static_assert(false, "OpenGL untested");
        Matrix<RealT, 3, 3> rot = R * Matrix<RealT, 3, 3>::fromDiagonal({1, -1, 1});
        return PinholeCamera0(cx, RealT(frame.range(1).size()) - cy, fx, fy, rot, t, frame);
      } else  if constexpr(CoordSys == CameraCoordinateSystemT::OpenCV) {
        Eigen::Matrix<RealT, 3, 3> swapXY({{0, 1, 0},{ 1, 0, 0}, {0, 0, 1}});
        auto rot = swapXY * R;
        return PinholeCamera0(cy, cx, fy, fx, rot, swapXY * t, frame);
      } else  if constexpr(CoordSys == CameraCoordinateSystemT::Native) {
        // Do nothing
      } else {
        static_assert(false, "Unknown coordinate system");
      }
    }


    //! Construct a camera that fills the image at the given distance
    //! with the camera at the origin looking along the +ve z-axis
    //! @param frame - the image frame for the camera
    //! @param horizontalSize - the size of the image in the x direction (frame.range(1))
    //! @param distance - the distance from the camera to the image plane
    static PinholeCamera0 fromFrame(const IndexRange<2> &frame,
                                    float horizontalSize,
                                    float distance)
    {
      float f = float(frame.range(1).size()-1) * distance * 2 / (horizontalSize);
      return PinholeCamera0(frame,f,Isometry3<float>(Quaternion<float>::identity(),{0,0,distance}));
    }
    
    //! Construct a camera that fills the image at the given distance
    //! with the camera at the origin looking along the +ve z-axis, at pixel 0,0
    //! @param frame - the image frame for the camera
    //! @param horizontalSize - the size of the image in the x direction  (frame.range(1))
    //! @param distance - the distance from the camera to the image plane
    static PinholeCamera0 fromFrameOrigin(const IndexRange<2> &frame,
                                    float horizontalSize,
                                    float distance)
    {
      float f = float(frame.range(1).size()-1) * distance * 2/ (horizontalSize);
      return PinholeCamera0(f,{0,0},frame,Isometry3<float>(Quaternion<float>::identity(),{0,0,distance}));
    }
    
    //! centre of projection, x co-ordinate
    [[nodiscard]] RealT &cx()
    {
      return m_cx;
    };

    //! centre of projection, x co-ordinate
    [[nodiscard]] const RealT &cx() const
    {
      return m_cx;
    };

    //! centre of projection, y co-ordinate
    [[nodiscard]] RealT &cy()
    {
      return m_cy;
    };

    //! centre of projection, y co-ordinate
    [[nodiscard]] const RealT &cy() const
    {
      return m_cy;
    };

    //! focal length in camera y direction (pixels)
    [[nodiscard]] RealT &fx()
    {
      return m_fx;
    };

    //! focal length in camera y direction (pixels)
    [[nodiscard]] const RealT &fx() const
    {
      return m_fx;
    };

    //! focal length in camera z direction (pixels)
    [[nodiscard]] RealT &fy()
    {
      return m_fy;
    };

    //! focal length in camera z direction (pixels)
    [[nodiscard]] const RealT &fy() const
    {
      return m_fy;
    };

    //! rotation world -> camera
    [[nodiscard]] Matrix<RealT, 3, 3> &R()
    {
      return m_R;
    };

    //! rotation world -> camera
    [[nodiscard]] const Matrix<RealT, 3, 3> &R() const
    {
      return m_R;
    };

    //! translation world -> camera (in camera co-ordinates)
    [[nodiscard]] Vector<RealT, 3> &t()
    {
      return m_t;
    };

    //! translation world -> camera (in camera co-ordinates)
    [[nodiscard]] const Vector<RealT, 3> &t() const
    {
      return m_t;
    };

    //! Image frame for the camera
    [[nodiscard]] const IndexRange<2> &range() const
    {
      return m_frame;
    }

    //! Set the image frame for the camera
    void setRange(const IndexRange<2> &frame)
    {
      m_frame = frame;
    }

    //! Set the camera pose, this is the transformation that takes a point
    //! from world to camera co-ordinates
    void setPose(const Isometry3<RealT> &pose)
    {
      m_R = pose.rotation().toMatrix();
      m_t = pose.translation();
    }

    //! project 3D point in space to 2D image point
    //!  Projects according to:<br>
    //!    z[0] = cx + fx*( (R*x + t)[0] / (R*x + t)[2] )<br>
    //!    z[1] = cy + fy*( (R*x + t)[1] / (R*x + t)[2] )<br>
    //!  Can result in a divide-by-zero for degenerate points.
    //!  See projectCheck if this is to be avoided.
    void project(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx = m_R * x + m_t;
      z[0] = m_cx + m_fx * Rx[0] / Rx[2];
      z[1] = m_cy + m_fy * Rx[1] / Rx[2];
    }

    //! project 3D point in space to 2D image point
    //!  Projects according to:<br>
    //!    z[0] = cx + fx*( (R*x + t)[0] / (R*x + t)[2] )<br>
    //!    z[1] = cy + fy*( (R*x + t)[1] / (R*x + t)[2] )<br>
    //!  Can result in a divide-by-zero for degenerate points.
    //!  See projectCheck if this is to be avoided.
    [[nodiscard]] Point<RealT, 2> project(const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx = m_R * x + m_t;
      return Point<RealT, 2>({m_cx + m_fx * Rx[0] / Rx[2], m_cy + m_fy * Rx[1] / Rx[2]});
    }

    //! Return the intrinsic matrix
    template<CameraCoordinateSystemT CoordSys>
    [[nodiscard]] Eigen::Matrix3f intrinsicMatrix() const
    {
      if constexpr (CoordSys == CameraCoordinateSystemT::OpenGL) {
        static_assert(false, "OpenGL untested");
        // Needs checking
        return Eigen::Matrix3f({
           {m_fx, 0, m_cx},
           {0, -m_fy, m_cy},
           {0, 0, -1}
          });
      } else if constexpr( CoordSys == CameraCoordinateSystemT::OpenCV) {
        return Eigen::Matrix3f({
           {m_fy, 0, m_cy},
           {0, m_fx, m_cx},
           {0, 0, 1}
          });
      } else if constexpr( CoordSys == CameraCoordinateSystemT::Native) {
        return Eigen::Matrix3f({
           {m_fx, 0, m_cx},
           {0, m_fy, m_cy},
           {0, 0, 1}
          });
      } else {
        static_assert(false, "Unknown coordinate system");
      }
    }

    //! Return the extrinsic matrix
    template<CameraCoordinateSystemT CoordSys>
    [[nodiscard]] Eigen::Matrix<float, 3, 4> extrinsicMatrix() const
    {
      if constexpr (CoordSys == CameraCoordinateSystemT::Native) {
        Eigen::Matrix<float, 3, 4> Rt;
        Rt.block<3, 3>(0, 0) = m_R;
        Rt.block<3, 1>(0, 3) = m_t;
        return Rt;
      } else if constexpr (CoordSys == CameraCoordinateSystemT::OpenGL) {
        static_assert(false, "OpenGL untested");
        Eigen::Matrix<float, 3, 4> Rt;
        Rt.block<3, 3>(0, 0) = m_R;
        Rt.block<3, 1>(0, 3) = m_t;
        return Rt;
      } else if constexpr (CoordSys == CameraCoordinateSystemT::OpenCV) {
        Eigen::Matrix<float, 3, 4> Rt;
        Eigen::Matrix<RealT, 3, 3> swapXY({{0, 1, 0},{ 1, 0, 0}, {0, 0, 1}});
        Rt.block<3, 3>(0, 0) = m_R * swapXY;
        Rt.block<3, 1>(0, 3) = m_t;
        return Rt;
      } else {
        static_assert(false, "Unknown coordinate system");
      }
      return Eigen::Matrix<float, 3, 4>::Identity();
    }

    //! Get the combined projection matrix
    template<CameraCoordinateSystemT CoordSys>
    [[nodiscard]] Matrix<RealT, 3, 4> projectionMatrix() const
    {
      return intrinsicMatrix<CoordSys>() * extrinsicMatrix<CoordSys>();
    }

    //! project 3D point in space to 2D image point
    //! The same as project(...) but checks that the point
    //! is not degenerate.
    bool projectCheck(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx =  m_R * x + m_t;
      if(Rx[2] < mNearPlane) {
        return false;
      }
      z[0] = m_cx + m_fx * Rx[0] / Rx[2];
      z[1] = m_cy + m_fy * Rx[1] / Rx[2];
      return true;
    }

    //! The Jacobian matrix of the projection function.
    void projectJacobian(Matrix<RealT, 2, 3> &Jz, const Vector<RealT, 3> &x) const
    {
      Vector<RealT, 3> Rx = (m_R * x) + m_t;
      RealT r_Rx2_2 = 1 / (Rx[2] * Rx[2]);
      Jz(0, 0) = m_fx * (m_R(0, 0) * Rx[2] - m_R(2, 0) * Rx[0]) * r_Rx2_2;
      Jz(0, 1) = m_fx * (m_R(0, 1) * Rx[2] - m_R(2, 1) * Rx[0]) * r_Rx2_2;
      Jz(0, 2) = m_fx * (m_R(0, 2) * Rx[2] - m_R(2, 2) * Rx[0]) * r_Rx2_2;
      Jz(1, 0) = m_fy * (m_R(1, 0) * Rx[2] - m_R(2, 0) * Rx[1]) * r_Rx2_2;
      Jz(1, 1) = m_fy * (m_R(1, 1) * Rx[2] - m_R(2, 1) * Rx[1]) * r_Rx2_2;
      Jz(1, 2) = m_fy * (m_R(1, 2) * Rx[2] - m_R(2, 2) * Rx[1]) * r_Rx2_2;
    }

    //! Inverse projection up to a scale factor.
    //! origin + lambda*projectInverseDirection is the camera ray
    //! corresponding to image point z.
    void projectInverseDirection(Vector<RealT, 3> &x, const Vector<RealT, 2> &z) const
    {
      Vector<RealT, 3> Rx;
      Rx[0] = (z[0] - m_cx) / m_fx;
      Rx[1] = (z[1] - m_cy) / m_fy;
      Rx[2] = 1.0;
      x = m_R.transpose() * Rx;
    }

    //! Inverse projection given a z coordinate in the camera frame
    [[nodiscard]] Point<RealT, 3> unprojectZ(const Vector<RealT, 2> &pix,RealT z) const
    {
      Vector<RealT, 3> Rx;
      Rx[0] = ((pix[0] - m_cx) * z)/ m_fx;
      Rx[1] = ((pix[1] - m_cy) * z)/ m_fy;
      Rx[2] = z;
      return m_R.transpose() * (Rx - m_t);
    }

    //! origin of the camera in world co-ordinates.
    //!  Computed as -R.T() * t.
    [[nodiscard]] Point<RealT, 3> origin() const
    {
      //TMul(m_R,m_t,org);
      return Point<RealT, 3>(-m_R.transpose() * m_t);
    }

    //! Look direction for the camera in the world co-ordinate frame
    //! Returns camera z-axis in world co-ordinate frame
    void direction(Vector<RealT, 3> &dir) const
    {
      dir[0] = m_R(2, 0);
      dir[1] = m_R(2, 1);
      dir[2] = m_R(2, 2);
    }

    //! Look direction for the camera in the world co-ordinate frame
    //! Returns camera z-axis in world co-ordinate frame
    [[nodiscard]] Vector<RealT, 3> direction() const
    {
      return toVector<RealT>(m_R(2, 0), m_R(2, 1), m_R(2, 2));
    }

    //! Return an undistorted image point for a simple pinhole model
    [[nodiscard]] Vector<RealT, 2> undistort(const Vector<RealT, 2> &z) const
    {
      return z;
    }

    //! Transform from a simple pinhole model point to a distorted image point
    [[nodiscard]] Vector<RealT, 2> distort(const Vector<RealT, 2> &z) const
    {
      return z;
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive, std::uint32_t const version)
    {
      (void) version;
      archive(cereal::make_nvp("cx", m_cx));
      archive(cereal::make_nvp("cy", m_cy));
      archive(cereal::make_nvp("fx", m_fx));
      archive(cereal::make_nvp("fy", m_fy));
      archive(cereal::make_nvp("R", m_R));
      archive(cereal::make_nvp("t", m_t));
      archive(cereal::make_nvp("frame", m_frame));
      archive(cereal::make_nvp("nearPlane", mNearPlane));
    }

    //! Access the camera frame
    [[nodiscard]] const IndexRange<2> &frame() const
    {
      return m_frame;
    }

    //! Access the near plane z distance for scene culling.
    [[nodiscard]] RealT nearPlane() const
    {
      return mNearPlane;
    }

  protected:
    RealT m_cx = 0;
    RealT m_cy = 0;
    RealT m_fx = 1;
    RealT m_fy = 1;
    Matrix<RealT, 3, 3> m_R = Matrix<RealT, 3, 3>::Identity();
    Vector<RealT, 3> m_t = Vector<RealT,3>::Zero();
    IndexRange<2> m_frame;
    RealT mNearPlane = 1e-2f;
  };

  //! Unproject a 2D image point to a 3D point in space
  //! The depth is the distance along the camera ray from the camera origin.
  //! @param camera - the camera model
  //! @param pix - the image point
  //! @param depth - the distance along the camera ray from the camera origin
  //! @return the 3D point in space
  template<typename RealT,typename CameraT>
  Point<RealT,3> unproject(const CameraT &camera, const Point<RealT,2> &pix, RealT depth)
  {
    Vector<RealT,3> dir;
    camera.projectInverseDirection(dir, pix);
    return camera.origin() + ((depth/ dir.norm()) *dir);
  }

  //! Unproject a 2D image point to a 3D point in space
  //! The z position is the distance along the camera z axis from the camera origin.
  //! @param camera - the camera model
  //! @param pix - the image point
  //! @param z - the distance along the camera ray from the camera origin
  //! @return the 3D point in space
  template<typename RealT,typename CameraT>
  Point<RealT,3> unprojectZ(const CameraT &camera, const Point<RealT,2> &pix, RealT z)
  { return camera.unprojectZ(pix,z); }

  //! Create a ray from a pixel in the world
  //! The ray is defined by the camera origin and the direction
  //! of the ray in world co-ordinates.
  template<typename RealT,typename CameraT>
  LinePV<RealT,3> ray(const CameraT &camera, const Point<RealT,2> &z)
  {
    Vector<RealT,3> dir;
    camera.projectInverseDirection(dir,z);
    return LinePV<RealT,3>(camera.origin(),dir);
  }


  template<typename RealT,typename CameraT>
  Point<RealT,2> project(const CameraT &camera, const Point<RealT,3> &pnt)
  {
    Point<RealT,2> z;
    camera.project(z,pnt);
    return z;
  }

  //! Project a ray from a pixel into the world
  //! The ray is defined by the camera origin and the direction
  //! of the ray in world co-ordinates.
  template<typename RealT,typename CameraT>
  LinePV<RealT,3> projectRay(const CameraT &camera, const Point<RealT,2> &z)
  {
    Vector<RealT,3> dir;
    camera.projectInverseDirection(dir,z);
    return LinePV<RealT,3>(camera.origin(),dir);
  }
  
  template<class RealT>
  std::ostream &operator<<(std::ostream &s,const PinholeCamera0<RealT> &v) {
    s << " c:" << v.cx() << ' ' << v.cy() << " f:" << v.fx() << ' ' << v.fy() << " r:" <<  Eigen::WithFormat(v.R(), Ravl2::defaultEigenFormat()) << " t:" << Eigen::WithFormat(v.t(), Ravl2::defaultEigenFormat()) << " r:" << v.range();
    return s;
  }

  extern template class PinholeCamera0<float>;
  extern template class PinholeCameraImpl<PinholeCamera0<float>>;

};// namespace Ravl2


template <typename RealT>
struct fmt::formatter<Ravl2::PinholeCamera0<RealT>> : fmt::ostream_formatter {};
