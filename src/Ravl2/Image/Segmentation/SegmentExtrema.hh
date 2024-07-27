// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Charles Galambos, based on code by Jiri Matas."
//! docentry="Ravl.API.Images.Segmentation"
//! file="Ravl/Image/Processing/Segmentation/SegmentExtrema.hh"
//! example="extrema.cc" 

#include <memory>
#include <cstdint>
#include "Ravl2/Array.hh"
#include "Ravl2/Image/Segmentation/FloodRegion.hh"
#include "Ravl2/IndexRangeSet.hh"

namespace Ravl2 {

  //! userlevel=Develop
  //: Extrema threshold information.
  
  class ExtremaThresholdC
  {
  public:
    int thresh;
    int pos;
    int margin;
    uint32_t area; // Expected area of region.
  };
  
  //! userlevel=Develop
  //: Extremal region
  
  class ExtremaRegionC {
  public:
    ExtremaRegionC()
      : merge(0),
        hist(0),
        total(0),
	thresh(0),
	nThresh(0),
	maxValue(0),
	minValue(0),
	closed(0)
    {}
    //: Constructor.
    
    ~ExtremaRegionC() {
      if(hist != 0)
	delete [] hist;
      if(thresh != 0)
	delete [] thresh;
    }
    //: Destructor.
    
    ExtremaRegionC *merge;
    int *hist; // Histogram of levels.
    int total;
    
    ExtremaThresholdC *thresh; // Thresholds
    int nThresh; // Number of thresholds.
    
    int maxValue;
    int minValue;
    Index<2> minat;
    ExtremaRegionC *closed;
  };
  
  //! userlevel=Develop
  //: Extremal pixel list.
  
  class ExtremaChainPixelC {
  public:
    ExtremaRegionC *region;
    ExtremaChainPixelC *next;
  };

  //! userlevel=Advanced
  //: Common parts to segmenting extrema.
  
  class SegmentExtremaBaseC {
  public:
    using RealT = float;

    SegmentExtremaBaseC(int nMinSize,RealT nMinMargin,int nlimitMaxValue = 255)
      : stride(0),
        labelAlloc(1),
        origin(0),
	minSize(nMinSize),
        maxSize(0), // This gets modified later.
	minMargin(nMinMargin),
	limitMaxValue(std::max(nlimitMaxValue,3)),
	histStack(0)
    {}
    //: Constructor.
    
    ~SegmentExtremaBaseC();
    //: Destructor.
    
    void SetupImage(const IndexRange<2> &rect);
    //: Setup structures for a given image size.
    
    void GenerateRegions();
    //: Generate regions.
    
    unsigned Levels() const 
    { return levels.Size(); }
    //: Get number of levels being considered.
    
    int MinSize() const 
    { return minSize; }
    //: Access minimum size.
    
    int MaxSize() const 
    { return maxSize; }
    //: Access maximum size.
    
    RealT MinMargin() const
    { return minMargin; }
    //: Access minimum margin
    
    int LimitMaxValue() const
    { return limitMaxValue; }
    //: Access value limit
    
    Array1dC<ExtremaChainPixelC *> &LevelSets()
    { return levels; }
    //: Access level set array.
    
  protected:
    void ReallocRegionMap(int size);
    //: Reallocate the current region set, free any memory used.
    
    static ExtremaRegionC *FindLabel(ExtremaChainPixelC *lab);
    //: Find matching label.
    
    int ConnectedLabels(ExtremaChainPixelC *pix,ExtremaRegionC **labelArray);
    //: Find the labels around the pixel 'pix'
    // put the results into 'labelArray' which must be at least 4 labels long.
    // The number of labels found is returned.

    int ConnectedLabels6(ExtremaChainPixelC *pix,ExtremaRegionC **labelArray);
    //: Find the labels around the pixel 'pix'
    // put the results into 'labelArray' which must be at least 4 labels long.
    // The number of labels found is returned.
        
    void AddRegion(ExtremaChainPixelC *pix,int level);
    //: Add a new region.
    
    void AddPixel(ExtremaChainPixelC *pix,int level,ExtremaRegionC *reg);
    //: Add pixel to region.
    
    void MergeRegions(ExtremaChainPixelC *pix,int level,ExtremaRegionC **labels,int n);
    //: Add pixel to region.

    void Thresholds();
    //: Generate thresholds
    
    void Thresholds2();
    //: Generate thresholds
    
    void PeakDetection(Array1dC<RealT> &real);
    //: Peak detection.
    
    Array<ExtremaChainPixelC,2> pixs;
    Array1dC<ExtremaChainPixelC *> levels;
    SArray1dC<ExtremaRegionC> regionMap;
    unsigned stride; // Image stride.
    unsigned labelAlloc;
    IndexRange<1> valueRange; // Range of pixel values.
    ExtremaChainPixelC *origin;
    
    // Paramiters.
    
    int minSize;
    int maxSize;
    RealT minMargin;
    int limitMaxValue; // Maximum image value that will be encountered.

    struct HistStackC {
      int max;
      HistStackC *next;
    };
    
    HistStackC *histStack;
    
    int *PopHist(int level) {
      if(histStack == 0) {
	int *val =  new int [limitMaxValue+2];
	memset(&(val[level]),0,sizeof(int) * ((limitMaxValue+1)-level));
	return val;
      }
      HistStackC *ret = histStack;
      histStack = ret->next;
      int *rret = reinterpret_cast<int *>(ret);
      int clearSize = std::max(ret->max+1,3) - level;
      if(clearSize > 0)
	memset(&(rret[level]),0,sizeof(int) * clearSize);
      return rret;
    }
    //: Get a new histogram.
    
    void PushHist(int *stack,int used) {
      HistStackC *tmp = reinterpret_cast<HistStackC *>(stack);
      tmp->max = used;
      tmp->next = histStack;
      histStack = tmp;
    }
    //: Push unused histogram
  };
  
  
  //! userlevel=Normal
  //: Maximimally stable extremal region's  (MSER's)

  // <p>In most images there are regions that can be detected with high repeatability since they
  // possess some distinguishing, invariant and stable property, the so called extremal regions.
  // Extremal regions have two desirable properties.  Firstly, the set is closed under
  // continuous (and thus perspective) transformation of image coordinates
  // and, secondly, it is closed under monotonic transformation of image intensities.
  // This class is an implementation of an efficient (near linear complexity) and practically
  // fast detection algorithm
  // for an affine-invariant stable subset of extremal regions, the maximally stable extremal
  // regions.</p>

  // <p> The concept can be explained informally as follows. Imagine all possible thresholdings
  // of a gray-level image I. We will refer to the pixels below a threshold as "black" and
  // to those above or equal as "white". If we were shown a movie of thresholded images It,
  // with frame t corresponding to threshold t, we would see first a white image. Subsequently
  // black spots corresponding to local intensity minima will appear and grow. At some point
  // regions corresponding to two local minima will merge. Finally, the last image will be
  // black. The set of all connected components of all frames of the movie is the set of all
  // maximal regions; minimal regions could be obtained by inverting the intensity of I and
  // running the same process.</p>

  // <p>References: <a href="http://cmp.felk.cvut.cz/~matas/papers/matas-bmvc02.pdf">J Matas et al. BMVC 2002</a>; Image and Vision Computing, 22(10):761-767, September 2004. </p>

  template<class PixelT>
  class SegmentExtremaC 
    : public SegmentExtremaBaseC
  {
  public:
    SegmentExtremaC(int minRegionSize,RealT minMargin = 10,int nlimitMaxPixelValue = 255)
      : SegmentExtremaBaseC(minRegionSize,minMargin,nlimitMaxPixelValue)
    {}
    //: Constructor.
    //!param:minRegionSize - Minimum region size to detect.
    //!param:minMargin - Threshold for region stability.
    
    DListC<BoundaryC> Apply(const Array<PixelT,2> &img) {
      if(typeid(PixelT) == typeid(ByteT)) // Should decided at compile time.
        SortPixelsByte(img);
      else
        SortPixels(img);
      GenerateRegions();
      Thresholds();
      return GrowRegionBoundary(img);
    }
    //: Apply operation to img.
    // Note: The boundaries generated by this function are not ordered. If you require ordered
    // boundries you can use its 'Order()' method to sort them.
    
    DListC<Array<int,2> > ApplyMask(const Array<PixelT,2> &img) {
      if(typeid(PixelT) == typeid(ByteT)) // Should decided at compile time.
        SortPixelsByte(img);
      else
        SortPixels(img);
      GenerateRegions();
      Thresholds();
      return GrowRegionMask(img);
    }
    //: Apply operation to img.
    // Generates labeled images.

    DListC<BoundaryC> Apply(const Array<PixelT,2> &img,const IndexRange2dSetC &roi) {
      SortPixels(img,roi);
      GenerateRegions();
      Thresholds();
      return GrowRegionBoundary(img);
    }
    //: Apply operation to img.
    // Note: The boundaries generated by this function are not ordered. If you require ordered
    // boundries you can use its 'Order()' method to sort them.
    
    DListC<Array<int,2> > ApplyMask(const Array<PixelT,2> &img,const IndexRange2dSetC &roi) {
      SortPixels(img,roi);
      GenerateRegions();
      Thresholds();
      return GrowRegionMask(img);
    }
    //: Apply operation to img.
    // Generates labeled images.
    
  protected:
    bool SortPixels(const Array<PixelT,2> &nimg);
    //: Do initial sorting of pixels.
    
    bool SortPixelsByte(const Array<PixelT,2> &nimg);
    //: Do initial sorting of pixels.
    
    bool SortPixels(const Array<PixelT,2> &img,const IndexRange2dSetC &roi);
    //: Build a list from a byte image in regions of interest.

    DListC<BoundaryC> GrowRegionBoundary(const Array<PixelT,2> &img);
    //: Grow regions.
    
    DListC<BoundaryC> GrowRegionBoundary(ExtremaRegionC &region);
    //: Grow regions associated with a extrema.
    
    DListC<Array<int,2> > GrowRegionMask(const Array<PixelT,2> &img);
    //: Grow regions.
    
    DListC<Array<int,2> > GrowRegionMask(ExtremaRegionC &region);
    //: Grow regions associated with a extrema.
    
    FloodRegionC<PixelT> flood; // Region fill code.    
        
    
  };

  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const Array<PixelT,2> &img) {
    
    SetupImage(img.Frame());
    // Find range of values in image.
    {
      auto it = img.begin();
      PixelT lmin = *it;
      PixelT lmax = *it;
      for(;it;it++) {
        if(*it < lmin)
          lmin = *it;
        if(*it > lmax)
          lmax = *it;
      }
      valueRange.min() = (int) lmin;
      valueRange.max() = (int) lmax;
    }
    
    if(valueRange.max() >= limitMaxValue)
      valueRange.max() = limitMaxValue-1;
    
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    
    // Check level list allocation.
    
    if(!levels.Range().Contains(valueRange))
      levels = Array<ExtremaChainPixelC *,1>(valueRange.expand(2));
    levels.Fill(0);
    
    // Sort pixels into appropriate lists.
    
    for(Array2dIter2C<PixelT,ExtremaChainPixelC> it(img,pixs,img.Frame());it;it++) {
      it.Data2().region = 0;
      PixelT val = it.Data1();
      if(val > limitMaxValue) {
        continue;
      }
      ExtremaChainPixelC * &tmp = levels[val]; 
      it.Data2().next = tmp;
      tmp = &it.Data2();
    }
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    
    return true;
  }
  
  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixelsByte(const Array<PixelT,2> &img) {
    SetupImage(img.Frame());
    
    // Check level list allocation.
    
    if(levels.Size() == 0)
      levels = Array<ExtremaChainPixelC *,1>(IndexRange<1>(-4,257));
    memset(&(levels[levels.IMin()]),0,levels.Size() * sizeof(ExtremaChainPixelC *));
    
    // Sort pixels into appropriate lists.
    
    BufferAccess2dIter2C<PixelT,ExtremaChainPixelC> it(img,img.Frame().Range1(),img.Frame().Range2(),
                                                       pixs,img.Frame().Range1(),img.Frame().Range2());
    PixelT lmin = it.Data1();
    PixelT lmax = it.Data1();
    if(it.Data1() > limitMaxValue) {
      lmax = limitMaxValue;
      lmin = limitMaxValue;
    }
    if(limitMaxValue >= 255) {
      for(;it;it++) {
        it.Data2().region = 0;
        PixelT val = it.Data1();
        if(val < lmin) lmin = val;
        if(val > lmax) lmax = val;
        ExtremaChainPixelC * &tmp = levels[val]; 
        it.Data2().next = tmp;
        tmp = &it.Data2();
      }
    } else {
      for(;it;it++) {
        it.Data2().region = 0;
        PixelT val = it.Data1();
        if(val > limitMaxValue)
          continue;
        if(val < lmin) lmin = val;
        if(val > lmax) lmax = val;
        ExtremaChainPixelC * &tmp = levels[val]; 
        it.Data2().next = tmp;
        tmp = &it.Data2();
      }
    }
    valueRange.Min() = (int) lmin;
    valueRange.Max() = (int) lmax;
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    return true;
  }
  
  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const Array<PixelT,2> &img,const IndexRange2dSetC &roi) {
    
    SetupImage(img.Frame());
    
    // Find range of values in image.
    bool first = true;
    PixelT lmin = 0;
    PixelT lmax = 0;
    for(DLIterC<IndexRange<2>> rit(roi);rit;rit++) {
      IndexRange<2> rng = *rit;
      rng.ClipBy(img.Frame());
      if(rng.Area() < 1)
	continue;
      Array2dIterC<PixelT> it(img,rng);
      if(first) {
        lmin = *it;
        lmax = *it;
        first = false;
      }
      for(;it;it++) {
	if(*it < lmin)
	  lmin = *it;
	if(*it > lmax)
	  lmax = *it;
      }
    }
    valueRange.Min() = (int) lmin;
    valueRange.Max() = (int) lmax;
    if(valueRange.Max() > limitMaxValue)
      valueRange.Max() = limitMaxValue;
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    // Check level list allocation.
    
    if(!levels.Range().Contains(valueRange))
      levels = Array1dC<ExtremaChainPixelC *>(valueRange.Expand(2));
    levels.Fill(0);
    
    // Clear chain image.
    
    for(Array2dIterC<ExtremaChainPixelC> it(pixs,img.Frame());it;it++) {
      it.Data().region = 0;
      it.Data().next = 0;
    }
    
    // Sort pixels into appropriate lists.
    
    for(DLIterC<IndexRange<2>> rit(roi);rit;rit++) {
      IndexRange<2> rng = *rit;
      rng.ClipBy(img.Frame());
      if(rng.Area() < 1)
	continue;
      for(Array2dIter2C<PixelT,ExtremaChainPixelC> it(img,pixs,rng);it;it++) {
	PixelT val = it.Data1();
	if(val > limitMaxValue)
	  continue;
	ExtremaChainPixelC * &tmp = levels[val]; 
	it.Data2().next = tmp;
	tmp = &it.Data2();
      }
    }
    return true;
  }

  
  template<class PixelT>
  DListC<BoundaryC> SegmentExtremaC<PixelT>::GrowRegionBoundary(const Array<PixelT,2> &img) {
    //cerr << "SegmentExtremaBaseC::GrowRegions() \n";
    flood.SetupImage(img);
    
    DListC<BoundaryC> bounds;
    if(labelAlloc == 0)
      return bounds;
    for(SArray1dIterC<ExtremaRegionC> it(regionMap,labelAlloc-1);it;it++) {
      if(it->nThresh > 0) {
	DListC<BoundaryC> tmp=GrowRegionBoundary(*it);
	bounds.MoveLast(tmp);
      }
      if(it->thresh != 0) {
	delete [] it->thresh;
	it->thresh = 0;
      }
    }
    
    return bounds;
  }
  
  template<class PixelT>
  DListC<BoundaryC> SegmentExtremaC<PixelT>::GrowRegionBoundary(ExtremaRegionC &region) {
    DListC<BoundaryC> ret;
    for(int i = 0;i < region.nThresh;i++) {
      BoundaryC boundary;
      if(flood.GrowRegion(region.minat,region.thresh[i].thresh,boundary))
	ret.InsLast(boundary);
    }
    return ret;
  }
  
  template<class PixelT>
  DListC<Array<int,2> > SegmentExtremaC<PixelT>::GrowRegionMask(const Array<PixelT,2> &img) {
    //cerr << "SegmentExtremaBaseC::GrowRegions() \n";
    flood.SetupImage(img);
    
    DListC<Array<int,2> > masks;
    if(labelAlloc == 0)
      return masks;
    for(SArray1dIterC<ExtremaRegionC> it(regionMap,labelAlloc-1);it;it++) {
      if(it->nThresh > 0) {
	DListC<Array<int,2> > tmp = GrowRegionMask(*it);
        masks.MoveLast(tmp);
      }
      if(it->thresh != 0) {
        delete [] it->thresh;
        it->thresh = 0;
      }
    }

    return masks;
  }
  
  template<class PixelT>
  DListC<Array<int,2> > SegmentExtremaC<PixelT>::GrowRegionMask(ExtremaRegionC &region) {
    DListC<Array<int,2> > ret;
    //cerr << " Thresholds=" << region.nThresh << "\n";
    for(int i = 0;i < region.nThresh;i++) {
      Array<int,2> mask;
      if(flood.GrowRegion(region.minat,region.thresh[i].thresh,mask,1))
        ret.InsLast(mask);
    }
    return ret;
  }
  

}

