// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos, based on code by Jiri Matas."
//! example="extrema.cc" 

#pragma once

#include <memory>
#include <cstdint>
#include "Ravl2/Array.hh"
#include "Ravl2/Image/Segmentation/FloodRegion.hh"
#include "Ravl2/IndexRangeSet.hh"

namespace Ravl2 {

  //: Extrema threshold information.
  
  class ExtremaThresholdC
  {
  public:
    int thresh;
    int pos;
    int margin;
    uint32_t area; // Expected area of region.
  };
  
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
    uint32_t *hist; // Histogram of levels.
    uint32_t total;
    
    ExtremaThresholdC *thresh; // Thresholds
    int nThresh; // Number of thresholds.
    
    int maxValue;
    int minValue;
    Index<2> minat;
    ExtremaRegionC *closed;
  };
  
  //: Extremal pixel list.
  
  class ExtremaChainPixelC {
  public:
    ExtremaRegionC *region;
    ExtremaChainPixelC *next;
  };

  //: Common parts to segmenting extrema.
  
  class SegmentExtremaBaseC {
  public:
    using RealT = float;

    //! Constructor.
    //! \param nMinSize - Minimum region size to detect.
    //! \param nMinMargin - Threshold for region stability.
    //! \param nlimitMaxValue - Maximum pixel value.
    SegmentExtremaBaseC(size_t nMinSize,RealT nMinMargin,uint32_t nlimitMaxValue = 255)
      : stride(0),
        labelAlloc(1),
        origin(nullptr),
	minSize(nMinSize),
        maxSize(0), // This gets modified later.
	minMargin(nMinMargin),
	limitMaxValue(std::max(nlimitMaxValue,uint32_t(3)))
    {
      histStack.reserve(100);
    }
    //: Constructor.
    
    ~SegmentExtremaBaseC();
    //: Destructor.
    
    void SetupImage(const IndexRange<2> &rect);
    //: Setup structures for a given image size.
    
    void GenerateRegions();
    //: Generate regions.
    
    [[nodiscard]] unsigned Levels() const
    { return unsigned(levels.range().size()); }
    //: Get number of levels being considered.

    [[nodiscard]] auto MinSize() const
    { return minSize; }
    //: Access minimum size.

    [[nodiscard]] auto MaxSize() const
    { return maxSize; }
    //: Access maximum size.

    [[nodiscard]] auto MinMargin() const
    { return minMargin; }
    //: Access minimum margin

    [[nodiscard]] auto LimitMaxValue() const
    { return limitMaxValue; }
    //: Access value limit

    [[nodiscard]] Array<ExtremaChainPixelC *,1> &LevelSets()
    { return levels; }
    //: Access level set array.
    
  protected:
    //! Reallocate the current region set, free any memory used.
    void ReallocRegionMap(size_t newSize);

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
    
    void PeakDetection(Array<RealT,1> &real);
    //: Peak detection.
    
    Array<ExtremaChainPixelC,2> pixs;
    Array<ExtremaChainPixelC *,1> levels;
    std::vector<ExtremaRegionC> regionMap;
    int stride = 0; // Image stride.
    unsigned labelAlloc = 0;
    IndexRange<1> valueRange; // Range of pixel values.
    ExtremaChainPixelC *origin = nullptr; // Origin of image.
    
    // Parameters.
    
    size_t minSize = 0;
    size_t maxSize = 0;
    RealT minMargin = 0;
    uint32_t limitMaxValue = 0; // Maximum image value that will be encountered.

    struct HistStackC {
      HistStackC(int nmax,uint32_t *nstack)
         : max(nmax),
           data(nstack)
      {}
      int max = 0;
      uint32_t *data = nullptr;
    };
    
    std::vector<HistStackC> histStack;

    uint32_t *PopHist(int level) {
      assert(level >= 0);
      if(histStack.empty()) {
        auto *val = new uint32_t [limitMaxValue+2];
	memset(&(val[level]),0,sizeof(uint32_t) * ((limitMaxValue+1)-size_t(level)));
	return val;
      }
      HistStackC &ret = histStack.back();
      uint32_t *rret = ret.data;
      int clearSize = std::max(ret.max+1,3) - level;
      histStack.pop_back();
      if(clearSize > 0)
	memset(&(rret[level]),0,sizeof(uint32_t) * size_t(clearSize));
      return rret;
    }
    //: Get a new histogram.
    
    void PushHist(uint32_t *stack,int used)
    {
      histStack.emplace_back(used,stack);
    }
    //: Push unused histogram
  };
  
  
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
    SegmentExtremaC(int minRegionSize,RealT theMinMargin = 10,int nlimitMaxPixelValue = 255)
      : SegmentExtremaBaseC(minRegionSize,theMinMargin,nlimitMaxPixelValue)
    {}
    //: Constructor.
    //!param:minRegionSize - Minimum region size to detect.
    //!param:minMargin - Threshold for region stability.
    
    std::vector<BoundaryC> Apply(const Array<PixelT,2> &img) {
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
    
    std::vector<Array<int,2> > ApplyMask(const Array<PixelT,2> &img) {
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

    std::vector<BoundaryC> Apply(const Array<PixelT,2> &img,const IndexRangeSet<2> &roi) {
      SortPixels(img,roi);
      GenerateRegions();
      Thresholds();
      return GrowRegionBoundary(img);
    }
    //: Apply operation to img.
    // Note: The boundaries generated by this function are not ordered. If you require ordered
    // boundries you can use its 'Order()' method to sort them.
    
    std::vector<Array<int,2> > ApplyMask(const Array<PixelT,2> &img,const IndexRangeSet<2> &roi) {
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
    
    bool SortPixels(const Array<PixelT,2> &img,const IndexRangeSet<2> &roi);
    //: Build a list from a byte image in regions of interest.

    std::vector<BoundaryC> GrowRegionBoundary(const Array<PixelT,2> &img);
    //: Grow regions.
    
    std::vector<BoundaryC> GrowRegionBoundary(ExtremaRegionC &region);
    //: Grow regions associated with a extrema.
    
    std::vector<Array<int,2> > GrowRegionMask(const Array<PixelT,2> &img);
    //: Grow regions.
    
    std::vector<Array<int,2> > GrowRegionMask(ExtremaRegionC &region);
    //: Grow regions associated with a extrema.
    
    FloodRegionC<PixelT> flood; // Region fill code.    
        
    
  };

  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const Array<PixelT,2> &img) {
    
    SetupImage(img.range());
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
      valueRange.min() = int(lmin);
      valueRange.max() = int(lmax);
    }
    
    if(valueRange.max() >= limitMaxValue)
      valueRange.max() = limitMaxValue-1;
    
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    
    // Check level list allocation.
    
    if(!levels.range().contains(valueRange))
      levels = Array<ExtremaChainPixelC *,1>(valueRange.expand(2));
    levels.fill(0);
    
    // Sort pixels into appropriate lists.
    
    for(auto it = begin(img,clip(pixs,img.range()));it;it++) {
      it.template data<1>().region = 0;
      PixelT val = it.template data<0>();
      if(val > limitMaxValue) {
        continue;
      }
      ExtremaChainPixelC * &tmp = levels[val]; 
      it.template data<1>().next = tmp;
      tmp = &it.template data<1>();
    }
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    
    return true;
  }
  
  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixelsByte(const Array<PixelT,2> &img)
  {
    SetupImage(img.range());
    
    // Check level list allocation.
    
    if(levels.range().size() == 0)
      levels = Array<ExtremaChainPixelC *,1>(IndexRange<1>(-4,257));
    memset(&(levels[levels.range().min()]),0,levels.range().size() * sizeof(ExtremaChainPixelC *));
    
    // Sort pixels into appropriate lists.
    
    auto it = begin(img,clip(pixs,img.range()));

    PixelT lmin = it.template data<0>();
    PixelT lmax = it.template data<0>();
    if(it.template data<0>() > limitMaxValue) {
      lmax = limitMaxValue;
      lmin = limitMaxValue;
    }
    if(limitMaxValue >= 255) {
      for(;it;it++) {
        it.template data<1>().region = 0;
        PixelT val = it.template data<0>();
        if(val < lmin) lmin = val;
        if(val > lmax) lmax = val;
        ExtremaChainPixelC * &tmp = levels[val]; 
        it.template data<1>().next = tmp;
        tmp = &it.template data<1>();
      }
    } else {
      for(;it;it++) {
        it.template data<1>().region = 0;
        PixelT val = it.template data<0>();
        if(val > limitMaxValue)
          continue;
        if(val < lmin) lmin = val;
        if(val > lmax) lmax = val;
        ExtremaChainPixelC * &tmp = levels[val]; 
        it.template data<1>().next = tmp;
        tmp = &it.template data<1>();
      }
    }
    valueRange.min() = int(lmin);
    valueRange.max() = int(lmax);
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    return true;
  }
  
  
  template<class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const Array<PixelT,2> &img,const IndexRangeSet<2> &roi) {
    
    SetupImage(img.range());
    
    // Find range of values in image.
    bool first = true;
    PixelT lmin = 0;
    PixelT lmax = 0;
    for(auto rit : roi) {
      IndexRange<2> rng = rit;
      rng.clipBy(img.range());
      if(rng.area() < 1)
	continue;
      auto it = clip(img,rng).begin();
      if(first) {
        lmin = *it;
        lmax = *it;
        first = false;
      }
      for(;it.valid;++it) {
	if(*it < lmin)
	  lmin = *it;
	if(*it > lmax)
	  lmax = *it;
      }
    }
    valueRange.min() = int(lmin);
    valueRange.max() = int(lmax);
    if(valueRange.max() > limitMaxValue)
      valueRange.max() = limitMaxValue;
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    // Check level list allocation.
    
    if(!levels.range().contains(valueRange))
      levels = Array<ExtremaChainPixelC *,1>(valueRange.expand(2));
    levels.fill(0);
    
    // Clear chain image.
    
    for(auto &it : clip(pixs,img.range())) {
      it.Data().region = 0;
      it.Data().next = 0;
    }
    
    // Sort pixels into appropriate lists.
    
    for(auto rit : roi) {
      IndexRange<2> rng = rit;
      rng.clipBy(img.range());
      if(rng.area() < 1)
	continue;
      for(auto it = begin(clip(img,rng),clip(pixs,rng));it;it++) {
	PixelT val = it.template data<0>();
	if(val > limitMaxValue)
	  continue;
	ExtremaChainPixelC * &tmp = levels[val]; 
	it.template data<1>().next = tmp;
	tmp = &it.template  data<1>();
      }
    }
    return true;
  }

  
  template<class PixelT>
  std::vector<BoundaryC> SegmentExtremaC<PixelT>::GrowRegionBoundary(const Array<PixelT,2> &img)
  {
    //cerr << "SegmentExtremaBaseC::GrowRegions() \n";
    flood.SetupImage(img);
    
    std::vector<BoundaryC> bounds;
    if(labelAlloc == 0)
      return bounds;
    auto end = regionMap.begin();
    end += labelAlloc;
    for(auto &it = regionMap.begin();it != end;++it) {
      if(it->nThresh > 0) {
	bounds.push_back(GrowRegionBoundary(*it));
      }
      if(it->thresh != 0) {
	delete [] it->thresh;
	it->thresh = 0;
      }
    }
    
    return bounds;
  }
  
  template<class PixelT>
  std::vector<BoundaryC> SegmentExtremaC<PixelT>::GrowRegionBoundary(ExtremaRegionC &region) {
    std::vector<BoundaryC> ret;
    for(int i = 0;i < region.nThresh;i++) {
      BoundaryC boundary;
      if(flood.GrowRegion(region.minat,region.thresh[i].thresh,boundary))
	ret.push_back(boundary);
    }
    return ret;
  }
  
  template<class PixelT>
  std::vector<Array<int,2> > SegmentExtremaC<PixelT>::GrowRegionMask(const Array<PixelT,2> &img) {
    //cerr << "SegmentExtremaBaseC::GrowRegions() \n";
    flood.SetupImage(img);
    
    std::vector<Array<int,2> > masks;
    if(labelAlloc == 0)
      return masks;
    auto end = regionMap.begin() + labelAlloc;
    for(auto it = regionMap.begin();it != end;++it) {
      if(it->nThresh > 0) {
        masks.push_back(GrowRegionMask(*it));
      }
      if(it->thresh != 0) {
        delete [] it->thresh;
        it->thresh = 0;
      }
    }

    return masks;
  }
  
  template<class PixelT>
  std::vector<Array<int,2> > SegmentExtremaC<PixelT>::GrowRegionMask(ExtremaRegionC &region) {
    std::vector<Array<int,2> > ret;
    //cerr << " Thresholds=" << region.nThresh << "\n";
    for(int i = 0;i < region.nThresh;i++) {
      Array<int,2> mask;
      if(flood.GrowRegion(region.minat,region.thresh[i].thresh,mask,1))
        ret.push_back(mask);
    }
    return ret;
  }
  

}

