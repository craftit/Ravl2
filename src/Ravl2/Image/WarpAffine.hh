// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Images.Scaling and Warping"
//! author="Charles Galambos"
//! date="16/07/2002"

#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/LinePP.hh"
//#include "Ravl2/Image/PixelMixer.hh"
#include "Ravl2/Image/BilinearInterpolation.hh"

namespace Ravl2 {


#if 0
  //! Performs an affine warp of an image using bi-Linear Interpolation.

  template <class InT, class OutT = InT,class WorkT = OutT,typename RealT = float,class MixerT = PixelMixerAssignC<WorkT,OutT>,class SampleT = SampleBilinearC<InT,WorkT> >
  class WarpAffineC
  {
  public:
    WarpAffineC(const IndexRange<2> &outRec,const Affine<RealT,2> &ntrans,bool nFillBackground = true,const MixerT &mix = MixerT())
      : rec(outRec),
	trans(ntrans),
	fillBackground(nFillBackground),
	mixer(mix),
	useMidPixelCorrection(true)
    {}
    //: Constructor.
    // 'outRec' is the output rectangle.
    // 'ntrans' is the transform to use. Maps output image pixel to one in input.
    
    WarpAffineC(const IndexRange<2> &outRec,bool nFillBackground = true,bool doMidPointCorrection = true,const MixerT &mix = MixerT())
      : rec(outRec),
	fillBackground(nFillBackground),
	mixer(mix),
	useMidPixelCorrection(doMidPointCorrection)
    {}
    //: Constructor.
    // 'outRec' is the output rectangle.
    
    void Apply(const Array<InT,2> &inImg,Array<OutT,2> &outImg);
    //: Interpolate input image working rectangle into output image.
    // If the supplied transformation transforms points in image A to
    // corresponding positions in image B
    // (i.e. transforms coordinate system of B to that of A),
    // Then this method warps image B to coordinate system of A.
    
    Array<OutT,2> Apply(const Array<InT,2> &img) {
      Array<OutT,2> out(rec);
      Apply(img,out);
      return out;
    }
    //: Interpolate input image working rectangle into
    //: output image rectangle.
    // The output rectangle is specified in the constructor.
    // If the supplied transformation transforms points in image A to
    // corresponding positions in image B
    // (i.e. transforms coordinate system of B to that of A),
    // Then this method warps image B to coordinate system of A.
    
    void SetTransform(const Affine<RealT,2> &transform) {
      trans = transform;
    }
    //: Set the current transform.
    
    IndexRange<2> InputRectangle() const { 
      Range<2,float> orng(rec);
      Range<2,float> rret(trans * orng.TopRight(),0);
      rret.involve(trans * orng.TopLeft());
      rret.involve(trans * orng.BottomRight());
      rret.involve(trans * orng.BottomLeft());
      return rret.IndexRange();
    }
    //: Get range of input rectangle that will be used.
    // Note: This may be larger than the actual input provided.

    IndexRange<2> InputRectangle(const IndexRange<2> &outRec) const { 
      Range<2,float> orng(outRec);
      Range<2,float> rret(trans * orng.TopRight(),0);
      rret.involve(trans * orng.TopLeft());
      rret.involve(trans * orng.BottomRight());
      rret.involve(trans * orng.BottomLeft());
      return rret.IndexRange();
    }
    //: Get range of input rectangle that is need to fill 'outRec'
    
    IndexRange<2> OutputRectangle(const IndexRange<2> &inrec) const { 
      Range<2,float> irng(inrec);
      Affine<RealT,2> itrans = trans.I();
      Range<2,float> rret(itrans * irng.TopRight(),0);
      rret.involve(itrans * irng.TopLeft());
      rret.involve(itrans * irng.BottomRight());
      rret.involve(itrans * irng.BottomLeft());
      return rret.IndexRange();
    }
    //: Get range of output rectangle that will required to get get all of 'inrec'.
    
    MixerT &Mixer() 
    { return mixer; }
    //: Access mixer class.
    
    void SetOutputRectangle(const IndexRange<2> &rng)
    { rec = rng; }
    //: Set the output rectangle.
    
    void SetMidPixelCorrection(bool correction)
    { useMidPixelCorrection = correction; }
    //: Set mid pixel correction flag.
    //!param: correction = true - coordinate system is at top l.h corner of pixel (0,0)
    //!param: correction = false - coordinate system is at centre of pixel (0,0)
    
  protected:
    IndexRange<2> rec;   // Output rectangle.
    Affine<RealT,2> trans;       // Transform.
    bool fillBackground;   // Fill background with zero ?
    MixerT mixer;
    bool useMidPixelCorrection;
  };
  
  template <class InT, class OutT,class WorkT,class MixerT,class SampleT >
  void WarpAffineC<InT,OutT,WorkT,MixerT,SampleT>::Apply(const Array<InT,2> &src,Array<OutT,2> &outImg) {
    Range<2,float> irng(src.range());
    irng = irng.expand(-1.001); // There's an off by a bit error somewhere in here...
    Range<2,float> orng(rec);
    if(!outImg.IsValid())
      outImg = Array<OutT,2>(rec);
    else
      orng = Range<2,float>(outImg.range());
    
    //cerr << "Trans0=" << trans * orng.TopRight() << " from " << orng.TopRight() << "\n";
    
    const Matrix<RealT,2,2> &srm = trans.SRMatrix();
    Vector<RealT,2> ldir(srm[0][1],srm[1][1]);
    Vector<RealT,2> sdir(srm[0][0],srm[1][0]);
    Affine<RealT,2> localTrans = trans;
    if (useMidPixelCorrection)
      // Equivalent to: shift 1/2 pixel; transform; shift back again.
      localTrans.Translate(srm*Vector<RealT,2>(0.5,0.5)+Vector<RealT,2>(-0.5,-0.5));
    Point<RealT,2> lstart = localTrans * Point<RealT,2>(orng.min());
    Array2dIterC<OutT> it(outImg);
    
    WorkT tmp;
    SampleT sampler;
    if(irng.contains(localTrans * orng.TopRight()) &&
       irng.contains(localTrans * orng.TopLeft()) &&
       irng.contains(localTrans * orng.BottomRight()) &&
       irng.contains(localTrans * orng.BottomLeft())) {
      // Output maps entirely within input, so we don't have to do any checking!
      for(;it;) {
	Point<RealT,2> pat = lstart;
	do {
	  sampler(src,pat,tmp);
	  mixer(*it,tmp);
	  pat += ldir;
	} while(it.next()) ;
	lstart += sdir;
      }
      return;
    }    
#if 0
    Vector<RealT,2> endv(0,((RealT) orng.range(1).size()));
    // This attempts to be clever project the line back into
    // the source space, clipping it and the projecting it back
    // again.
    for(;it;) {
      Point<RealT,2> pat = lstart;
      LinePP2dC rline(pat,endv);
      if(!rline.clipBy(irng)) {
	it.NextRow();
	lstart += sdir;
	continue;
      }
      // Map clipped line back into output image.
      Point<RealT,2> sp = ilocalTrans * rline.P1();
      Point<RealT,2> ep = ilocalTrans * rline.P2();
      
      int c = (((int) sp[1]) - outImg.min(1));
      if(fillBackground) {
	for(;c >= 0;c--,it++)
	  SetZero(*it);
      } else
	it.NextCol(c);
      int ce = (((int) ep[1]) - outImg.min(1));
      const OutT *end = &(*it) + ce;
      for(;&(*it) < end;it.NextCol()) {
	sampler(src,pat,*it);
	pat += ldir;
      }
      if(fillBackground) {
	for(;it.IsColElm();it.NextCol())
	  SetZero(*it);
      } else
	it.NextRow();
      lstart += sdir;
    }
#else 
    // Do simple check for each pixel that its contained in the input image.
    for(;it;) {
      Point<RealT,2> pat = lstart;
      if(fillBackground) {
	do {
	  if(irng.contains(pat)) {
	    sampler(src,pat,tmp);
	    mixer(*it,tmp);
	  } else
	    SetZero(*it);
	  pat += ldir;
	} while(it.next()) ;
      } else {
	do {
	  if(irng.contains(pat)) {
	    sampler(src,pat,tmp);
	    mixer(*it,tmp);
	  }
	  pat += ldir;
	} while(it.next()) ;
      }
      lstart += sdir;
    }
    
#endif    
  }
#endif

}
