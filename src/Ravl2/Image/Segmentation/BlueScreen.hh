// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_BLUESCREEN_HEADER
#define RAVLIMAGE_BLUESCREEN_HEADER 1
////////////////////////////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! docentry="Ravl.API.Images.Segmentation"
//! author="Charles Galambos"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Segmentation/BlueScreen.hh"

#include "Ravl/Image/Image.hh"
#include "Ravl/Image/ByteRGBValue.hh"
#include "Ravl/Image/ByteRGBAValue.hh"
#include "Ravl/Image/ByteYUV422Value.hh"
#include "Ravl/Image/ByteYUVValue.hh"

namespace RavlImageN {
  
  //! userlevel=Normal
  //: Simple and fast blue screen mask generation.
  // Calculates "(2*Blue - (Red+Green)) < thresh" for each pixel
  // This is the same as U - 2.3*V in YUV space
  
  class BlueScreenC {
  public:
    BlueScreenC(int nthresh = 40)
      : thresh(nthresh)
    {}
    //: Constructor.
  
    void SetThreshold(int value)
    { thresh=value; }
    //: Get colour threshold
    // Values should be between 0 and 512
    
    int GetThreshold() const
    { return thresh; }
    //: Get threshold used

    void Apply(ImageC<ByteT>& mask, 
	       const ImageC<ByteRGBValueC> &image) const;
    //: Produce a background/foreground mask from an RGB image
    //  255 is assigned to foreground, 0 otherwise

    void Apply(ImageC<ByteT>& mask, 
	       const ImageC<ByteRGBAValueC> &image) const;
    //: Produce a background/foreground mask from an RGBA image
    //  255 is assigned to foreground, 0 otherwise

    ImageC<ByteT> Apply(const ImageC<ByteRGBValueC> &image) const
    {
      ImageC<ByteT> ret(image.Frame());
      Apply(ret, image);
      return ret;
    }
    //: Produce a background/foreground mask from an RGB image
    //  255 is assigned to foreground, 0 otherwise

    ImageC<ByteT> Apply(const ImageC<ByteRGBAValueC> &image) const
    {
      ImageC<ByteT> ret(image.Frame());
      Apply(ret, image);
      return ret;
    }
    //: Produce a background/foreground mask from an RGBA image
    //  255 is assigned to foreground, 0 otherwise
    
    void Apply(ImageC<ByteT>& mask,
	       const ImageC<ByteYUV422ValueC>& image) const;
    //: Produce a background/foreground mask from YUV 422 image
    //  255 is assigned to foreground, 0 otherwise

    ImageC<ByteT> Apply(const ImageC<ByteYUV422ValueC>& image) const
    {
      ImageC<ByteT> ret(image.Frame());
      Apply(ret, image);
      return ret;
    }
    //: Produce a background/foreground mask from YUV 422 image
    //  255 is assigned to foreground, 0 otherwise


    void Apply(ImageC<ByteT>& mask,
	       const ImageC<ByteYUVValueC>& image) const;
    //: Produce a background/foreground mask from YUV (444) image
    //  255 is assigned to foreground, 0 otherwise

    ImageC<ByteT> Apply(const ImageC<ByteYUVValueC>& image) const
    {
      ImageC<ByteT> ret(image.Frame());
      Apply(ret, image);
      return ret;
    }
    //: Produce a background/foreground mask from YUV (444) image
    //  255 is assigned to foreground, 0 otherwise

  protected:
    int thresh;
  };
  
}


#endif
