// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_WARPPROJECTIVE_HEADER
#define RAVLIMAGE_WARPPROJECTIVE_HEADER 1
//! docentry="Ravl.API.Images.Scaling and Warping"
//! author="Charles Galambos"
//! date="16/07/2002"

#include "Ravl2/Image/PixelMixer.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Image/BilinearInterpolation.hh"
#include "Ravl2/Matrix3d.hh"
#include "Ravl2/Vector3d.hh"
#include "Ravl2/Point2d.hh"
#include "Ravl2/RealRange2d.hh"
#include "Ravl2/LinePP2d.hh"
#include "Ravl2/Projection2d.hh"

namespace Ravl2 {
  
  //: Warp image with a projective transformation.
  // <p> By default, the coordinate system origin is taken to be the top left
  // corner of pixel[0][0].</p>
  //
  // <p> The output image frame is determined as follows:<ul>
  // <li> if the output image is gven, with a valid frame, use that frame
  // <li> else if the constructor specifies a valid frame, use that
  // <li> else find the bounding box of the input image in the output coords,
  // shrink it to the nearest pixel position, and use that.</ul>
  // </p>
  
  template <class InT, class OutT = InT,class MixerT = PixelMixerAssignC<InT,OutT>,class SampleT = SampleBilinearC<InT,OutT>  >
  class WarpProjectiveC
  {
  public:
    WarpProjectiveC()
    {}
    //: Default constructor.

    WarpProjectiveC(const IndexRange<2> &orec,
		    const Projection2dC &transform,
		    bool nFillBackground = true,
		    const MixerT &mix = MixerT())
      : trans(transform.Matrix()),
	iz(transform.IZ()),
	oz(transform.OZ()),
	rec(orec),
	fillBackground(nFillBackground),
	mixer(mix),
        pixelShift(0.5)
    { Init(); }
    //: Constructor from Projection2dC.
    //!param: orec - Rectangle for output image.
    //!param: transform - Projective transform to use
    //!param: nFillBackground - If true, background is filled with black.
    //!param: mix - Pixel mixer instance to use.
    // N.B. <code>transform</code> transforms points in the <i>input</i> image to the <i>output</i> image
    
    WarpProjectiveC(const Projection2dC &transform,
		    bool nFillBackground = true,
		    const MixerT &mix = MixerT())
      : trans(transform.Matrix()),
	iz(transform.IZ()),
	oz(transform.OZ()),
	fillBackground(nFillBackground),
	mixer(mix),
        pixelShift(0.5)
    { Init(); }
    //: Constructor from Projection2dC.
    
    WarpProjectiveC(const IndexRange<2> &orec,
		    const Matrix<RealT,3,3> &transform,
		    RealT niz = 1,
		    RealT noz = 1,
		    bool nFillBackground = true,
		    const MixerT &mix = MixerT())
      : trans(transform),
	iz(niz),
	oz(noz),
	rec(orec),
	fillBackground(nFillBackground),
	mixer(mix),
        pixelShift(0.5)
    { Init(); }
    //: Constructor from Matrix<RealT,3,3>.
    // Where orec is the size of the output rectangle.
    // See above for argument descriptions
    
    WarpProjectiveC(const Matrix<RealT,3,3> &transform,
		    RealT niz = 1,
		    RealT noz = 1,
		    bool nFillBackground = true,
		    const MixerT &mix = MixerT())
      : trans(transform),
	iz(niz),
	oz(noz),
	fillBackground(nFillBackground),
	mixer(mix),
        pixelShift(0.5)
    { Init(); }
    //: Constructor from Matrix<RealT,3,3>.
    // See above for argument descriptions
    
    void Apply(const Array<InT,2> &img,Array<OutT,2> &out);
    //: Warp image 'img' with the given transform and write it into 'out'
    // If the output image"<code>out</code>" has a valid frame, it will take precedence over the one specified in the constructor.
    
    Array<OutT,2> Apply(const Array<InT,2> &img) {
      Array<OutT,2> out(rec);
      Apply(img,out);
      return out;
    }
    //: Interpolate input image working rectangle into output image rectangle.
    // If the output rectangle is specified in the constructor, it is used.
    // Otherwise it is created from the bounding box of the projected input
    // image.
    
    void SetTransform(const Matrix<RealT,3,3> &transform) {
      trans = transform;
      Init();
    }
    //: Set projective transform.
    
    IndexRange<2> InputRectangle() const { 
      Range<2,float> orng(rec);
      IndexRange<2> ret(Project(orng.TopRight()),0);
      ret.involve(Project(orng.TopLeft()));
      ret.involve(Project(orng.BottomRight()));
      ret.involve(Project(orng.BottomLeft()));
      return ret;
    }
    //: Get range of input rectangle that will be used.
    // Note: This may be larger than the actual input provided.
    
    Point<RealT,2> BackProject(const Point<RealT,2> &pnt) const;
    //: Transform a point from the source to destination.

    Point<RealT,2> Project(const Point<RealT,2> &pnt) const;
    //: Transform a point from the destination to source.

    void SetMidPixelCorrection(bool correction) {
      pixelShift =  correction ?  0.5 : 0.0;
      Init();
    }
    //: Set mid pixel correction flag.
    //!param: correction = true - coordinate system is at top l.h corner of pixel (0,0) (the default)
    //!param: correction = false - coordinate system is at centre of pixel (0,0)
    
  protected:
    void Init();
    
    Matrix<RealT,3,3> trans;
    Matrix<RealT,3,3> inv;
    RealT iz, oz;
    IndexRange<2> rec;
    bool fillBackground;
    MixerT mixer;
    RealT pixelShift;
  };

  template <class InT, class OutT,class MixerT,class SampleT>
  void WarpProjectiveC<InT,OutT,MixerT,SampleT>::Init() {
    // If mid-pixel correction is needed, transform is replaced by:
    //   shift 1/2 pixel; transform; shift back again.
    Matrix<RealT,3,3> shiftIn (1,0,-pixelShift/iz, 0,1,-pixelShift/iz, 0,0,1);
    Matrix<RealT,3,3> shiftOut(1,0, pixelShift/oz, 0,1, pixelShift/oz, 0,0,1);
    inv = shiftIn*trans.Inverse()*shiftOut;
  }
  
  template <class InT, class OutT,class MixerT,class SampleT>
  Point<RealT,2> WarpProjectiveC<InT,OutT,MixerT,SampleT>::BackProject(const Point<RealT,2> &pnt) const {
    Vector<RealT,3> vi = trans * Vector<RealT,3>(pnt[0],pnt[1],iz);
    return Point<RealT,2>(oz*vi[0]/vi[2],oz*vi[1]/vi[2]);
  }
  
  template <class InT, class OutT,class MixerT,class SampleT>
  Point<RealT,2> WarpProjectiveC<InT,OutT,MixerT,SampleT>::Project(const Point<RealT,2> &pnt) const {
    Vector<RealT,3> vo = inv * Vector<RealT,3>(pnt[0],pnt[1],oz);
    return Point<RealT,2>(iz*vo[0]/vo[2],iz*vo[1]/vo[2]);          
  }
  
  template <class InT, class OutT,class MixerT,class SampleT>
  void WarpProjectiveC<InT,OutT,MixerT,SampleT>::Apply(const Array<InT,2> &src,Array<OutT,2> &outImg) {
    
    // First we have to decide what our output image frame is
    if (outImg.range().empty()) {
      if (!rec.empty())  outImg = Array<OutT,2>(rec);
      else { // use i/p frame projected into o/p coords
        Range<2,float> irng(src.range());
        Point<RealT,2> pnt = BackProject(irng.TopRight());
        Range<2,float> trng(pnt,pnt);
        trng.involve(BackProject(irng.TopLeft()));
        trng.involve(BackProject(irng.BottomRight()));
        trng.involve(BackProject(irng.BottomLeft()));
        IndexRange<2> oclip(Ceil(trng.min(0)),Floor(trng.max(0)),
                            Ceil(trng.min(1)),Floor(trng.max(1)));
        outImg = Array<OutT,2>(oclip);
      }
    }

    // Then we do the interpolation

    // pat:  o/p pt in o/p image cartesian coords
    // at:   o/p pt in i/p image homogeneous coords
    // ipat: o/p pt in i/p image cartesian coords
    // ldir: "horizontal" o/p increment in i/p image homogeneous coords
    // checkRange: valid region in i/p coords for truncated o/p pixel positions

    // set pat as top-left pixel in output image
    Point<RealT,2> pat(outImg.range().min());
    Vector<RealT,3> ldir(inv[0][1] * iz,inv[1][1] * iz,inv[2][1]);
    IndexRange<2> checkRange(src.range());
    checkRange.max(0) -= 1;
    checkRange.max(1) -= 1;
    OutT tmp;
    SampleT sampler;
    
    // Do simple check for each pixel that its contained in the input image.
    // This could be sped up by projecting the line corresponding to the
    // current row into the source image space, clipping it
    // and then projecting back into the output image and only iterate
    // along that bit of the line.
    // If the output maps entirely within input, we dont't have to do any
    // checking.  But we do.

    Array2dIterC<OutT> it(outImg);  
    for(;it;) { // iterate over rows
      Vector<RealT,3> at = inv * Vector<RealT,3>(pat[0],pat[1],oz);
      at[0] *= iz;
      at[1] *= iz;
      
      do { // iterate over cols
        Point<RealT,2> ipat(at[0]/at[2],at[1]/at[2]);
        if(checkRange.contains(Index<2>(Floor(ipat[0]),Floor(ipat[1])))) {
          sampler(src,ipat,tmp);
          mixer(*it,tmp);
        } else
          if(fillBackground) SetZero(*it);
        at += ldir;
      } while(it.next()) ;
      pat[0]++;
    }
  }
}


#endif
