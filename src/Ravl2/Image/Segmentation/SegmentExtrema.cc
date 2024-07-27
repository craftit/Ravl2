// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos, based on code by Jiri Matas."

#include "Ravl2/Image/Segmentation/SegmentExtrema.hh"
#include "Ravl2/Image/DrawFrame.hh"
#include "Ravl2/ArrayIterZip.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //: Destructor.
  
  SegmentExtremaBaseC::~SegmentExtremaBaseC() {
    ReallocRegionMap(0);
    
    // Free histogram stack.
    while(!histStack.empty()) {
      delete [] histStack.back().data;
      histStack.pop_back();
    }
  }

  //: Delete the current region set, free any memory used.
  
  void SegmentExtremaBaseC::ReallocRegionMap(size_t newSize) {
    if(newSize == 0)
      regionMap.clear();
    else 
      regionMap.resize(newSize);
  }
  
  //: Setup structures for a given image size.
  
  void SegmentExtremaBaseC::SetupImage(const IndexRange<2> &rect) {
    IndexRange<2> imgRect = rect.expand(1);
    labelAlloc = 0; // Reset region allocation counter.
    if(imgRect == pixs.range())
      return ; // Done already.
    
    // Allocate ExtremaChainPixelC image.
    pixs = Array<ExtremaChainPixelC,2>(imgRect);
    origin = &(pixs[rect.min()]);
    stride = pixs.stride(1);
    
    // Put a frame of zero labels around the edge.
    ExtremaChainPixelC zeroPix{nullptr,nullptr};
    DrawFrame(pixs,zeroPix,imgRect);

    assert(!rect.empty());
    // Create region map.
    ReallocRegionMap(size_t(rect.area()/2));
    
    // ...
    if(maxSize == 0)
      maxSize = size_t(rect.area());
  }
  
  //: Find matching label.
  // This looks at the region assosciated with a given pixel, 
  // and resolves any merged regions to the current label.
  
  inline
  ExtremaRegionC * SegmentExtremaBaseC::FindLabel(ExtremaChainPixelC *cp) {
    ExtremaRegionC *lab = cp->region;
    if(lab == nullptr || lab->merge == nullptr) return lab;
    ExtremaRegionC *at = lab->merge;
    while(at->merge != nullptr)
      at = at->merge;
    // Relabel mappings so its faster next time.
    do {
      ExtremaRegionC *tmp = lab;
      lab = lab->merge;
      tmp->merge = at;
    } while(lab != at);
    cp->region = at; // This makes things run slightly faster, but we loose info about the pixel.
    return at;
  }

  //: Find the number of distinct labels around the pixel 'pix'
  // This eliminates duplicate values by comparing each result to
  // those obtrained previously.
  // Puts the results into 'labelArray' which must be at least 4 labels long.
  // The total number of labels found is returned.
  
  inline
  int SegmentExtremaBaseC::ConnectedLabels(ExtremaChainPixelC *pix,ExtremaRegionC **labelArray) {
    //cerr << "SegmentExtremaBaseC::ConnectedLabels(), Pix=" << ((void *) pix) << "\n";
    int n = 0;
    ExtremaRegionC *l1 = FindLabel(pix + 1);
    if (l1!= nullptr )                               { labelArray[n++]=l1; }
    ExtremaRegionC *l2 = FindLabel(pix + stride);
    if (l2!= nullptr && l2!=l1)                      { labelArray[n++]=l2; }
    ExtremaRegionC *l3 = FindLabel(pix - 1);
    if (l3!= nullptr && l3!=l1 && l3!=l2)            { labelArray[n++]=l3; }
    ExtremaRegionC *l4 = FindLabel(pix - stride);
    if (l4!= nullptr && l4!=l1 && l4!=l2 && l4!=l3)  { labelArray[n++]=l4; }
    return n;
  }
  
  //: Add a new region.
  inline
  void SegmentExtremaBaseC::AddRegion(ExtremaChainPixelC *pix,int level) {
    ExtremaRegionC &region = regionMap[labelAlloc++];
    pix->region = &region;
    //cerr << "SegmentExtremaBaseC::AddRegion(), Pix=" << (void *) pix << " Region=" << (void *) &region << "\n";
    region.total = 0;
    region.merge = nullptr; //&region;
    int nlevel = level+1; // Don't need to clear this level as its going to be set anyway
    if(region.hist == nullptr) {
      region.hist = PopHist(nlevel);
    } else {
      int clearSize = (region.maxValue + 1) - nlevel;
      if(clearSize > 0)
	memset(&(region.hist[nlevel]),0,size_t(clearSize) * sizeof(int));
    }
#if 0
    // Check the histogram is clear.
    for(int i = level;i <= valueRange.max();i++) {
      if(region.hist[i] != 0) 
        cerr << "Non zero at " << i << " Max=" << valueRange.max() <<  "\n";
      RavlAlwaysAssert(region.hist[i] == 0);
    }
#endif

    auto offset = pix - origin;
    region.minat = toIndex((offset / stride)+1,(offset % stride)+1) + pixs.range().min();
    region.minValue = level;
    region.maxValue = valueRange.max();
    region.hist[level] = 1;
    region.total = 1;
    region.thresh = 0;
    region.closed = 0;
  }
  
  //: Add pixel to region.
  
  inline
  void SegmentExtremaBaseC::AddPixel(ExtremaChainPixelC *pix,int level,ExtremaRegionC *reg) {
    reg->hist[level]++;
    reg->total++;
    pix->region = reg;
  }

  //: Add pixel to region.
  
  inline
  void SegmentExtremaBaseC::MergeRegions(ExtremaChainPixelC *pix,int level,ExtremaRegionC **labels,int n) {
    auto maxValue = labels[0]->total;
    ExtremaRegionC *max = labels[0];
    
    // Find largest region.
    int i;
    for(i = 1;i < n;i++) {
      auto tot = labels[i]->total;
      if(maxValue < tot) {
	max = labels[i];
	maxValue = tot;
      }
    }
    
    // Merge regions, and finalise
    for(i = 0;i < n;i++) {
      if(labels[i] == max)
	continue;
      ExtremaRegionC &oldr = *labels[i];
      oldr.maxValue = level;
      oldr.merge = max;
      oldr.closed = max;
      
      // If we don't need the histogram, put it back in a pool
      if(oldr.hist != nullptr && (oldr.total < minSize || (RealT(level - oldr.minValue) < minMargin) )) {
	PushHist(oldr.hist,level);
	oldr.hist = nullptr;
      }
      
      max->total += oldr.total;
      max->hist[level] += oldr.total;
    }
    AddPixel(pix,level,max);
  }
  
  //: Generate regions.
  
  void SegmentExtremaBaseC::GenerateRegions() {
    ExtremaChainPixelC *at;
    int n, clevel = levels.range().min();
    ExtremaRegionC *labels[6];
    
    // For each grey level value in image.
    for(auto lit : levels) {
      //ONDEBUG(std::cerr << "Level=" << clevel << " \n");
      
      // Go through linked list of pixels at the current brightness level.
      for(at = lit;at != nullptr;at = at->next) {
	// Got a standard pixel.
	//ONDEBUG(std::cerr << "Pixel=" << (void *) at << " Region=" << at->region << "\n");

        // Look at the region membership of the pixels surrounding the new
        // one.  n is the number of different regions found.
	n = ConnectedLabels(at,labels);
        
	switch(n) {
	case 0: // Add a new region ?
          AddRegion(at,clevel);  
          break;
          
	case 1: // 1 adjacent region to this pixel.
          AddPixel(at,clevel,labels[0]); 
          break;
          
	default: // 2 or more adjacent regions to merge.
          MergeRegions(at,clevel,labels,n); 
          break;
	}
      }
      clevel++;
    }
    //cerr << "Regions labeled = " << labelAlloc << "\n";
  }
  
  //: Generate thresholds
  
  void SegmentExtremaBaseC::Thresholds()
  {
    //cerr << "SegmentExtremaBaseC::Thresholds() ********************************************** \n";
    std::vector<ExtremaThresholdC> thresh(size_t(limitMaxValue + 2));
    size_t nthresh = 0;
    assert(limitMaxValue + 2 < std::numeric_limits<int>::max());
    Array<uint32_t,1> chist(IndexRange<1>(0,int(limitMaxValue + 2)));
    chist.fill(0);

    auto end = regionMap.begin() + labelAlloc;

    for(auto it = regionMap.begin();it != end;++it) {
      if(it->total < minSize || RealT(it->maxValue - it->minValue) < minMargin) {
	it->nThresh = 0;// Ingore these regions.
	continue; // Not enough levels in the region.
      }
      // Build the cumulative histogram.
      int maxValue = it->maxValue;
      int minValue = it->minValue;
      uint32_t sum = 0;
      int i;
      
      ONDEBUG(std::cerr << "Hist= " << it->minValue << " :");
      for(i = minValue;i <= maxValue;i++) {
	sum += it->hist[i];
	chist[i] = sum;
	ONDEBUG(std::cerr << " " << it->hist[i]);
      }
      chist[maxValue] = sum;
      
      ONDEBUG(std::cerr << "  Closed=" << (it->closed != 0)<< "\n");
      int up;
      // look for threshold that guarantee area bigger than minSize.
      for(i = minValue; i <= maxValue;i++)
	if(chist[i] >= minSize) break; 
      
      // Find thresholds.
      nthresh = 0;
      ONDEBUG(std::cerr << "Min=" << minValue << " Max=" << maxValue << " Init=" << i << " MaxSize=" << maxSize << "\n");
      size_t lastThresh = 0;
      for(up=i+1; up < maxValue && i < maxValue; i++) {
	auto area_i = chist[i];
	if(area_i > maxSize) {
	  //cerr << "Size limit reached. \n";
	  break; // Quit if area is too large.
	}
        
	auto half_perimeter_i = int_round<double,decltype(area_i)>(2.1 * std::sqrt(double(area_i))) + area_i;
        if(half_perimeter_i == lastThresh)
          continue; // If the thresholds are the same, the next margin will only be shorter.
        lastThresh = half_perimeter_i;
	while(up <= maxValue && chist[up] < half_perimeter_i)
	  up++;
	
	int margin = up - i;
	//ONDEBUG(std::cerr << " Margin=" << margin << "\n");
	if(RealT(margin) > minMargin) { // && margin > prevMargin
	  ExtremaThresholdC &et = thresh[nthresh++];
	  et.pos = i;
 	  et.margin = margin;
	  et.thresh = i + margin/2;
	  // cerr << " Threshold=" << et.thresh << " " << chist[et.thresh] << "\n";
	}
      }
      
      if(it->closed == 0) { // Is region closed ?
	ExtremaThresholdC &et = thresh[nthresh++];
	et.pos = maxValue-1;
	int margin = up - i;
	et.margin = margin;
	et.thresh = maxValue-1;	
      }
      //ONDEBUG(std::cerr << "NThresh=" <<  nthresh << "\n");
      ExtremaThresholdC *newthresh = new ExtremaThresholdC[size_t(nthresh)];
      int nt = 0;
      
      unsigned lastSize = 0;
      unsigned lastInd = 0;
      for(unsigned j = 0;j < nthresh;j++) {
	auto size = chist[thresh[j].pos];
	if((lastSize * 1.15) > size) { // Is size only slighly different ?
	  if(thresh[j].margin > thresh[lastInd].margin) {
	    newthresh[nt-1] = thresh[j]; // Move threshold if margin is bigger.
	    newthresh[nt-1].area = chist[thresh[j].thresh];
            lastSize = size;
          }
	  continue; // Reject it, not enough difference.
	}
	newthresh[nt] = thresh[j];
        newthresh[nt].area = chist[newthresh[nt].thresh];
        nt++;
	lastSize = size;
	lastInd = j;
      }
      
      if(nt > 0) {
	it->thresh = newthresh;
	it->nThresh = nt;
      } else {
	it->nThresh = 0;
	delete [] newthresh;
      }
      //cerr << "Thresholds=" << nthresh << " Kept=" << it->nThresh << "\n";
    } // for(SArray1dIterC<ExtremaRegionC> it(...
    //cerr << "SegmentExtremaBaseC::Thresholds() Interesting regions=" << regions <<" \n";
  }  
}
