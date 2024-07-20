// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_SEGMENTEXTREMA_HEADER
#define RAVLIMAGE_SEGMENTEXTREMA_HEADER 1
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Charles Galambos, based on code by Jiri Matas."
//! docentry="Ravl.API.Images.Segmentation"
//! file="Ravl/Image/Processing/Segmentation/SegmentExtrema.hh"
//! example="extrema.cc" 

#include "Ravl/Image/Image.hh"
#include "Ravl/Image/FloodRegion.hh"
#include "Ravl/SArray1d.hh"
#include "Ravl/Array1d.hh"
#include "Ravl/Array1dIter.hh"
#include "Ravl/SArray1dIter.hh"
#include "Ravl/IndexRange2dSet.hh"
#include <string.h>

namespace RavlImageN {
  using namespace RavlN;
  
  //! userlevel=Develop
  //: Extrema threshold information.
  
  class ExtremaThresholdC {
  public:
    int thresh;
    int pos;
    int margin;
    UIntT area; // Expected area of region.
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
    IntT *hist; // Histogram of levels.
    IntT total;
    
    ExtremaThresholdC *thresh; // Thresholds
    IntT nThresh; // Number of thresholds.
    
    IntT maxValue;
    IntT minValue;
    Index2dC minat;
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
    SegmentExtremaBaseC(IntT nMinSize,RealT nMinMargin,IntT nlimitMaxValue = 255)
      : stride(0),
        labelAlloc(1),
        origin(0),
	minSize(nMinSize),
        maxSize(0), // This gets modified later.
	minMargin(nMinMargin),
	limitMaxValue(Max(nlimitMaxValue,3)),
	histStack(0)
    {}
    //: Constructor.
    
    ~SegmentExtremaBaseC();
    //: Destructor.
    
    void SetupImage(const IndexRange2dC &rect);
    //: Setup structures for a given image size.
    
    void GenerateRegions();
    //: Generate regions.
    
    UIntT Levels() const 
    { return levels.Size(); }
    //: Get number of levels being considered.
    
    IntT MinSize() const 
    { return minSize; }
    //: Access minimum size.
    
    IntT MaxSize() const 
    { return maxSize; }
    //: Access maximum size.
    
    RealT MinMargin() const
    { return minMargin; }
    //: Access minimum margin
    
    IntT LimitMaxValue() const
    { return limitMaxValue; }
    //: Access value limit
    
    Array1dC<ExtremaChainPixelC *> &LevelSets()
    { return levels; }
    //: Access level set array.
    
  protected:
    void ReallocRegionMap(IntT size);
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
        
    void AddRegion(ExtremaChainPixelC *pix,IntT level);
    //: Add a new region.
    
    void AddPixel(ExtremaChainPixelC *pix,IntT level,ExtremaRegionC *reg);
    //: Add pixel to region.
    
    void MergeRegions(ExtremaChainPixelC *pix,IntT level,ExtremaRegionC **labels,IntT n);
    //: Add pixel to region.

    void Thresholds();
    //: Generate thresholds
    
    void Thresholds2();
    //: Generate thresholds
    
    void PeakDetection(Array1dC<RealT> &real);
    //: Peak detection.
    
    ImageC<ExtremaChainPixelC> pixs;
    Array1dC<ExtremaChainPixelC *> levels;
    SArray1dC<ExtremaRegionC> regionMap;
    UIntT stride; // Image stride.
    UIntT labelAlloc;
    IndexRangeC valueRange; // Range of pixel values.
    ExtremaChainPixelC *origin;
    
    // Paramiters.
    
    IntT minSize;
    IntT maxSize;
    RealT minMargin;
    IntT limitMaxValue; // Maximum image value that will be encountered.

    struct HistStackC {
      IntT max;
      HistStackC *next;
    };
    
    HistStackC *histStack;
    
    IntT *PopHist(IntT level) {
      if(histStack == 0) {
	IntT *val =  new IntT [limitMaxValue+2];
	memset(&(val[level]),0,sizeof(IntT) * ((limitMaxValue+1)-level));
	return val;
      }
      HistStackC *ret = histStack;
      histStack = ret->next;
      IntT *rret = reinterpret_cast<IntT *>(ret);
      IntT clearSize = Max(ret->max+1,3) - level;
      if(clearSize > 0)
	memset(&(rret[level]),0,sizeof(IntT) * clearSize);
      return rret;
    }
    //: Get a new histogram.
    
    void PushHist(IntT *stack,IntT used) {
      register HistStackC *tmp = reinterpret_cast<HistStackC *>(stack);
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
    SegmentExtremaC(IntT minRegionSize,RealT minMargin = 10,IntT nlimitMaxPixelValue = 255)
      : SegmentExtremaBaseC(minRegionSize,minMargin,nlimitMaxPixelValue)
    {}
    //: Constructor.
    //!param:minRegionSize - Minimum region size to detect.
    //!param:minMargin - Threshold for region stability.
    
    DListC<BoundaryC> Apply(const ImageC<PixelT> &img) {
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
    
    DListC<ImageC<IntT> > ApplyMask(const ImageC<PixelT> &img) {
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

    DListC<BoundaryC> Apply(const ImageC<PixelT> &img,const IndexRange2dSetC &roi) {
      SortPixels(img,roi);
      GenerateRegions();
      Thresholds();
      return GrowRegionBoundary(img);
    }
    //: Apply operation to img.
    // Note: The boundaries generated by this function are not ordered. If you require ordered
    // boundries you can use its 'Order()' method to sort them.
    
    DListC<ImageC<IntT> > ApplyMask(const ImageC<PixelT> &img,const IndexRange2dSetC &roi) {
      SortPixels(img,roi);
      GenerateRegions();
      Thresholds();
      return GrowRegionMask(img);
    }
    //: Apply operation to img.
    // Generates labeled images.
    
  protected:
    bool SortPixels(const ImageC<PixelT> &nimg);
    //: Do initial sorting of pixels.
    
    bool SortPixelsByte(const ImageC<PixelT> &nimg);
    //: Do initial sorting of pixels.
    
    bool SortPixels(const ImageC<PixelT> &img,const IndexRange2dSetC &roi);
    //: Build a list from a byte image in regions of interest.

    DListC<BoundaryC> GrowRegionBoundary(const ImageC<PixelT> &img);
    //: Grow regions.
    
    DListC<BoundaryC> GrowRegionBoundary(ExtremaRegionC &region);
    //: Grow regions associated with a extrema.
    
    DListC<ImageC<IntT> > GrowRegionMask(const ImageC<PixelT> &img);
    //: Grow regions.
    
    DListC<ImageC<IntT> > GrowRegionMask(ExtremaRegionC &region);
    //: Grow regions associated with a extrema.
    
    FloodRegionC<PixelT> flood; // Region fill code.    
        
    
  };

  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const ImageC<PixelT> &img) {
    
    SetupImage(img.Frame());
    // Find range of values in image.
    {
      Array2dIterC<PixelT> it(img);
      PixelT lmin = *it;
      PixelT lmax = *it;
      for(;it;it++) {
        if(*it < lmin)
          lmin = *it;
        if(*it > lmax)
          lmax = *it;
      }
      valueRange.Min() = (IntT) lmin;
      valueRange.Max() = (IntT) lmax;
    }
    
    if(valueRange.Max() >= limitMaxValue)
      valueRange.Max() = limitMaxValue-1;
    
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    
    // Check level list allocation.
    
    if(!levels.Range().Contains(valueRange))
      levels = Array1dC<ExtremaChainPixelC *>(valueRange.Expand(2));
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
  bool SegmentExtremaC<PixelT>::SortPixelsByte(const ImageC<PixelT> &img) {
    SetupImage(img.Frame());
    
    // Check level list allocation.
    
    if(levels.Size() == 0)
      levels = Array1dC<ExtremaChainPixelC *>(IndexRangeC(-4,257));
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
    valueRange.Min() = (IntT) lmin;
    valueRange.Max() = (IntT) lmax;
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    return true;
  }
  
  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const ImageC<PixelT> &img,const IndexRange2dSetC &roi) {
    
    SetupImage(img.Frame());
    
    // Find range of values in image.
    bool first = true;
    PixelT lmin = 0;
    PixelT lmax = 0;
    for(DLIterC<IndexRange2dC> rit(roi);rit;rit++) {
      IndexRange2dC rng = *rit;
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
    valueRange.Min() = (IntT) lmin;
    valueRange.Max() = (IntT) lmax;
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
    
    for(DLIterC<IndexRange2dC> rit(roi);rit;rit++) {
      IndexRange2dC rng = *rit;
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
  DListC<BoundaryC> SegmentExtremaC<PixelT>::GrowRegionBoundary(const ImageC<PixelT> &img) {
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
  DListC<ImageC<IntT> > SegmentExtremaC<PixelT>::GrowRegionMask(const ImageC<PixelT> &img) {
    //cerr << "SegmentExtremaBaseC::GrowRegions() \n";
    flood.SetupImage(img);
    
    DListC<ImageC<IntT> > masks;
    if(labelAlloc == 0)
      return masks;
    for(SArray1dIterC<ExtremaRegionC> it(regionMap,labelAlloc-1);it;it++) {
      if(it->nThresh > 0) {
	DListC<ImageC<IntT> > tmp = GrowRegionMask(*it);
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
  DListC<ImageC<IntT> > SegmentExtremaC<PixelT>::GrowRegionMask(ExtremaRegionC &region) {
    DListC<ImageC<IntT> > ret;
    //cerr << " Thresholds=" << region.nThresh << "\n";
    for(int i = 0;i < region.nThresh;i++) {
      ImageC<IntT> mask;
      if(flood.GrowRegion(region.minat,region.thresh[i].thresh,mask,1))
        ret.InsLast(mask);
    }
    return ret;
  }
  

}


#endif
