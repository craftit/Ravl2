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
    mLabelAlloc = 0;// Reset region allocation counter.
    if(imgRect == mPixs.range())
      return;// Done already.

    // Allocate ExtremaChainPixelC image.
    mPixs = Array<ExtremaChainPixelC, 2>(imgRect);
    mOrigin = &(mPixs[rect.min()]);
    stride = mPixs.stride(0);

    // Put a frame of zero labels around the edge.
    ExtremaChainPixelC zeroPix {nullptr, nullptr};
    DrawFrame(mPixs, zeroPix, imgRect);

    assert(!rect.empty());
    // Create region map.
    size_t newSize = size_t(rect.area() / 2);
    if(mRegionMap.size() < newSize)
      mRegionMap.resize(newSize);

    // ...
    if(mMaxSize == 0)
      mMaxSize = size_t(rect.area());
  }

  //: Find matching label.
  // This looks at the region assosciated with a given pixel,
  // and resolves any merged regions to the current label.

  inline ExtremaRegionC *SegmentExtremaBaseC::FindLabel(ExtremaChainPixelC *cp)
  {
    ExtremaRegionC *lab = cp->mRegion;
    if(lab == nullptr || lab->mMerge == nullptr)
      return lab;
    ExtremaRegionC *at = lab->mMerge;
    while(at->mMerge != nullptr)
      at = at->mMerge;
    // Relabel mappings so its faster next time.
    do {
      ExtremaRegionC *tmp = lab;
      lab = lab->mMerge;
      tmp->mMerge = at;
    } while(lab != at);
    cp->mRegion = at;// This makes things run slightly faster, but we loose info about the pixel.
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
    ExtremaRegionC &region = mRegionMap[mLabelAlloc++];
    pix->mRegion = &region;
    region.mTotal = 0;
    region.mMerge = nullptr;//&region;
    int nlevel = level + 1;// Don't need to clear this level as its going to be set anyway
    if(region.mHist == nullptr) {
      region.mHist = PopHist(nlevel);
    } else {
      int clearSize = (region.mMaxValue + 1) - nlevel;
      if(clearSize > 0) {
        memset(&(region.mHist[nlevel]), 0, size_t(clearSize) * sizeof(int));
      }
    }
#ifndef NDEBUG
    // Check the histogram is clear.
    for(int i = nlevel; i <= mValueRange.max(); i++) {
      if(region.mHist[i] != 0) {
        SPDLOG_WARN("Non zero at {} Max={}", i, mValueRange.max());
        abort();
      }
    }
#endif

    auto offset = pix - mOrigin;
    region.mMinat = toIndex((offset / stride) + 1, (offset % stride) + 1) + mPixs.range().min();
    ONDEBUG(SPDLOG_INFO("Region mMinat={} level={} Pix={} Offset={} Stride={} origin={}", region.mMinat, level, static_cast<void *>(pix), offset, stride, static_cast<void *>(mOrigin)));
    RavlAssert(&mPixs[region.mMinat] == pix);
    region.mMinValue = level;
    region.mMaxValue = mValueRange.max();
    region.mHist[level] = 1;
    region.mTotal = 1;
    region.mThresh.clear();
    region.mClosed = nullptr;
  }

  //: Add pixel to region.

  inline void SegmentExtremaBaseC::AddPixel(ExtremaChainPixelC *pix, int level, ExtremaRegionC *reg)
  {
    reg->mHist[level]++;
    reg->mTotal++;
    pix->mRegion = reg;
  }

  //: Add pixel to region.

  inline void SegmentExtremaBaseC::MergeRegions(ExtremaChainPixelC *pix, int level, ExtremaRegionC **labels, int n)
  {
    ONDEBUG(SPDLOG_INFO("MergeRegions() Pix={} Level={} N={}", static_cast<void *>(pix), level, n));
    auto maxValue = labels[0]->mTotal;
    ExtremaRegionC *max = labels[0];

    // Find largest region.
    int i;
    for(i = 1; i < n; i++) {
      auto tot = labels[i]->mTotal;
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
      oldr.mMaxValue = level;
      oldr.mMerge = max;
      oldr.mClosed = max;

      // If we don't need the histogram, put it back in a pool
      if(oldr.mHist != nullptr && (oldr.mTotal < mMinSize || ((level - oldr.mMinValue) < mMinMargin))) {
        PushHist(oldr.mHist, level);
        oldr.mHist = nullptr;
      }

      max->mTotal += oldr.mTotal;
      max->mHist[level] += oldr.mTotal;
    }
    AddPixel(pix, level, max);
  }

  //: Generate regions.

  void SegmentExtremaBaseC::GenerateRegions()
  {
    ExtremaChainPixelC *at;
    int clevel = mValueRange.min();
    ExtremaRegionC *labels[6];

    // For each grey level value in image.
    for(auto lit : clipUnsafe(mLevels, mValueRange)) {
      //ONDEBUG(SPDLOG_INFO("Level={}", clevel));

      // Go through linked list of pixels at the current brightness level.
      for(at = lit; at != nullptr; at = at->mNext) {
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
    ONDEBUG(SPDLOG_INFO("Regions labeled={}", mLabelAlloc));
  }

  //: Generate thresholds

  void SegmentExtremaBaseC::Thresholds()
  {
    ONDEBUG(SPDLOG_INFO("Computing thresholds **********************************************"));
    std::vector<ExtremaThresholdC> thresh(size_t(mLimitMaxValue + 2));
    size_t nthresh = 0;
    assert(mLimitMaxValue + 2 < std::numeric_limits<int>::max());
    Array<uint32_t, 1> chist(IndexRange<1>(0, mLimitMaxValue + 2), 0);

    auto end = mRegionMap.begin() + mLabelAlloc;

    for(auto it = mRegionMap.begin(); it != end; ++it) {
      if(it->mTotal < mMinSize || (it->mMaxValue - it->mMinValue) < mMinMargin) {
        it->mThresh.clear();// Ignore these regions.
        continue;       // Not enough levels in the region.
      }
      // Build the cumulative histogram.
      int maxValue = it->mMaxValue;
      int minValue = it->mMinValue;
      uint32_t sum = 0;

      //ONDEBUG(std::cerr << "Hist= " << it->mMinValue << " :");
      ONDEBUG(std::string histStr = fmt::format("Hist= {}->{} :", it->minValue, it->maxValue));
      // Build cumulative histogram.
      for(int i = minValue; i <= maxValue; i++) {
        sum += it->mHist[i];
        chist[i] = sum;
        ONDEBUG(histStr += fmt::format(" {}", it->mHist[i]));
      }
      assert(chist[maxValue] == sum);
      ONDEBUG(SPDLOG_INFO("{}", histStr));

      //ONDEBUG(std::cerr << "  Closed=" << (it->closed != 0)<< "\n");
      ONDEBUG(SPDLOG_INFO("Region={} Min={} Max={} Total={} Closed={}", it - mRegionMap.begin(), minValue, maxValue, sum, it->mClosed != nullptr));
      int up;
      // look for threshold that guarantee area bigger than mMinSize.
      int i;
      for(i = minValue; i <= maxValue; i++) {
        if(chist[i] >= mMinSize)
          break;
      }

      // Find thresholds.
      nthresh = 0;
      ONDEBUG(SPDLOG_INFO("Min={} Max={} Init={} MaxSize={}", minValue, maxValue, i, mMaxSize));
      size_t lastThresh = 0;
      for(up = i + 1; up < maxValue && i < maxValue; i++) {
        auto area_i = chist[i];
        if(area_i > mMaxSize) {
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
        if(margin > mMinMargin) {// && margin > prevMargin
          ExtremaThresholdC &et = thresh[nthresh++];
          et.mPos = i;
          et.mMargin = margin;
          et.mThresh = i + margin / 2;
          ONDEBUG(SPDLOG_INFO("Threshold={} Area={}", et.mThresh, chist[et.mThresh]));
        }
      }

      if(it->mClosed == nullptr) {// Is region closed ?
        ExtremaThresholdC &et = thresh[nthresh++];
        et.mPos = maxValue - 1;
        et.mMargin = up - i;
        et.mThresh = maxValue - 1;
      }
      ONDEBUG(SPDLOG_INFO("Thresholds={} Kept={}", nthresh, it->nThresh));
      //std::vector<ExtremaThresholdC> newthresh = new std::vector<ExtremaThresholdC>(nthresh);
      std::vector<ExtremaThresholdC> newthresh;
      newthresh.reserve(nthresh);

      unsigned lastSize = 0;
      unsigned lastInd = 0;
      for(unsigned j = 0; j < nthresh; j++) {
        auto size = chist[thresh[j].mPos];
        if((lastSize * 1.15) > size) {// Is size only slightly different ?
          if(thresh[j].mMargin > thresh[lastInd].mMargin) {
            newthresh.back() = thresh[j];// Move threshold if margin is bigger.
            newthresh.back().mArea = chist[thresh[j].mThresh];
            lastSize = size;
          }
          ONDEBUG(SPDLOG_INFO("Rejecting threshold={} LastArea={} Area={}", thresh[j].thresh, size, chist[thresh[j].thresh]));
          continue;// Reject it, not enough difference.
        }
        newthresh.push_back(thresh[j]);
        newthresh.back().mArea = chist[thresh[j].mThresh];
        lastSize = size;
        lastInd = j;
      }

      if(!newthresh.empty()) {
        it->mThresh = std::move(newthresh);
      } else {
        it->mThresh.clear();
      }
      ONDEBUG(SPDLOG_INFO("Thresholds={} Kept={}", nthresh, it->nThresh));
    }// for(SArray1dIterC<ExtremaRegionC> it(...
    //cerr << "SegmentExtremaBaseC::Thresholds() Interesting regions=" << regions <<" \n";
  }
}// namespace Ravl2
