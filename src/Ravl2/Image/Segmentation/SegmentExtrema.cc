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

  SegmentExtremaBaseC::~SegmentExtremaBaseC()
  {
    // Free histogram stack.
    while(!histStack.empty()) {
      delete[] histStack.back().data;
      histStack.pop_back();
    }
  }

  //: Setup structures for a given image size.

  void SegmentExtremaBaseC::SetupImage(const IndexRange<2> &rect)
  {
    IndexRange<2> imgRect = rect.expand(1);
    labelAlloc = 0;// Reset region allocation counter.
    if(imgRect == pixs.range())
      return;// Done already.

    // Allocate ExtremaChainPixelC image.
    pixs = Array<ExtremaChainPixelC, 2>(imgRect);
    origin = &(pixs[rect.min()]);
    stride = pixs.stride(0);

    // Put a frame of zero labels around the edge.
    ExtremaChainPixelC zeroPix {nullptr, nullptr};
    DrawFrame(pixs, zeroPix, imgRect);

    assert(!rect.empty());
    // Create region map.
    size_t newSize = size_t(rect.area() / 2);
    if(regionMap.size() < newSize)
      regionMap.resize(newSize);

    // ...
    if(maxSize == 0)
      maxSize = size_t(rect.area());
  }

  //: Find matching label.
  // This looks at the region assosciated with a given pixel,
  // and resolves any merged regions to the current label.

  inline ExtremaRegionC *SegmentExtremaBaseC::FindLabel(ExtremaChainPixelC *cp)
  {
    ExtremaRegionC *lab = cp->region;
    if(lab == nullptr || lab->merge == nullptr)
      return lab;
    ExtremaRegionC *at = lab->merge;
    while(at->merge != nullptr)
      at = at->merge;
    // Relabel mappings so its faster next time.
    do {
      ExtremaRegionC *tmp = lab;
      lab = lab->merge;
      tmp->merge = at;
    } while(lab != at);
    cp->region = at;// This makes things run slightly faster, but we loose info about the pixel.
    return at;
  }

  //: Find the number of distinct labels around the pixel 'pix'
  // This eliminates duplicate values by comparing each result to
  // those obtained previously.
  // Puts the results into 'labelArray' which must be at least 4 labels long.
  // The total number of labels found is returned.

  inline int SegmentExtremaBaseC::ConnectedLabels(ExtremaChainPixelC *pix, ExtremaRegionC **labelArray)
  {
    //cerr << "SegmentExtremaBaseC::ConnectedLabels(), Pix=" << ((void *) pix) << "\n";
    int n = 0;
    ExtremaRegionC *l1 = FindLabel(pix + 1);
    if(l1 != nullptr) {
      labelArray[n++] = l1;
    }
    ExtremaRegionC *l2 = FindLabel(pix + stride);
    if(l2 != nullptr && l2 != l1) {
      labelArray[n++] = l2;
    }
    ExtremaRegionC *l3 = FindLabel(pix - 1);
    if(l3 != nullptr && l3 != l1 && l3 != l2) {
      labelArray[n++] = l3;
    }
    ExtremaRegionC *l4 = FindLabel(pix - stride);
    if(l4 != nullptr && l4 != l1 && l4 != l2 && l4 != l3) {
      labelArray[n++] = l4;
    }
    return n;
  }

  inline void SegmentExtremaBaseC::AddRegion(ExtremaChainPixelC *pix, int level)
  {
    ExtremaRegionC &region = regionMap[labelAlloc++];
    pix->region = &region;
    region.total = 0;
    region.merge = nullptr;//&region;
    int nlevel = level + 1;// Don't need to clear this level as its going to be set anyway
    if(region.hist == nullptr) {
      region.hist = PopHist(nlevel);
    } else {
      int clearSize = (region.maxValue + 1) - nlevel;
      if(clearSize > 0) {
        memset(&(region.hist[nlevel]), 0, size_t(clearSize) * sizeof(int));
      }
    }
#ifndef NDEBUG
    // Check the histogram is clear.
    for(int i = nlevel; i <= valueRange.max(); i++) {
      if(region.hist[i] != 0) {
        SPDLOG_WARN("Non zero at {} Max={}", i, valueRange.max());
        abort();
      }
    }
#endif

    auto offset = pix - origin;
    region.minat = toIndex((offset / stride) + 1, (offset % stride) + 1) + pixs.range().min();
    ONDEBUG(SPDLOG_INFO("Region minat={} level={} Pix={} Offset={} Stride={} origin={}", region.minat, level, static_cast<void *>(pix), offset, stride, static_cast<void *>(origin)));
    RavlAssert(&pixs[region.minat] == pix);
    region.minValue = level;
    region.maxValue = valueRange.max();
    region.hist[level] = 1;
    region.total = 1;
    region.thresh.clear();
    region.closed = nullptr;
  }

  //: Add pixel to region.

  inline void SegmentExtremaBaseC::AddPixel(ExtremaChainPixelC *pix, int level, ExtremaRegionC *reg)
  {
    reg->hist[level]++;
    reg->total++;
    pix->region = reg;
  }

  //: Add pixel to region.

  inline void SegmentExtremaBaseC::MergeRegions(ExtremaChainPixelC *pix, int level, ExtremaRegionC **labels, int n)
  {
    ONDEBUG(SPDLOG_INFO("MergeRegions() Pix={} Level={} N={}", static_cast<void *>(pix), level, n));
    auto maxValue = labels[0]->total;
    ExtremaRegionC *max = labels[0];

    // Find largest region.
    int i;
    for(i = 1; i < n; i++) {
      auto tot = labels[i]->total;
      if(maxValue < tot) {
        max = labels[i];
        maxValue = tot;
      }
    }

    // Merge regions, and finalise
    for(i = 0; i < n; i++) {
      if(labels[i] == max)
        continue;
      ExtremaRegionC &oldr = *labels[i];
      oldr.maxValue = level;
      oldr.merge = max;
      oldr.closed = max;

      // If we don't need the histogram, put it back in a pool
      if(oldr.hist != nullptr && (oldr.total < minSize || ((level - oldr.minValue) < minMargin))) {
        PushHist(oldr.hist, level);
        oldr.hist = nullptr;
      }

      max->total += oldr.total;
      max->hist[level] += oldr.total;
    }
    AddPixel(pix, level, max);
  }

  //: Generate regions.

  void SegmentExtremaBaseC::GenerateRegions()
  {
    ExtremaChainPixelC *at;
    int clevel = valueRange.min();
    ExtremaRegionC *labels[6];

    // For each grey level value in image.
    for(auto lit : clip(levels, valueRange)) {
      //ONDEBUG(SPDLOG_INFO("Level={}", clevel));

      // Go through linked list of pixels at the current brightness level.
      for(at = lit; at != nullptr; at = at->next) {
        // Got a standard pixel.
        // ONDEBUG(SPDLOG_INFO("Pixel={} Region={}", static_cast<void *>(at), static_cast<void *>(at->region)));

        // Look at the region membership of the pixels surrounding the new
        // one.  n is the number of different regions found.
        int n = ConnectedLabels(at, labels);

        switch(n) {
          case 0:// Add a new region ?
            AddRegion(at, clevel);
            break;

          case 1:// 1 adjacent region to this pixel.
            AddPixel(at, clevel, labels[0]);
            break;

          default:// 2 or more adjacent regions to merge.
            MergeRegions(at, clevel, labels, n);
            break;
        }
      }
      clevel++;
    }
    ONDEBUG(SPDLOG_INFO("Regions labeled={}", labelAlloc));
  }

  //: Generate thresholds

  void SegmentExtremaBaseC::Thresholds()
  {
    ONDEBUG(SPDLOG_INFO("Computing thresholds **********************************************"));
    std::vector<ExtremaThresholdC> thresh(size_t(limitMaxValue + 2));
    size_t nthresh = 0;
    assert(limitMaxValue + 2 < std::numeric_limits<int>::max());
    Array<uint32_t, 1> chist(IndexRange<1>(0, limitMaxValue + 2), 0);

    auto end = regionMap.begin() + labelAlloc;

    for(auto it = regionMap.begin(); it != end; ++it) {
      if(it->total < minSize || (it->maxValue - it->minValue) < minMargin) {
        it->thresh.clear();// Ignore these regions.
        continue;       // Not enough levels in the region.
      }
      // Build the cumulative histogram.
      int maxValue = it->maxValue;
      int minValue = it->minValue;
      uint32_t sum = 0;

      //ONDEBUG(std::cerr << "Hist= " << it->minValue << " :");
      ONDEBUG(std::string histStr = fmt::format("Hist= {}->{} :", it->minValue, it->maxValue));
      // Build cumulative histogram.
      for(int i = minValue; i <= maxValue; i++) {
        sum += it->hist[i];
        chist[i] = sum;
        ONDEBUG(histStr += fmt::format(" {}", it->hist[i]));
      }
      assert(chist[maxValue] == sum);
      ONDEBUG(SPDLOG_INFO("{}", histStr));

      //ONDEBUG(std::cerr << "  Closed=" << (it->closed != 0)<< "\n");
      ONDEBUG(SPDLOG_INFO("Region={} Min={} Max={} Total={} Closed={}", it - regionMap.begin(), minValue, maxValue, sum, it->closed != nullptr));
      int up;
      // look for threshold that guarantee area bigger than minSize.
      int i;
      for(i = minValue; i <= maxValue; i++) {
        if(chist[i] >= minSize)
          break;
      }

      // Find thresholds.
      nthresh = 0;
      ONDEBUG(SPDLOG_INFO("Min={} Max={} Init={} MaxSize={}", minValue, maxValue, i, maxSize));
      size_t lastThresh = 0;
      for(up = i + 1; up < maxValue && i < maxValue; i++) {
        auto area_i = chist[i];
        if(area_i > maxSize) {
          ONDEBUG(SPDLOG_INFO("Size limit reached. "));
          break;// Quit if area is too large.
        }

        auto half_perimeter_i = intRound<double, decltype(area_i)>(2.1 * std::sqrt(double(area_i))) + area_i;
        if(half_perimeter_i == lastThresh)
          continue;// If the thresholds are the same, the next margin will only be shorter.
        lastThresh = half_perimeter_i;
        while(up <= maxValue && chist[up] < half_perimeter_i) {
          up++;
        }

        int margin = up - i;
        ONDEBUG(SPDLOG_INFO("Margin={}", margin));
        if(margin > minMargin) {// && margin > prevMargin
          ExtremaThresholdC &et = thresh[nthresh++];
          et.pos = i;
          et.margin = margin;
          et.thresh = i + margin / 2;
          ONDEBUG(SPDLOG_INFO("Threshold={} Area={}", et.thresh, chist[et.thresh]));
        }
      }

      if(it->closed == nullptr) {// Is region closed ?
        ExtremaThresholdC &et = thresh[nthresh++];
        et.pos = maxValue - 1;
        et.margin = up - i;
        et.thresh = maxValue - 1;
      }
      ONDEBUG(SPDLOG_INFO("Thresholds={} Kept={}", nthresh, it->nThresh));
      //std::vector<ExtremaThresholdC> newthresh = new std::vector<ExtremaThresholdC>(nthresh);
      std::vector<ExtremaThresholdC> newthresh;
      newthresh.reserve(nthresh);

      unsigned lastSize = 0;
      unsigned lastInd = 0;
      for(unsigned j = 0; j < nthresh; j++) {
        auto size = chist[thresh[j].pos];
        if((lastSize * 1.15) > size) {// Is size only slightly different ?
          if(thresh[j].margin > thresh[lastInd].margin) {
            newthresh.back() = thresh[j];// Move threshold if margin is bigger.
            newthresh.back().area = chist[thresh[j].thresh];
            lastSize = size;
          }
          ONDEBUG(SPDLOG_INFO("Rejecting threshold={} LastArea={} Area={}", thresh[j].thresh, size, chist[thresh[j].thresh]));
          continue;// Reject it, not enough difference.
        }
        newthresh.push_back(thresh[j]);
        newthresh.back().area = chist[thresh[j].thresh];
        lastSize = size;
        lastInd = j;
      }

      if(!newthresh.empty()) {
        it->thresh = std::move(newthresh);
      } else {
        it->thresh.clear();
      }
      ONDEBUG(SPDLOG_INFO("Thresholds={} Kept={}", nthresh, it->nThresh));
    }// for(SArray1dIterC<ExtremaRegionC> it(...
    //cerr << "SegmentExtremaBaseC::Thresholds() Interesting regions=" << regions <<" \n";
  }
}// namespace Ravl2
