// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_REGIONGROW_HEADER
#define RAVL_REGIONGROW_HEADER 1
//////////////////////////////////////////////////////////////
//! example=exSegmentation.cc
//! author="Ratna Rambaruth"
//! date="12/06/1998"

#include "Ravl2/Image/SegmentRegion.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Array.hh"
#include <vector>

namespace Ravl2 {

  template<class StatT> class RegionSetC;
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT> class RegionGrowC;
  
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
    
    RegionSetC<StatT> Apply(const Array<PixelT,2> &in)   {
      Array<ByteT,2> roi(in.range()); roi.fill(0);
      return Apply(in, roi);
    }
    //: Performs the segmentation of "in"
    
    RegionSetC<StatT> Apply(const Array<PixelT,2> &in, const Array<ByteT,2> & roi);
    //: Performs the segmentation on "in" on the region of interest "roi". The region of interest should be labelled 0 and all other pixels in the region should be labelled 1. In the output segmentation map, the pixels outside the region of interest are undefined. This function cannot be used as a process.
    
  protected:
    unsigned label;
    Array<ByteT,2> done;
    Array<unsigned,2> res;
    HashC<unsigned,StatT> stats;
    ClassifierT classifier;
    PixelSelectorT pxl_selector;
    Array<PixelT,2> img;
    Array2dIterC<ByteT> seed_it;
    
    bool eightConnectivity;
    
    void SetNewLabel() 
    { label++; }
    // Create a new label.
    
    Index<2> NextSeed();
    //: Find next seed point.
    
    bool ShouldGrow(const Index<2> &pxl) const
    { return (classifier.contains(pxl, img[pxl]) && !done[pxl]); }
    //: Check if we should grow a pixel.
    
    void SideEffectsSelf(const Index<2> &pxl) {
      res[pxl] = label;
      done[pxl] = 1;
      classifier.Include(pxl, img[pxl]);
    }
    //: side effects onto all the pixels grown?? check
    
    void SideEffectsNeigh(const Index<2> &)
    {}
    // side effects onto all the neighbours
    
    void SideEffectsSeed(const Index<2> &pxl)   {
      classifier.Initialise();
      classifier.Include(pxl, img[pxl]);
    }
    // for side effects onto all pixels visited use this + SideEffectsNeigh with the same body
    
    void GrowPxl();
    //: Grow a pixel.
    
    inline void ProcessPixel(const Index<2> &pxl);
    //: Process a single neighbouring pixel.
    
    StatT GrowComponent(const Index<2> &);
  };
  
  
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

    RegionSetC<StatT> Apply(const Array<PixelT,2> &in) {return Body().apply(in); }
    //: Performs the segmentation on "in"

    RegionSetC<StatT> Apply(const Array<PixelT,2> &in, const Array<ByteT,2> & roi) { return Body().apply(in, roi); }
    //: Performs the segmentation on "in" on the region of interest "roi". The region of interest should be labelled 0 and all other pixels in the image should be labelled 1. In the output segmentation map, the pixels outside the region of interest are undefined. This function cannot be used as a process.

  protected:
    RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &Body()
    { return static_cast<RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &>(RCHandleC<SegmentRegionBodyC<PixelT,StatT> >::Body()); }
    
    const RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &Body() const
    { return static_cast<const RegionGrowBodyC<PixelSelectorT,ClassifierT,PixelT,StatT> &>(RCHandleC<SegmentRegionBodyC<PixelT,StatT> >::Body()); }
  
  };
  
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  Index<2> RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::NextSeed()  { 
    Index<2> seed = done.range().min().UpN();
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
  inline void RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::ProcessPixel(const Index<2> &pxl) {
    if (!pxl.IsInside(img.range()))
      return ;
    SideEffectsNeigh(pxl);
    if (!ShouldGrow(pxl))
      return ;
    if (!pxl_selector.IsInside(pxl))
      pxl_selector.Include(pxl);
  }
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  void RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::GrowPxl() {
    Index<2> pxl = pxl_selector.next();
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
  StatT RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::GrowComponent(const Index<2> & pxl) {
    pxl_selector.Initialise();
    pxl_selector.Include(pxl);
    SideEffectsSeed(pxl);
    while (!pxl_selector.empty())
      GrowPxl();
    return classifier.Stat();
  }
  
  template<class PixelSelectorT, class ClassifierT, class PixelT, class StatT>
  RegionSetC<StatT> RegionGrowBodyC<PixelSelectorT, ClassifierT, PixelT, StatT>::Apply(const Array<PixelT,2> &in, const Array<ByteT,2> & roi) {
    done = roi.Copy();
    pxl_selector = PixelSelectorT(in.range());
    label = -1;
    res = Array<unsigned,2>(in.range()); 
    res.fill(label);
    img = in.Copy();
    seed_it = done;
    Index<2> pxl = NextSeed();
    while (pxl.IsInside(in.range())){
      SetNewLabel();
      StatT stat = GrowComponent(pxl);
      stats[label] = stat;
      pxl = NextSeed();
    }
    return RegionSetC<StatT>(res, label+1,stats);
  }

}

#endif




