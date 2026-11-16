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

  enum class CameraCoordinateSystemT : uint8_t
  {
    Native, //!< As OpenCV but image coordinates are swapped
    OpenCV,
    OpenGL
  };

  //! CRTP base class for pinhole camera models
  template<class ImplT, typename RealT = typename ImplT::ValueT>
  class PinholeCameraBase
  {
  public:
    PinholeCameraBase() = default;




  protected:
  };



  //! @brief Simple pinhole camera model with no lens distortion
  //!  Projects 2D image points z from 3D points x according to:<br>
  //!    z[0] = cx + fx*( (R*x + t)[0] / (R*x + t)[2] )<br>
  //!    z[1] = cy + fy*( (R*x + t)[1] / (R*x + t)[2] )<br>
  //!
  //! @section camera_conventions Conventions
  //!
  //! **Image points are always Ravl2 (row, col)** — row down, col right, the
  //! same as array indexing. project(), projectCheck(), unprojectZ(),
  //! projectInverseDirection(), ray() and isInView() all take or return
  //! (row, col), whichever way the camera was constructed. Convert to OpenCV's
  //! (x, y) only at the OpenCV boundary (toCvImagePoint(), intrinsicMatrix<OpenCV>()).
  //!
  //! **The 3D camera frame is OpenCV's** (x right, y down, z forward) for every
  //! camera built by fromParameters(), the frame constructor, fromFrame(),
  //! fromFrameOrigin() or a Configuration with the default coordinateSystem.
  //! So 3D points agree with depth sensors, BOP poses and cv::solvePnP.
  //!
  //! @section internal_coords Internal storage
  //!
  //! The stored m_R, m_t, m_fx, m_fy, m_cx, m_cy are **internal**: m_cx is the
  //! principal point's ROW, m_fx the focal length along rows, and m_R / m_t
  //! map world points to the internal (row, col, depth) frame. A camera built
  //! from external (OpenCV) parameters stores `m_R = S * R`, `m_t = S * t`
  //! (S = [[0,1,0],[1,0,0],[0,0,1]], its own inverse) with fx↔fy, cx↔cy
  //! swapped, and records it in hasCoordSwap(). So **R() and t() are not the
  //! world-to-camera extrinsic**; use extrinsicRotation() / extrinsicTranslation()
  //! (or extrinsicMatrix<OpenCV>()) for that.
  //!
  //! The raw data constructors take these internal values directly. With
  //! R = I they give a camera whose 3D frame is (row, col, depth) — a mirror
  //! image of OpenCV's. They exist for internal use and for code that genuinely
  //! wants that frame; to build a camera from OpenCV / ROS / RealSense
  //! intrinsics use fromParameters<OpenCV>().

  template <typename RealT>
  class PinholeCamera0
    : public PinholeCameraBase<PinholeCamera0<RealT>, RealT>
  {
  public:
    using ValueT = RealT;
    
    //! Default constructor
    PinholeCamera0() = default;

    //! @brief Data constructor from INTERNAL parameters.
    //! @param cx Principal point ROW (not OpenCV's cx).
    //! @param cy Principal point COLUMN.
    //! @param fx Focal length along rows, pixels.
    //! @param fy Focal length along columns, pixels.
    //! @param R, t World to internal (row, col, depth) frame.
    //! @details To build from OpenCV-convention intrinsics use fromParameters<OpenCV>();
    //! see the class documentation.
    PinholeCamera0(const RealT &cx, const RealT &cy,
                   const RealT &fx, const RealT &fy,
                   const Matrix<RealT, 3, 3> &R,
                   const Vector<RealT, 3> &t,
                   const IndexRange<2> &frame)
        : m_cx(cx), m_cy(cy), m_fx(fx), m_fy(fy), m_R(R), m_t(t), m_frame(frame)
    {
    }
    
    //! @brief Data constructor from INTERNAL parameters.
    //! @param centre Principal point, (row, col).
    //! @param pose World to internal (row, col, depth) frame.
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
    //! @param pose - world to camera, in the OpenCV camera frame (x right, y down, z forward)
    //! The camera is placed at the centre of the frame with the given focal length.
    explicit PinholeCamera0(const IndexRange<2> &frame, float f, const Isometry3<RealT> &pose = Isometry3<RealT>())
      : m_cx(RealT(frame.range(0).min())+RealT(frame.range(0).size()-1)/RealT(2.0)),
        m_cy(RealT(frame.range(1).min())+RealT(frame.range(1).size()-1)/RealT(2.0)),
        m_fx(f),
        m_fy(f),
        m_R(swapMatrix() * pose.rotation().toMatrix()),
        m_t(swapMatrix() * pose.translation()),
        m_frame(frame),
        m_hasCoordSwap(true)
    {}

    //! The (row, col) <-> (x, y) swap matrix S; S is its own inverse.
    [[nodiscard]] static Matrix<RealT, 3, 3> swapMatrix()
    {
      return Matrix<RealT, 3, 3>{{0, 1, 0}, {1, 0, 0}, {0, 0, 1}};
    }

    //! True when the camera was built from external-convention parameters, so
    //! R() / t() include the swap S (see the class documentation).
    [[nodiscard]] bool hasCoordSwap() const
    {
      return m_hasCoordSwap;
    }


  protected:
  public:

    //! Construct from external-convention parameters.
    //! @param cx, cy  Principal point. OpenCV: (x = col, y = row). Native: (row, col).
    //! @param fx, fy  Focal lengths, in the same order as cx, cy.
    //! @param R  Extrinsic rotation (world → camera), OpenCV camera frame (x right, y down, z forward)
    //! @param t  Extrinsic translation (world → camera), the same frame
    //! @param frame  Image frame
    //!
    //! Native differs from OpenCV only in the order of the 2D parameters; the 3D
    //! frame is OpenCV's for both. The stored m_R, m_t are `S * R`, `S * t`, so R()
    //! and t() will NOT return the R and t passed here; see the class documentation.
    template <CameraCoordinateSystemT CoordSys>
    static PinholeCamera0 fromParameters(RealT cx, RealT cy,
                                         RealT fx, RealT fy,
                                          const Matrix<RealT, 3, 3> &R,
                                          const Vector<RealT, 3> &t,
                                          const IndexRange<2> &frame)
    {
#if 0
      if constexpr(CoordSys == CameraCoordinateSystemT::OpenGL) {
        Matrix<RealT, 3, 3> rot = R * Matrix<RealT, 3, 3>::fromDiagonal({1, -1, 1});
        return PinholeCamera0(cx, RealT(frame.range(1).size()) - cy, fx, fy, rot, t, frame);
      } else
#endif
      const Matrix<RealT, 3, 3> S = swapMatrix();
      if constexpr(CoordSys == CameraCoordinateSystemT::OpenCV) {
        return PinholeCamera0(cy, cx, fy, fx, S * R, S * t, frame, true);
      } else if constexpr(CoordSys == CameraCoordinateSystemT::Native) {
        return PinholeCamera0(cx, cy, fx, fy, S * R, S * t, frame, true);
      } else {
        static_assert(false, "Unsupported coordinate system");
      }
      return PinholeCamera0(cy, cx, fy, fx, R, t, frame);
    }

    template <CameraCoordinateSystemT CoordSys>
    static PinholeCamera0 fromParameters(RealT f,
                                         const Point<RealT,2> &centre,
                                         Isometry3<RealT> pose,
                                         const IndexRange<2> &frame)
    {
      return fromParameters<CoordSys>(centre[0], centre[1], f, f, pose.rotation().toMatrix(), pose.translation(), frame);
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
      Range<float,2> range = toRange<float>(frame);
      RealT f = range.size(1) * distance  / (horizontalSize);
      Point<float,2> centre = range.center();
      return fromParameters<CameraCoordinateSystemT::Native>(f,{centre[0],centre[1]},Isometry3<float>(Quaternion<float>::identity(),{0,0,0}),frame);
      //return PinholeCamera0(frame,f,Isometry3<float>(Quaternion<float>::identity(),{0,0,distance}));
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
      float f = float(frame.range(1).size()-1) * distance / (horizontalSize);
      return fromParameters<CameraCoordinateSystemT::Native>(f,{0,0},Isometry3<float>(Quaternion<float>::identity(),{0,0,0}),frame);
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

    //! Internal rotation matrix used in the projection formula.
    //! **Note:** This is S * R_extrinsic where S is the (row,col)↔(x,y) swap.
    //! See class-level documentation for details.
    [[nodiscard]] Matrix<RealT, 3, 3> &R()
    {
      return m_R;
    };

    //! Internal rotation matrix used in the projection formula.
    //! **Note:** This is S * R_extrinsic where S is the (row,col)↔(x,y) swap.
    //! See class-level documentation for details.
    [[nodiscard]] const Matrix<RealT, 3, 3> &R() const
    {
      return m_R;
    };

    //! Internal translation vector used in the projection formula (in internal coords).
    [[nodiscard]] Vector<RealT, 3> &t()
    {
      return m_t;
    };

    //! Internal translation vector used in the projection formula (in internal coords).
    [[nodiscard]] const Vector<RealT, 3> &t() const
    {
      return m_t;
    };

    //! Pure extrinsic rotation (world → camera) without coordinate-convention swap.
    //! Use this instead of R() when you need the actual camera orientation,
    //! e.g. for OpenCV's solvePnP extrinsic handling.
    [[nodiscard]] Matrix<RealT, 3, 3> extrinsicRotation() const
    {
      if(m_hasCoordSwap) {
        // m_R = S * R_extrinsic, so R_extrinsic = S * m_R (S is its own inverse)
        return Matrix<RealT, 3, 3>(swapMatrix() * m_R);
      }
      return m_R;
    }

    //! Pure extrinsic translation (world → camera) without coordinate-convention swap.
    [[nodiscard]] Vector<RealT, 3> extrinsicTranslation() const
    {
      if(m_hasCoordSwap) {
        return Vector<RealT, 3>(swapMatrix() * m_t);
      }
      return m_t;
    }

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
    //! from world to camera co-ordinates. For a camera with hasCoordSwap() the
    //! pose is in the OpenCV camera frame (as for fromParameters()); otherwise it
    //! is in the internal frame.
    void setPose(const Isometry3<RealT> &pose)
    {
      m_R = pose.rotation().toMatrix();
      m_t = pose.translation();
      if(m_hasCoordSwap) {
        m_R = swapMatrix() * m_R;
        m_t = swapMatrix() * m_t;
      }
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
      } else if constexpr( CoordSys == CameraCoordinateSystemT::OpenCV ) {
        return Eigen::Matrix3f({
           {m_fy, 0, m_cy},
           {0, m_fx, m_cx},
           {0, 0, 1}
          });
      } else if constexpr( CoordSys == CameraCoordinateSystemT::Native) {
        return Eigen::Matrix3f({{m_fx, 0, m_cx},
                                {0, m_fy, m_cy},
                                {0, 0, 1}});
      } else {
        static_assert(false, "Unhandled coordinate system");
      }
    }

    //! Return the extrinsic matrix
    template<CameraCoordinateSystemT CoordSys>
    [[nodiscard]] Eigen::Matrix<float, 3, 4> extrinsicMatrix() const
    {
      if constexpr (CoordSys == CameraCoordinateSystemT::OpenGL) {
        static_assert(false, "OpenGL untested");
        Eigen::Matrix<float, 3, 4> Rt;
        Rt.block<3, 3>(0, 0) = m_R;
        Rt.block<3, 1>(0, 3) = m_t;
        return Rt;
      } else if constexpr (CoordSys == CameraCoordinateSystemT::OpenCV) {
        // The world-to-camera extrinsic in the OpenCV frame; pairs with intrinsicMatrix<OpenCV>().
        // Only meaningful for a camera in the OpenCV frame (hasCoordSwap()).
        Eigen::Matrix<float, 3, 4> Rt;
        Rt.block<3, 3>(0, 0) = extrinsicRotation();
        Rt.block<3, 1>(0, 3) = extrinsicTranslation();
        return Rt;
      } else if constexpr (CoordSys == CameraCoordinateSystemT::Native) {
        // The internal matrix; pairs with intrinsicMatrix<Native>() for (row, col) points.
        Eigen::Matrix<float, 3, 4> Rt;
        Rt.block<3, 3>(0, 0) = m_R;
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

    //! Test if a point is in frame
    [[nodiscard]] bool isInView(const Vector<RealT, 3> &pnt) const
    {
      Point<float,2> pix;
      if(!projectCheck(pix,pnt)) {
        return false;
      }
      return m_frame.contains(toIndex(pix));
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
      archive(cereal::make_nvp("cx", m_cx));
      archive(cereal::make_nvp("cy", m_cy));
      archive(cereal::make_nvp("fx", m_fx));
      archive(cereal::make_nvp("fy", m_fy));
      archive(cereal::make_nvp("R", m_R));
      archive(cereal::make_nvp("t", m_t));
      archive(cereal::make_nvp("frame", m_frame));
      archive(cereal::make_nvp("nearPlane", mNearPlane));
      if(version >= 1) {
        archive(cereal::make_nvp("hasCoordSwap", m_hasCoordSwap));
      }
      // Before version 2, fromParameters() stored the external translation unswapped
      // (m_t = t instead of S * t). Correct it on load; m_R was already S * R.
      if constexpr(Archive::is_loading::value) {
        if(version < 2 && m_hasCoordSwap) {
          m_t = swapMatrix() * m_t;
        }
      }
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

  private:
    //! Internal constructor with explicit coordinate-swap flag.
    PinholeCamera0(const RealT &cx, const RealT &cy,
                   const RealT &fx, const RealT &fy,
                   const Matrix<RealT, 3, 3> &R,
                   const Vector<RealT, 3> &t,
                   const IndexRange<2> &frame,
                   bool hasCoordSwap)
        : m_cx(cx), m_cy(cy), m_fx(fx), m_fy(fy), m_R(R), m_t(t), m_frame(frame), m_hasCoordSwap(hasCoordSwap)
    {}

  protected:
    RealT m_cx = 0;
    RealT m_cy = 0;
    RealT m_fx = 1;
    RealT m_fy = 1;
    Matrix<RealT, 3, 3> m_R = Matrix<RealT, 3, 3>::Identity();
    Vector<RealT, 3> m_t = Vector<RealT,3>::Zero();
    IndexRange<2> m_frame;
    RealT mNearPlane = 1e-2f;
    bool m_hasCoordSwap = false;  //!< True when m_R includes the (row,col)↔(x,y) swap from fromParameters().
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

CEREAL_CLASS_VERSION(Ravl2::PinholeCamera0<float>, 2);


template <typename RealT>
struct fmt::formatter<Ravl2::PinholeCamera0<RealT>> : fmt::ostream_formatter {};
