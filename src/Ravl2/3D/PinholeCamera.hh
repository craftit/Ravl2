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

  //: Transform from a camera point to a point in a simple pinhole model

  //: Base Pinhole Camera handle.
  // This class provides access to transform between 3D world and 2D image points.

  template<typename RealT>
  class PinholeCamera
  {
  public:
    //: Default constructor.
    PinholeCamera() = default;

    //: Destructor.
    virtual ~PinholeCamera() = default;

    //: Make a copy of body.
    // This should be provided in derived classes.
    virtual PinholeCamera& Copy() const = 0;

    virtual void Project(Vector<RealT,2>& z, const Vector<RealT,3>& x) const = 0;
    //: Project a 3D point to a 2D image point
    // This should be provided in derived classes.

    virtual bool ProjectCheck(Vector<RealT,2>& z, const Vector<RealT,3>& x) const = 0;
    //: Project 3D point in space to 2D image point checking for degeneracy
    // This should be provided in derived classes.

    virtual void ProjectInverseDirection(Vector<RealT,3>& x, const Vector<RealT,2>& z) const =0;
    //:Inverse projection up to a scale factor
    // Origin + lambda*x is the camera ray for image point z.
    // This should be provided in derived classes.
    
    virtual void ProjectJacobian(Matrix<RealT,2,3>& Jz, const Vector<RealT,3>& x) const = 0;
    //:The Jacobian matrix of the projection funtion
    // This should be provided in derived classes.

    virtual void Origin(Vector<RealT,3>& org) const = 0;
    //: Origin of the camera in world co-ordinates
    // This should be provided in derived classes.

    virtual void Direction(Vector<RealT,3>& dit) const = 0;
    //: Look direction for the camera in the world co-ordinate frame
    // This should be provided in derived classes.

    virtual const IndexRange<2>& Frame() const = 0;
    //: Image frame for the camera
    // This should be provided in derived classes.

    virtual void SetFrame(const IndexRange<2>& frame) = 0;
    //: Set the image frame for the camera
    // This should be provided in derived classes.

    virtual Vector<RealT,2> Undistort(const Vector<RealT,2>& z) const = 0;
    //: Transform from a camera point to a point in a simple pinhole model
    // This should be provided in derived classes.

    virtual Vector<RealT,2> Distort(const Vector<RealT,2>& z) const = 0;
    //: Transform from a simple pinhole model point to a distorted image point
    // This should be provided in derived classes.


#if 0
    template<class PixelT> 
    Array<PixelT,2> UndistortImage(const Array<PixelT,2>& img, bool resize = false) const;
    //: Return an undistorted image for a simple pinhole model

    template<class PixelT> 
    Array<PixelT,2> UndistortImage(const Array<PixelT,2>& img, const PixelT bg, bool resize = false) const;
    //: Return an undistorted image for a simple pinhole model filling empty pixels with bg value

    template<class PixelT> 
    Array<PixelT,2> DistortImage(const Array<PixelT,2>& img, bool resize = false) const;
    //: Return a distorted image for a simple pinhole model

    template<class PixelT> 
    Array<PixelT,2> DistortImage(const Array<PixelT,2>& img, const PixelT bg, bool resize = false) const;
    //: Return a distorted image for a simple pinhole model filling empty pixels with bg value
#endif
  };

#if 0
  template<class PixelT> Array<PixelT,2>
  PinholeCamera::UndistortImage(const Array<PixelT,2>& img, bool resize) const
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
};

