// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001-12, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.3D.Camera Modelling"

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! @brief Transform from a camera point to a point in a simple pinhole model
  //! Base Pinhole Camera handle.
  //! This class provides access to transform between 3D world and 2D image points.

  template <typename RealT = float>
  class PinholeCamera
  {
  public:
    //: Default constructor.
    PinholeCamera() = default;

    //: Destructor.
    virtual ~PinholeCamera() = default;

    //: Make a copy of body.
    virtual std::shared_ptr<PinholeCamera<RealT> > copy() const = 0;
    
    //: project a 3D point to a 2D image point
    virtual void project(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const = 0;

    //: project a set of 3D point to a 2D image point
    virtual void project(std::span<Vector<RealT, 2> > z,std::span<const Vector<RealT, 3> > x) const = 0;

    //: project 3D point in space to 2D image point checking for degeneracy
    virtual bool projectCheck(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const = 0;
    
    //:Inverse projection up to a scale factor
    // origin + lambda*x is the camera ray for image point z.
    virtual void inverseProjectDirection(Vector<RealT, 3> &x, const Vector<RealT, 2> &z) const = 0;
    
    //:The Jacobian matrix of the projection funtion
    virtual void projectJacobian(Matrix<RealT, 2, 3> &Jz, const Vector<RealT, 3> &x) const = 0;
    
    //: origin of the camera in world co-ordinates
    [[nodiscard]] virtual Vector<RealT, 3> origin() const = 0;
    
    //: Look direction for the camera in the world co-ordinate frame
    [[nodiscard]] virtual Vector<RealT, 3> direction() const = 0;
    
    //: Image frame for the camera
    [[nodiscard]] virtual const IndexRange<2> &range() const = 0;
    
    //: Set the image frame for the camera
    virtual void setRange(const IndexRange<2> &frame) = 0;
    
    //: Transform from a camera point to a point in a simple pinhole model
    // This should be provided in derived classes.
    [[nodiscard]] virtual Vector<RealT, 2> undistort(const Vector<RealT, 2> &z) const = 0;
    
    //: Transform from a simple pinhole model point to a distorted image point
    // This should be provided in derived classes.
    [[nodiscard]] virtual Vector<RealT, 2> distort(const Vector<RealT, 2> &z) const = 0;
    
    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
      archive();
    }

#if 0
    //: Return an undistorted image for a simple pinhole model
    template<class PixelT>
    Array<PixelT,2> undistortImage(const Array<PixelT,2>& img, bool resize = false) const;

    //: Return an undistorted image for a simple pinhole model filling empty pixels with bg value
    template<class PixelT>
    Array<PixelT,2> UndistortImage(const Array<PixelT,2>& img, const PixelT bg, bool resize = false) const;

    //: Return a distorted image for a simple pinhole model
    template<class PixelT>
    Array<PixelT,2> DistortImage(const Array<PixelT,2>& img, bool resize = false) const;

    //: Return a distorted image for a simple pinhole model filling empty pixels with bg value
    template<class PixelT>
    Array<PixelT,2> DistortImage(const Array<PixelT,2>& img, const PixelT bg, bool resize = false) const;
#endif
  };
  
  
  template <typename CameraT,typename RealT = typename CameraT::ValueT>
  class PinholeCameraImpl
    : public PinholeCamera<RealT>
  {
  public:
    PinholeCameraImpl() = default;

    //! Constructor.
    PinholeCameraImpl(const CameraT &camera)
      : m_camera(camera)
    {}
    
    //! Destructor.
    ~PinholeCameraImpl() override = default;
    
    //! Make a copy of body.
    std::shared_ptr<PinholeCamera<RealT> > copy() const final
    {
      return std::make_shared<PinholeCameraImpl<CameraT,RealT>>(m_camera);
    }
    
    //! project a 3D point to a 2D image point
    void project(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const final
    {
      m_camera.project(z, x);
    }

    //: project a set of 3D point to a 2D image point
    void project(std::span<Vector<RealT, 2> > z,std::span<const Vector<RealT, 3> > x) const final
    {
      assert(z.size() == x.size());
      if(z.size() != x.size()) {
        throw std::runtime_error("Size mismatch");
      }
      for(size_t i = 0; i < z.size(); i++)
      {
        m_camera.project(z[i], x[i]);
      }
    }


    //! project 3D point in space to 2D image point checking for degeneracy
    bool projectCheck(Vector<RealT, 2> &z, const Vector<RealT, 3> &x) const final
    {
      return m_camera.projectCheck(z, x);
    }
    
    //! Inverse projection up to a scale factor
    void inverseProjectDirection(Vector<RealT, 3> &x, const Vector<RealT, 2> &z) const final
    {
      m_camera.projectInverseDirection(x, z);
    }
    
    //! The Jacobian matrix of the projection function
    void projectJacobian(Matrix<RealT, 2, 3> &Jz, const Vector<RealT, 3> &x) const final
    {
      m_camera.projectJacobian(Jz, x);
    }
    
    //! origin of the camera in world co-ordinates
    [[nodiscard]] Vector<RealT, 3> origin() const final
    {
      return m_camera.origin();
    }
    
    //! Look direction for the camera in the world co-ordinate frame
    [[nodiscard]] Vector<RealT, 3> direction() const final
    {
      return m_camera.direction();
    }
    
    //! Image frame for the camera
    [[nodiscard]] const IndexRange<2> &range() const final
    {
      return m_camera.range();
    }
    
    //! Set the image frame for the camera
    void setRange(const IndexRange<2> &frame) final
    {
      m_camera.setRange(frame);
    }
    
    //! Transform from a camera point to a point in a simple pinhole model
    [[nodiscard]] Vector<RealT, 2> undistort(const Vector<RealT, 2> &z) const final
    {
      return m_camera.undistort(z);
    }
    
    //! Transform from a simple pinhole model point to a distorted image point
    [[nodiscard]] Vector<RealT, 2> distort(const Vector<RealT, 2> &z) const final
    {
      return m_camera.distort(z);
    }
    
    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
       archive(cereal::base_class<PinholeCamera<RealT>>(this),
               cereal::make_nvp("camera", m_camera));
    }
  
  private:
    CameraT m_camera;
  };

#if 0
  
  template <typename RealT>
  template<class PixelT>
  Array<PixelT,2> PinholeCamera<RealT>::undistortImage(const Array<PixelT,2>& img, bool resize) const
  {
     IndexRange<2> rect;
     RealT minr = img.range().min(0); RealT maxr = img.range().max(0);
     RealT minc = img.range().min(1); RealT maxc = img.range().max(1);

     if(resize) {
       Array2dIterC<PixelT> it0(img);
       IndexRange2dIterC itrect0(img.range());
       for (; it0; it0++, itrect0++)
       {
         Index<2> pix = itrect0.Data();
         if(pix[0]==minr || pix[0]==maxr || pix[1]==minc || pix[1]==maxc) {
           Vector<RealT,2> z = this->undistort(Vector<RealT,2>(pix[1], pix[0]));
           rect.involve(Index<2>(z[1],z[0]));
         }
       }
     } else {
        rect = img.range();
     }

     Array<PixelT,2> ret(rect);

     Array2dIterC<PixelT> it(ret);
     IndexRange2dIterC itrect(rect);
     for (; it; it++, itrect++)
     {
       Index<2> pix = itrect.Data();
       Vector<RealT,2> z = distort(Vector<RealT,2>(pix[1], pix[0]));
       if (z[1]>minr && z[1]<maxr && z[0]>minc && z[0]<maxc)
       {
         it.Data() = interpolateBilinear(img, Vector<RealT,2>(z[1],z[0]);
       }
     }

     return ret;
  }
  
  //: Derive undistort an image using the 1st order radial distortion model

  template<class PixelT> Array<PixelT,2>
  PinholeCamera::UndistortImage(const Array<PixelT,2>& img, const PixelT bg, bool resize) const
  {
     SampleBilinearC<PixelT, PixelT> sample;
     IndexRange<2> rect;
     RealT minr = img.range().min(0); RealT maxr = img.range().max(0);
     RealT minc = img.range().min(1); RealT maxc = img.range().max(1);

     if(resize) {
       Array2dIterC<PixelT> it0(img);
       IndexRange2dIterC itrect0(img.range());
       for (; it0; it0++, itrect0++)
       {
         Index<2> pix = itrect0.Data();
         if(pix[0]==minr || pix[0]==maxr || pix[1]==minc || pix[1]==maxc) {
           Vector<RealT,2> z = Undistort(Vector<RealT,2>(pix[1], pix[0]));
           rect.involve(Index<2>(z[1],z[0]));
         }
       }
     } else {
        rect = img.range();
     }

     Array<PixelT,2> ret(rect);

     Array2dIterC<PixelT> it(ret);
     IndexRange2dIterC itrect(rect);
     for (; it; it++, itrect++)
     {
       Index<2> pix = itrect.Data();
       Vector<RealT,2> z = Distort(Vector<RealT,2>(pix[1], pix[0]));
       if (z[1]>minr && z[1]<maxr && z[0]>minc && z[0]<maxc)
       {
         sample(img, Vector<RealT,2>(z[1],z[0]), it.Data());
       } else {
         it.Data() = bg;
       }
     }

     return ret;
  }
  //: Derive undistort an image using the 1st order radial distortion model filling empty pixels with bg value

  template<class PixelT> Array<PixelT,2>
  PinholeCamera::DistortImage(const Array<PixelT,2>& img, bool resize) const
  {
     SampleBilinearC<PixelT, PixelT> sample;
     IndexRange<2> rect;
     RealT minr = img.range().min(0); RealT maxr = img.range().max(0);
     RealT minc = img.range().min(1); RealT maxc = img.range().max(1);

     if(resize) {
       Array2dIterC<PixelT> it0(img);
       IndexRange2dIterC itrect0(img.range());
       for (; it0; it0++, itrect0++)
       {
         Index<2> pix = itrect0.Data();
         if(pix[0]==minr || pix[0]==maxr || pix[1]==minc || pix[1]==maxc) {
           Vector<RealT,2> z = Distort(Vector<RealT,2>(pix[1], pix[0]));
           rect.involve(Index<2>(z[1],z[0]));
         }
       }
     } else {
        rect = img.range();
     }

     Array<PixelT,2> ret(rect);

     Array2dIterC<PixelT> it(ret);
     IndexRange2dIterC itrect(rect);
     for (; it; it++, itrect++)
     {
       Index<2> pix = itrect.Data();
       Vector<RealT,2> z = Undistort(Vector<RealT,2>(pix[1], pix[0]));
       if (z[1]>minr && z[1]<maxr && z[0]>minc && z[0]<maxc)
       {
         sample(img, Vector<RealT,2>(z[1],z[0]), it.Data());
       }
     }

     return ret;
  }
  //: Derive distort an image using the 1st order radial distortion model

  template<class PixelT> Array<PixelT,2>
  PinholeCamera::DistortImage(const Array<PixelT,2>& img, const PixelT bg, bool resize) const
  {
     SampleBilinearC<PixelT, PixelT> sample;
     RealT minr = img.range().min(0); RealT maxr = img.range().max(0);
     RealT minc = img.range().min(1); RealT maxc = img.range().max(1);
     IndexRange<2> rect;

     if(resize) {
       Array2dIterC<PixelT> it0(img);
       IndexRange2dIterC itrect0(img.range());
       for (; it0; it0++, itrect0++)
       {
         Index<2> pix = itrect0.Data();
         if(pix[0]==minr || pix[0]==maxr || pix[1]==minc || pix[1]==maxc) {
           Vector<RealT,2> z = Distort(Vector<RealT,2>(pix[1], pix[0]));
           rect.involve(Index<2>(z[1],z[0]));
         }
       }
     } else {
        rect = img.range();
     }

     Array<PixelT,2> ret(rect);

     Array2dIterC<PixelT> it(ret);
     IndexRange2dIterC itrect(rect);
     for (; it; it++, itrect++)
     {
       Index<2> pix = itrect.Data();
       Vector<RealT,2> z = Undistort(Vector<RealT,2>(pix[1], pix[0]));
       if (z[1]>minr && z[1]<maxr && z[0]>minc && z[0]<maxc)
       {
         sample(img, Vector<RealT,2>(z[1],z[0]), it.Data());
       } else {
         it.Data() = bg;
       }
     }

     return ret;
  }
  //: Derive distort an image using the 1st order radial distortion model filling empty pixels with bg value
#endif
};// namespace Ravl2
