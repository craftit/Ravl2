// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_CHROMA_THRESHOLD_HH_
#define RAVLIMAGE_CHROMA_THRESHOLD_HH_
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Joel Mitchelson"
//! docentry="Ravl.API.Images.Segmentation"
//! file="Ravl/Image/Processing/Segmentation/ChromaThreshold.hh"

#include "Ravl2/Array.hh"
#include "Ravl2/Image/Pixel/ByteRGBValue.hh"
#include "Ravl2/Image/Pixel/ByteRGBAValue.hh"

namespace RavlImageN
{

  //! userlevel=Normal
  //: Chroma thresholding
  //
  // <p>This is a multi-purpose class for searching for a known colour in an RGB image.
  // Can be used for skin colour detection, blue-screening, etc. Is intended as
  // an easy way to try out new ideas, rather than a very efficient or accurate method.</p>
  //
  // <p>RGB is normalised by intensity to be robust to illumination changes.
  // The range of colours to be found are modelled as a Gaussian.  The (incorrect)
  // assumption is made that normalised RGB components are independent, but this
  // seems to work well for many applications. The Gaussian can either be
  // constructed directly, or computed from an example image containing only
  // the required colour.

  class ChromaThresholdRGBC
  {
    friend std::ostream &operator<<(std::ostream &os, const ChromaThresholdRGBC &c);
    friend std::istream &operator>>(std::istream &is, ChromaThresholdRGBC &c);

  public:
    using RealT = float;
    using ByteT = uint8_t;

    ChromaThresholdRGBC()
    {}
    //: default constructor

    ChromaThresholdRGBC(RealT nr0, RealT ng0, RealT nb0,
                        RealT nrw = 33.33, RealT ngw = 33.33, RealT nbw = 33.33,
                        RealT nblack_thresh = 0.1,
                        ByteT nlabel_match = 255,
                        ByteT nlabel_no_match = 0,
                        ByteT nlabel_black = 0) : r0(nr0), g0(ng0), b0(nb0),
                                                  rw(nrw), gw(ngw), bw(nbw),
                                                  black_thresh(nblack_thresh),
                                                  label_match(nlabel_match),
                                                  label_no_match(nlabel_no_match),
                                                  label_black(nlabel_black)
    {}
    //: Construct with user-specified params of the Gaussian
    //!param: nr0 - mean red component
    //!param: ng0 - mean green component
    //!param: nb0 - mean blue component
    //!param: nrw - inverse variance of red component
    //!param: ngw - inverse variance of green component
    //!param: nbw - inverse variance of blue component
    //!param: nblack_thresh - luminance in range (0,1] below which a pixel is considered black rather than coloured.
    //!param: nlabel_match - label to give to pixels which match the specified colour
    //!param: nlabel_no_match - label to give to pixels which do not match the specified colour
    //!param: nlabel_black - label to give to black pixels (ones below nblack_thresh)
    // Should have nr0 + ng0 + nb0 = 1 for a valid model.

    ChromaThresholdRGBC(const ImageC<ByteRGBValueC> &image,
                        RealT tolerance = 1.0,
                        RealT black_thresh = 0.01,
                        ByteT nlabel_match = 255,
                        ByteT nlabel_no_match = 0,
                        ByteT nlabel_black = 0);
    //: Construct from example image
    //!param: image - example image containing only the required colour
    //!param: nblack_thresh - luminance in range (0,1] below which a pixel is considered black rather than coloured.
    //!param: nlabel_match - label to give to pixels which match the specified colour
    //!param: nlabel_no_match - label to give to pixels which do not match the specified colour
    //!param: nlabel_black - label to give to black pixels (ones below nblack_thresh)

  public:
    void Apply(ImageC<ByteT> &result, const ImageC<ByteRGBValueC> &image) const;
    //: perform threshold on RGB image and return binary result

    void Apply(ImageC<ByteT> &result, const ImageC<ByteRGBAValueC> &image) const;
    //: perform threshold on RGB image and return binary result

    void ApplyCopyAlpha(ImageC<ByteT> &result,
                        const ImageC<ByteRGBAValueC> &image,
                        ImageC<ByteRGBAValueC> &auximage) const;
    //: perform threshold on RGBA image, return binary result
    //: and copy results to alpha channel of aux image

    void ApplyCopyAlpha(ImageC<ByteT> &result,
                        ImageC<ByteRGBAValueC> &image)
    {

      ApplyCopyAlpha(result, image, image);
    }
    //: perform threshold on RGBA image, return binary result
    //: and copy results to alpha channel of original image

  protected:
    RealT r0;
    RealT g0;
    RealT b0;

    RealT rw;
    RealT gw;
    RealT bw;

    RealT black_thresh;

    ByteT label_match;
    ByteT label_no_match;
    ByteT label_black;
  };

  std::ostream &operator<<(std::ostream &os, const ChromaThresholdRGBC &c);
  std::istream &operator>>(std::istream &is, ChromaThresholdRGBC &c);
}// namespace RavlImageN

#endif
