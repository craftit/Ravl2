// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_REGIONGROWSTEAL_HEADER
#define RAVLIMAGE_REGIONGROWSTEAL_HEADER
//////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! example=exSegmentation.cc
//! file="Ravl/Image/Processing/Segmentation/RegionGrowSteal.hh"
//! author = "Fangxiang Cheng"
//! rcsid="$Id$"
//! lib=RavlImageProc
//! date="09/05/2000"

#include "Ravl/Image/SegmentRegion.hh"
#include "Ravl/Image/RegionSet.hh"
#include "Ravl/Image/Image.hh"
#include "Ravl/SArray1d.hh"
#include "Ravl/SArray1dIter.hh"
#include "Ravl/Array2dIter.hh"

namespace RavlImageN {
  
  //! userlevel=Develop
  //: Region growing class with each region grown sequentially
     
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  class RegionGrowStealBodyC 
    : public SegmentRegionBodyC<PixelT, StatT> 
  { 
  public:
    RegionGrowStealBodyC()
    {}
    //: Default constructor

    RegionGrowStealBodyC(const ClassifyT & cl)
      : classifier(cl)
    {}
    //: Default constructor
    
    void SetClassifier(const ClassifyT & cl)
    {classifier = cl;}
    //: set classifier info
  
    RegionSetC<StatT> Apply(const ImageC<PixelT> &in);
    //: Performs the segmentation on "in"
    
  protected:
    UIntT label;
    ImageC<ByteT> done;
    ImageC<UIntT> res;
    HashC<UIntT,StatT> stats;
    //ImageC<ByteT> steal_map;
    HashC<UIntT, ClassifyT> result; // hashtable container
    ClassifyT classifier;
    PixelSelectorT pxl_selector, pxl_selector_edge;
    ImageC<PixelT> img;
    Array2dIterC<ByteT> seed_it;
    
    void SetNewLabel() 
    { label++;}
    //: Select new label.
    
    Index2dC NextSeed();
    //: Find an unassigned pixel.
    
    bool ShouldGrow(const Index2dC &pxl) const
    { return ((classifier.Contains(pxl, img[pxl])&& !done[pxl])); }
    // if a pixel should be grown;
    
    void ShouldSteal(const Index2dC &);
    // include pixels into the stack if they should be stolen;
    
    void SideEffectsSelf(const Index2dC &pxl)
    { res[pxl] = label; done[pxl] = 1; classifier.Include(pxl, img[pxl]); }
    // side effects onto all the pixels grown?? check
    
    void SideEffectsNeigh(const Index2dC &)
    {}
    // side effects onto all the neighbours
    
    void SideEffectsSeed(const Index2dC &pxl)
    { classifier.Initialise(); classifier.Include(pxl, img[pxl]);}
    // for side effects onto all pixels visited use this + SideEffectsNeigh with the same body
  
    inline void ProcessPixel(const Index2dC &cpxl,const Index2dC &pxl);
    //: Process a pixel.
    
    void GrowPxl();
    //: Grow region around a pixel.
    
    void GrowComponent(const Index2dC &);
    //: Grow a component from a seed.
    
    void StealPixel(const Index2dC &cpxl,const Index2dC &chk);
    //: Steal a pixel from another region.
    
    void Steal();
    //: Check for pixels that should be reassigned.
  };
  
  //! userlevel=Normal
  //: Region growing class with each region grown sequentially
  //
  // Class for segmenting an image. Regions are grown sequentially. The seeds
  // are selected in a raster scan fashion from the portion of the image which
  // has not been processed yet. A fast stealing processing is implemented. it
  // is a little bit different from the one named "RegionGrowSteal", but much faster.
  
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  class RegionGrowStealC : public SegmentRegionC<PixelT, StatT> {
  public:
    RegionGrowStealC()
      : SegmentRegionC<PixelT, StatT> (*new RegionGrowStealBodyC<PixelSelectorT,ClassifyT,PixelT, StatT> ())
    {}
    //: Default constructor

    RegionGrowStealC(const ClassifyT &cl)
      : SegmentRegionC<PixelT, StatT> (*new RegionGrowStealBodyC<PixelSelectorT,ClassifyT,PixelT, StatT> (cl))
    {}
    //: Constructor

    void SetClassifier(const ClassifyT &cl){ Body().SetClassifier(cl); }
    //: set classifier info
    
  protected:
    RegionGrowStealBodyC<PixelSelectorT,ClassifyT,PixelT, StatT> &Body()
    { return static_cast<RegionGrowStealBodyC<PixelSelectorT,ClassifyT,PixelT, StatT> &>(RCHandleC<SegmentRegionBodyC<PixelT, StatT> >::Body()); }
  
    const RegionGrowStealBodyC<PixelSelectorT,ClassifyT,PixelT, StatT> &Body() const
    { return static_cast<const RegionGrowStealBodyC<PixelSelectorT,ClassifyT,PixelT, StatT> &>(RCHandleC<SegmentRegionBodyC<PixelT, StatT> >::Body()); }
  
  };

  

  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  Index2dC RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::NextSeed() 
  { 
    Index2dC seed = done.Rectangle().Origin().UpN();  
    for(; seed_it; seed_it++){
      if (seed_it.Data()==0){
	seed = seed_it.Index();
	break;
      }
    }
    return seed;
  }
  
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  void RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::ShouldSteal(const Index2dC & pxl) 
  {
    SArray1dC<Index2dC> neigh(4);// define 4-connected neighbouring system;
    neigh[0]=pxl.UpN();
    neigh[1]=pxl.DownN();
    neigh[2]=pxl.LeftN();
    neigh[3]=pxl.RightN();
    
    SArray1dIterC<Index2dC> n_it(neigh);
    for (n_it.First(); n_it.IsElm(); n_it.Next()){
      if (n_it.Data().IsInside(img.Rectangle())) {
	if((done[n_it.Data()]) && (res[n_it.Data()]!=res[pxl])) {
	  if (!pxl_selector_edge.IsInside(pxl))
	    pxl_selector_edge.Include(pxl);
	}
      }
    }
  }
  
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  void RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::StealPixel(const Index2dC &cpxl,const Index2dC &pxl) {
    if (!pxl.IsInside(img.Rectangle()))
      return ;
    if (!done[pxl] || res[cpxl] == res[pxl])
      return ;
    if ((classifier.Distance(pxl,img[pxl])) < result[res[pxl]].Distance(pxl,img[pxl])){
      if ((classifier.Counter()>100) ){
	//steal_map[pxl]=255;
	classifier.Include(pxl, img[pxl]);
	result[res[pxl]].Remove(pxl, img[pxl]);
	if  (result[res[pxl]].Counter()<=0){
	  if (result.Del(res[pxl])) {
	    //cout<<pxl<<"    "<<"one region removed"<<endl;
	  }
	}
	result.Update(res[pxl]);
	res[pxl]= res[cpxl];
	ShouldSteal(pxl);
      }
    }
  }
  
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  void RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::Steal()
  {
    Index2dC pxl = pxl_selector_edge.Next();
    StealPixel(pxl,pxl.UpN());
    StealPixel(pxl,pxl.DownN());
    StealPixel(pxl,pxl.LeftN());
    StealPixel(pxl,pxl.RightN());
  }
  
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  inline void RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::ProcessPixel(const Index2dC &cpxl,const Index2dC &pxl) {
    if (!pxl.IsInside(img.Rectangle()))
      return ;
    SideEffectsNeigh(pxl);
    if (ShouldGrow(pxl)){
      if (!pxl_selector.IsInside(pxl))
	pxl_selector.Include(pxl);
    }
    if((done[pxl]) && (res[pxl]!=res[cpxl])){
      if (!pxl_selector_edge.IsInside(cpxl))
	pxl_selector_edge.Include(cpxl);
    }
  }
  
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  void RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::GrowPxl() {
    Index2dC pxl = pxl_selector.Next();
    ProcessPixel(pxl,pxl.UpN());
    ProcessPixel(pxl,pxl.DownN());
    ProcessPixel(pxl,pxl.LeftN());
    ProcessPixel(pxl,pxl.RightN());    
    SideEffectsSelf(pxl);
  }

  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  void RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::GrowComponent(const Index2dC & pxl)
  {
    pxl_selector.Initialise();
    pxl_selector_edge.Initialise();
    pxl_selector.Include(pxl);
    SideEffectsSeed(pxl);
    while (!pxl_selector.IsEmpty())
      {GrowPxl();}
    while(!pxl_selector_edge.IsEmpty())
      {Steal();}
  }
  
  template<class PixelSelectorT, class ClassifyT, class PixelT, class StatT>
  RegionSetC<StatT> RegionGrowStealBodyC<PixelSelectorT, ClassifyT, PixelT, StatT>::Apply(const ImageC<PixelT> &in)
  {
    done = ImageC<ByteT>(in.Rectangle()); 
    done.Fill(0);
    //steal_map = ImageC<ByteT>(in.Rectangle());
    //steal_map.Fill(0);
    
    pxl_selector = PixelSelectorT(in.Rectangle());
    pxl_selector_edge = PixelSelectorT(in.Rectangle());
    res = ImageC<UIntT>(in.Rectangle()); 
    res.Fill(0);
    
    img = in.Copy();
    label = -1;
    seed_it = done;
    Index2dC pxl = NextSeed();
    while (pxl.IsInside(in.Rectangle())){
      SetNewLabel();
      GrowComponent(pxl);
      result.Insert(label, classifier); 
      pxl = NextSeed();
    }      // region growing procedure;;    
    //RavlN::Save("@X:steal_map",steal_map); // a map to tell which pixels are stolen;
    for (UIntT i=0; i<= label; i++)
      stats[i] = result[i].Stat();
    return RegionSetC<StatT>(res,label+1, stats);
  }
}
#endif





