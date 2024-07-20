// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_REGIONGROW_HEADER
#define RAVL_REGIONGROW_HEADER 1
//////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! example=exSegmentation.cc
//! file="Ravl/Image/Processing/Segmentation/RegionGrow.hh"
//! lib=RavlImageProc
//! author="Ratna Rambaruth"
//! date="12/06/1998"
//! rcsid="$Id$"

#include "Ravl/RefCounter.hh"
#include "Ravl/Image/SegmentRegion.hh"
#include "Ravl/Index2d.hh"
#include "Ravl/Array2dIter.hh"
#include "Ravl/Image/Image.hh"
#include "Ravl/Hash.hh"
#include "Ravl/Image/Image.hh"
#include "Ravl/SArray1d.hh"
#include "Ravl/SArray1dIter.hh"

namespace RavlImageN {

  template<class StatT> class RegionSetC;
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT> class RegionGrowC;
  
  //! userlevel=Develop
  //: Region growing class with each region grown sequentially
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  class RegionGrowBodyC 
    : public SegmentRegionBodyC<PixelT,StatT> 
  { 
  public:
    RegionGrowBodyC()
      : eightConnectivity(false)
    {}
    //: default constructor
    
    RegionGrowBodyC(const ClassifierT & cl)
      : classifier(cl),
        eightConnectivity(false)
    {}
    //: Construct from a classifier.
    
    void SetClassifier(const ClassifierT & cl)
    { classifier = cl; }
    //: Set classifier info
    
    RegionSetC<StatT> Apply(const ImageC<PixelT> &in)   {
      ImageC<ByteT> roi(in.Rectangle()); roi.Fill(0);
      return Apply(in, roi);
    }
    //: Performs the segmentation of "in"
    
    RegionSetC<StatT> Apply(const ImageC<PixelT> &in, const ImageC<ByteT> & roi);
    //: Performs the segmentation on "in" on the region of interest "roi". The region of interest should be labelled 0 and all other pixels in the region should be labelled 1. In the output segmentation map, the pixels outside the region of interest are undefined. This function cannot be used as a process.
    
  protected:
    UIntT label;
    ImageC<ByteT> done;
    ImageC<UIntT> res;
    HashC<UIntT,StatT> stats;
    ClassifierT classifier;
    PixelSelectorT pxl_selector;
    ImageC<PixelT> img;
    Array2dIterC<ByteT> seed_it;
    
    bool eightConnectivity;
    
    void SetNewLabel() 
    { label++; }
    // Create a new label.
    
    Index2dC NextSeed();
    //: Find next seed point.
    
    bool ShouldGrow(const Index2dC &pxl) const
    { return (classifier.Contains(pxl, img[pxl]) && !done[pxl]); }
    //: Check if we should grow a pixel.
    
    void SideEffectsSelf(const Index2dC &pxl) {
      res[pxl] = label;
      done[pxl] = 1;
      classifier.Include(pxl, img[pxl]);
    }
    //: side effects onto all the pixels grown?? check
    
    void SideEffectsNeigh(const Index2dC &)
    {}
    // side effects onto all the neighbours
    
    void SideEffectsSeed(const Index2dC &pxl)   {
      classifier.Initialise();
      classifier.Include(pxl, img[pxl]);
    }
    // for side effects onto all pixels visited use this + SideEffectsNeigh with the same body
    
    void GrowPxl();
    //: Grow a pixel.
    
    inline void ProcessPixel(const Index2dC &pxl);
    //: Process a single neighbouring pixel.
    
    StatT GrowComponent(const Index2dC &);
  };
  
  
  //! userlevel=Normal
  //: Region growing class with each region grown sequentially
  //
  // Class for segmenting an image. Regions are grown sequentially. The seeds
  // are selected in a raster scan fashion from the portion of the image which
  // has not been processed yet.
     
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  class RegionGrowC 
    : public SegmentRegionC<PixelT,StatT> 
  {
  public:
    RegionGrowC()
      : SegmentRegionC<PixelT,StatT> 
    (*new RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> ())
    {}
    //: Default constructor

    RegionGrowC(const ClassifierT &cl)
      : SegmentRegionC<PixelT,StatT> 
    (*new RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> (cl))
    {}
    //: Construct from a classifier


    void SetClassifier(const ClassifierT &cl){ Body().SetClassifier(cl); }
    //: set classifier info

    RegionSetC<StatT> Apply(const ImageC<PixelT> &in) {return Body().Apply(in); }
    //: Performs the segmentation on "in"

    RegionSetC<StatT> Apply(const ImageC<PixelT> &in, const ImageC<ByteT> & roi) { return Body().Apply(in, roi); }
    //: Performs the segmentation on "in" on the region of interest "roi". The region of interest should be labelled 0 and all other pixels in the image should be labelled 1. In the output segmentation map, the pixels outside the region of interest are undefined. This function cannot be used as a process.

  protected:
    RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &Body()
    { return static_cast<RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &>(RCHandleC<SegmentRegionBodyC<PixelT,StatT> >::Body()); }
    
    const RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &Body() const
    { return static_cast<const RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &>(RCHandleC<SegmentRegionBodyC<PixelT,StatT> >::Body()); }
  
  };
  
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  Index2dC RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::NextSeed()  { 
    Index2dC seed = done.Rectangle().Origin().UpN();
    //seed_it = done;
    for(;seed_it; seed_it++) {
      if (seed_it.Data()==0) {
	seed = seed_it.Index();
	break;
      }
    }
    return seed;
  }
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  inline void RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::ProcessPixel(const Index2dC &pxl) {
    if (!pxl.IsInside(img.Rectangle()))
      return ;
    SideEffectsNeigh(pxl);
    if (!ShouldGrow(pxl))
      return ;
    if (!pxl_selector.IsInside(pxl))
      pxl_selector.Include(pxl);
  }
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  void RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::GrowPxl() {
    Index2dC pxl = pxl_selector.Next();
    SideEffectsSelf(pxl);
    ProcessPixel(pxl.UpN());
    ProcessPixel(pxl.DownN()); 
    ProcessPixel(pxl.LeftN()); 
    ProcessPixel(pxl.RightN());
    
    if(eightConnectivity) {
      ProcessPixel(pxl.LowerLeftN());
      ProcessPixel(pxl.LowerRightN());
      ProcessPixel(pxl.UpperLeftN());
      ProcessPixel(pxl.UpperRightN());
    }
  }
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  StatT RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::GrowComponent(const Index2dC & pxl) {
    pxl_selector.Initialise();
    pxl_selector.Include(pxl);
    SideEffectsSeed(pxl);
    while (!pxl_selector.IsEmpty())
      GrowPxl();
    return classifier.Stat();
  }
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  RegionSetC<StatT> RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::Apply(const ImageC<PixelT> &in, const ImageC<ByteT> & roi) {
    done = roi.Copy();
    pxl_selector = PixelSelectorT(in.Rectangle());
    label = -1;
    res = ImageC<UIntT>(in.Rectangle()); 
    res.Fill(label);
    img = in.Copy();
    seed_it = done;
    Index2dC pxl = NextSeed();
    while (pxl.IsInside(in.Rectangle())){
      SetNewLabel();
      StatT stat = GrowComponent(pxl);
      stats[label] = stat;
      pxl = NextSeed();
    }
    return RegionSetC<StatT>(res, label+1,stats);
  }

}

#endif




