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
#include <spdlog/spdlog.h>
#include "Ravl2/Index.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Image/Segmentation/FloodRegion.hh"
#include "Ravl2/IndexRangeSet.hh"

namespace Ravl2
{

  //: Common parts to segmenting extrema.

  class SegmentExtremaBase
  {
  public:
    using RealT = float;

    //! Constructor.
    //! \param minRegionSize - Minimum region size to detect.
    //! \param nMinMargin - Threshold for region stability.
    //! \param nlimitMaxValue - Maximum pixel value.
    //! \param maxRegionSize - Maximum region size.
    SegmentExtremaBase(uint32_t minRegionSize, int nMinMargin, int nlimitMaxValue = 255, size_t maxRegionSize = 0)
        : mStride(0),
          mLabelAlloc(1),
          mOrigin(nullptr),
          mMinSize(minRegionSize),
          mMaxSize(maxRegionSize),// If 0 this gets set to the image size.
          mMinMargin(nMinMargin),
          mLimitMaxValue(std::max(nlimitMaxValue, 3))
    {
      histStack.reserve(100);
    }

    SegmentExtremaBase(const SegmentExtremaBase &) = delete;
    SegmentExtremaBase &operator=(const SegmentExtremaBase &) = delete;

    //! Destructor.
    ~SegmentExtremaBase();

    //! Setup structures for a given image size.
    void SetupImage(const IndexRange<2> &rect);

    //! Generate regions.
    void GenerateRegions();

    //! Get number of levels being considered.
    [[nodiscard]] unsigned levels() const
    {
      return unsigned(mLevels.range().size());
    }

    //! Access minimum size.
    [[nodiscard]] auto minSize() const
    {
      return mMinSize;
    }

    //! Access maximum size.
    [[nodiscard]] auto maxSize() const
    {
      return mMaxSize;
    }

    //! Access minimum margin
    [[nodiscard]] auto minMargin() const
    {
      return mMinMargin;
    }

    //! Access value limit
    [[nodiscard]] auto limitMaxValue() const
    {
      return mLimitMaxValue;
    }


  protected:

    //! Extrema threshold information.

    class ExtremaThreshold
    {
    public:
      int mThresh;   //!< Threshold value.
      int mPos;      //!< Start of margin.  (mThresh = pos + margin/2)
      int mMargin;   //!< Measure of stability.
      uint32_t mArea;//!< Expected area of region.
    };

    //! Extremal region

    class ExtremaRegion
    {
    public:
      //! Constructor.
      ExtremaRegion() = default;

      //! Destructor.
      ~ExtremaRegion()
      {
        if(mHist != nullptr)
          delete[] mHist;
      }

      ExtremaRegion(const ExtremaRegion &) = delete;
      ExtremaRegion &operator=(const ExtremaRegion &) = delete;

      ExtremaRegion(ExtremaRegion &&other) {
        mMerge = other.mMerge;
        mHist = other.mHist;
        mTotal = other.mTotal;
        mThresh = std::move(other.mThresh);
        mMaxValue = other.mMaxValue;
        mMinValue = other.mMinValue;
        mMinat = other.mMinat;
        mClosed = other.mClosed;
        other.mHist = nullptr;
#ifndef NDEBUG
        other.mMerge = nullptr;
        other.mTotal = 0;
        other.mMaxValue = 0;
        other.mMinValue = 0;
        other.mMinat = {};
        other.mClosed = nullptr;
#endif
      }

      ExtremaRegion &operator=(ExtremaRegion &&other) {
        if(this != &other) {
          mMerge = other.mMerge;
          mHist = other.mHist;
          mTotal = other.mTotal;
          mThresh = std::move(other.mThresh);
          mMaxValue = other.mMaxValue;
          mMinValue = other.mMinValue;
          mMinat = other.mMinat;
          mClosed = other.mClosed;
          other.mHist = nullptr;
#ifndef NDEBUG
          other.mMerge = nullptr;
          other.mTotal = 0;
          other.mMaxValue = 0;
          other.mMinValue = 0;
          other.mMinat = {};
          other.mClosed = nullptr;
#endif
        }
        return *this;
      }

      ExtremaRegion *mMerge = nullptr;//!< Region to merge with.
      uint32_t *mHist = nullptr;       //!< Histogram of pixels values at level.
      uint32_t mTotal = 0;

      std::vector<ExtremaThreshold> mThresh;//!< thresholds

      int mMaxValue = 0;
      int mMinValue = 0;
      Index<2> mMinat {};
      ExtremaRegion *mClosed = nullptr;
    };


    //: Extremal pixel list.

    class ExtremaChainPixel
    {
    public:
      ExtremaRegion *mRegion;
      ExtremaChainPixel *mNext;
    };

    //! Access level set array.
    [[nodiscard]] Array<ExtremaChainPixel *, 1> &levelSets()
    {
      return mLevels;
    }

    //! Find the labels around the pixel 'pix'
    // put the results into 'labelArray' which must be at least 4 labels long.
    // The number of labels found is returned.
    int connectedLabels(ExtremaChainPixel *pix, ExtremaRegion **labelArray);

    //! Add a new region.
    void addRegion(ExtremaChainPixel *pix, int level);

    //! Add pixel to region.
    void addPixel(ExtremaChainPixel *pix, int level, ExtremaRegion *reg);

    //! Add pixel to region.
    void mergeRegions(ExtremaChainPixel *pix, int level, ExtremaRegion **labels, int n);

    //! Generate thresholds
    void thresholds();

    //! Find matching label.
    static ExtremaRegion *findLabel(ExtremaChainPixel *lab);

    Array<ExtremaChainPixel, 2> mPixs;
    Array<ExtremaChainPixel *, 1> mLevels;

    std::vector<ExtremaRegion> mRegionMap;
    int mStride = 0;// Image stride.
    unsigned mLabelAlloc = 0;
    IndexRange<1> mValueRange;            // Range of pixel values.
    ExtremaChainPixel *mOrigin = nullptr;// Origin of image.

    // Parameters.

    size_t mMinSize = 0;
    size_t mMaxSize = 0;
    int mMinMargin = 0;
    const int mLimitMaxValue = 0;// Maximum image value that will be encountered.

  private:

    struct HistStackC {
      HistStackC(int nmax, uint32_t *nstack)
        : max(nmax),
          data(nstack)
      {}
      int max = 0;
      uint32_t *data = nullptr;
    };

    std::vector<HistStackC> histStack;

    //! Get a new histogram.
    uint32_t *popHist(int level)
    {
      assert(level >= 0);
      if(histStack.empty()) {
        // Should this deal with negative pixel values?
        assert(mLimitMaxValue > 0);
        auto *val = new uint32_t[size_t(mLimitMaxValue + 2)];
        memset(&(val[level]), 0, sizeof(uint32_t) * size_t((mLimitMaxValue + 1) - level));
        return val;
      }
      HistStackC &ret = histStack.back();
      uint32_t *rret = ret.data;
      ret.data = nullptr;
      int clearSize = std::max(ret.max + 1, 3) - level;
      histStack.pop_back();
      if(clearSize > 0)
        memset(&(rret[level]), 0, sizeof(uint32_t) * size_t(clearSize));
      return rret;
    }

    //! Push unused histogram
    void pushHist(uint32_t *stack, int used)
    {
      histStack.emplace_back(used, stack);
    }
  };

  //! @brief Maximally Stable Extremal Region's  (MSER's)

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

  template <class PixelT>
  class SegmentExtrema : public SegmentExtremaBase
  {
  public:
    //! @brief Constructor.
    //! @param minRegionSize - Minimum region size in pixels to detect.
    //! @param minMargin - Threshold for region stability.
    //! @param minRegionSize - Maximum pixel value to consider.
    explicit SegmentExtrema(uint32_t minRegionSize = 12, int theMinMargin = 10, int nLimitMaxPixelValue = 255)
        : SegmentExtremaBase(minRegionSize, theMinMargin, nLimitMaxPixelValue)
    {
      assert(minRegionSize > 0);
      assert(nLimitMaxPixelValue > 0);
    }

    //! @brief Segment image with boundaries
    //! The boundaries generated by this function are not ordered. If you require ordered
    //! boundaries you can use its 'order()' method to sort them.
    std::vector<Boundary> apply(const Array<PixelT, 2> &img)
    {
      if(typeid(PixelT) == typeid(uint8_t)) {// Should decided at compile time.
        sortPixelsByte(img);
      } else {
        sortPixels(img);
      }
      GenerateRegions();
      thresholds();
      return growRegionBoundary(img);
    }

    //! @brief Segment image using callback
    //! The callback is given, a location of a pixel inside the boundary, the threshold and
    //! @param img Image to segment.
    //! @param callback Callback to apply to each region.
    //! The argument for the callback are:
    //!   Index<2> - location in a region
    //!   int - Threshold to apply
    //!   size_t - Expected area of the region.

    template <typename CallbackT>
    void apply(const Array<PixelT, 2> &img, CallbackT &&callback)
    {
      if constexpr(typeid(PixelT) == typeid(uint8_t)) {
        sortPixelsByte(img);
      } else {
        sortPixels(img);
      }
      GenerateRegions();
      thresholds();

      auto end = mRegionMap.begin();
      end += mLabelAlloc;
      for(auto it = mRegionMap.begin(); it != end; ++it) {

        for(const auto &at : it->mThresh) {
          callback(it->mMinat, at.mThresh, at.mArea);
        }
        it->mThresh.clear();
      }
    }

    //! @brief Segment image into label images.
    //! Generates labeled images.
    std::vector<Array<int, 2>> applyMask(const Array<PixelT, 2> &img)
    {
      if(typeid(PixelT) == typeid(uint8_t))// Should decided at compile time.
        sortPixelsByte(img);
      else
        sortPixels(img);
      GenerateRegions();
      thresholds();
      return growRegionMask(img);
    }

    //! Apply operation to specified region of an img.
    //! Note: The boundaries generated by this function are not ordered. If you require ordered
    //! boundaries you can use its 'order()' method to sort them.

    std::vector<Boundary> apply(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi)
    {
      sortPixels(img, roi);
      GenerateRegions();
      thresholds();
      return growRegionBoundary(img);
    }

    //! Apply operation to img.
    // Generates labeled images.
    std::vector<Array<int, 2>> applyMask(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi)
    {
      sortPixels(img, roi);
      GenerateRegions();
      thresholds();
      return growRegionMask(img);
    }

  protected:
    //! Do initial sorting of pixels.
    bool sortPixels(const Array<PixelT, 2> &nimg);

    //! Do initial sorting of pixels.
    bool sortPixelsByte(const Array<PixelT, 2> &nimg);

    //! Build a list from a byte image in regions of interest.
    bool sortPixels(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi);

    //! Grow regions.
    std::vector<Boundary> growRegionBoundary(const Array<PixelT, 2> &img);

    //! Grow regions associated with a extrema.
    void growRegionBoundary(std::vector<Boundary> &boundaries, ExtremaRegion &region);

    //! Grow regions.
    std::vector<Array<int, 2>> growRegionMask(const Array<PixelT, 2> &img);

    //! Grow regions associated with a extrema.
    std::vector<Array<int, 2>> growRegionMask(ExtremaRegion &region);

    FloodRegionC<PixelT> mFlood;//!< Region fill code.
  };

  template <class PixelT>
  bool SegmentExtrema<PixelT>::sortPixels(const Array<PixelT, 2> &img)
  {

    SetupImage(img.range());
    // Find range of values in image.
    {
      auto it = img.begin();
      PixelT lmin = *it;
      PixelT lmax = *it;
      for(; it.valid(); it++) {
        if(*it < lmin)
          lmin = *it;
        if(*it > lmax)
          lmax = *it;
      }
      mValueRange.min() = int(lmin);
      mValueRange.max() = int(lmax);
    }

    if(mValueRange.max() >= mLimitMaxValue)
      mValueRange.max() = mLimitMaxValue - 1;

    //cerr << "SegmentExtrema<PixelT>::sortPixels, Value Range=" << mValueRange << "\n";

    // Check level list allocation.

    if(!mLevels.range().contains(mValueRange))
      mLevels = Array<ExtremaChainPixel *, 1>(mValueRange.expand(2));
    fill(mLevels, nullptr);

    // Sort pixels into appropriate lists.
    RavlAssert(mPixs.range().contains(img.range()));

    for(auto it = zip(img, clipUnsafe(mPixs, img.range())); it.valid(); it++) {
      it.template data<1>().mRegion = 0;
      PixelT val = it.template data<0>();
      if(val > mLimitMaxValue) {
        continue;
      }
      auto &tmp = mLevels[val];
      it.template data<1>().mNext = tmp;
      tmp = &it.template data<1>();
    }
    SPDLOG_INFO("SegmentExtrema<PixelT>::sortPixels, Value Range={}", mValueRange);

    return true;
  }

  template <class PixelT>
  bool SegmentExtrema<PixelT>::sortPixelsByte(const Array<PixelT, 2> &img)
  {
    SetupImage(img.range());

    // Check level list allocation.
    IndexRange<1> targetRange(std::numeric_limits<PixelT>::min(), std::numeric_limits<PixelT>::max());
    if(mLevels.range() != targetRange)
      mLevels = Array<ExtremaChainPixel *, 1>(targetRange);
    memset(&(mLevels[mLevels.range().min()]), 0, size_t(mLevels.range().size()) * sizeof(ExtremaChainPixel *));
    static_assert(std::numeric_limits<PixelT>::max() <= std::numeric_limits<int>::max(), "PixelT must be a byte type.");

    // Sort pixels into appropriate lists.
    mValueRange = IndexRange<1>::mostEmpty();
    for(auto it = zip(img, clipUnsafe(mPixs, img.range())); it.valid(); ++it) {
      it.template data<1>().mRegion = nullptr;
      int val = it.template data<0>();
      if(val > mLimitMaxValue)
        continue;
      mValueRange.involve(val);
      ExtremaChainPixel *&tmp = mLevels[val];
      it.template data<1>().mNext = tmp;
      tmp = &it.template data<1>();
    }
    //SPDLOG_INFO("SegmentExtrema<PixelT>::sortPixels, Value Range={}", mValueRange);
    return true;
  }

  template <class PixelT>
  bool SegmentExtrema<PixelT>::sortPixels(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi)
  {

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
      auto it = clipUnsafe(img, rng).begin();
      if(first) {
        lmin = *it;
        lmax = *it;
        first = false;
      }
      for(; it.valid; ++it) {
        if(*it < lmin)
          lmin = *it;
        if(*it > lmax)
          lmax = *it;
      }
    }
    mValueRange.min() = int(lmin);
    mValueRange.max() = int(lmax);
    if(mValueRange.max() > mLimitMaxValue)
      mValueRange.max() = mLimitMaxValue;
    //cerr << "SegmentExtrema<PixelT>::sortPixels, Value Range=" << mValueRange << "\n";
    // Check level list allocation.

    if(!mLevels.range().contains(mValueRange))
      mLevels = Array<ExtremaChainPixel *, 1>(mValueRange.expand(2));
    fill(mLevels, nullptr);

    // Clear chain image.

    for(auto &it : clipUnsafe(mPixs, img.range())) {
      it.Data().region = 0;
      it.Data().next = 0;
    }

    // Sort pixels into appropriate lists.

    for(auto rit : roi) {
      IndexRange<2> rng = rit;
      rng.clipBy(img.range());
      if(rng.area() < 1)
        continue;
      for(auto it = begin(clipUnsafe(img, rng), clipUnsafe(mPixs, rng)); it; it++) {
        PixelT val = it.template data<0>();
        if(val > mLimitMaxValue)
          continue;
        ExtremaChainPixel *&tmp = mLevels[val];
        it.template data<1>().next = tmp;
        tmp = &it.template data<1>();
      }
    }
    return true;
  }

  template <class PixelT>
  std::vector<Boundary> SegmentExtrema<PixelT>::growRegionBoundary(const Array<PixelT, 2> &img)
  {
    //cerr << "SegmentExtremaBase::GrowRegions() \n";
    mFlood.SetupImage(img);

    std::vector<Boundary> bounds;
    bounds.reserve(size_t(img.range().area()) / (mMinSize * 3u));
    if(mLabelAlloc == 0)
      return bounds;
    auto end = mRegionMap.begin();
    end += mLabelAlloc;
    for(auto it = mRegionMap.begin(); it != end; ++it) {
      if(!it->mThresh.empty()) {
        growRegionBoundary(bounds, *it);
      }
      it->mThresh.clear();
    }

    return bounds;
  }

  template <class PixelT>
  void SegmentExtrema<PixelT>::growRegionBoundary(std::vector<Boundary> &ret, ExtremaRegion &region)
  {
    for(size_t i = 0; i < region.mThresh.size(); i++) {
      Boundary boundary;
      if(!mFlood.GrowRegion(region.mMinat, FloodRegionLessThanThresholdC<PixelT>(PixelT(region.mThresh[i].mThresh)), boundary, mMaxSize)) {
        SPDLOG_WARN("growRegionBoundary, Failed to grow region at:{} Threshold:{} Area:{} SeedValue:{}", region.mMinat, region.mThresh[i].mThresh, region.mThresh[i].mArea, mFlood.Image()[region.mMinat]);
        continue;
      }
#ifndef NDEBUG
      if(boundary.area() != int(region.mThresh[i].mArea)) {
        SPDLOG_WARN("Area mismatch, At:{} Threshold:{} Boundary Size={} Area:{} Expected:{} ", region.mMinat, region.mThresh[i].mThresh, boundary.size(), boundary.area(), region.mThresh[i].mArea);
      }
#endif
      ret.push_back(boundary);
    }
  }

  template <class PixelT>
  std::vector<Array<int, 2>> SegmentExtrema<PixelT>::growRegionMask(const Array<PixelT, 2> &img)
  {
    //cerr << "SegmentExtremaBase::GrowRegions() \n";
    mFlood.SetupImage(img);

    std::vector<Array<int, 2>> masks;
    if(mLabelAlloc == 0)
      return masks;
    auto end = mRegionMap.begin() + mLabelAlloc;
    for(auto it = mRegionMap.begin(); it != end; ++it) {
      if(!it->mThresh.empty()) {
        auto regions = growRegionMask(*it);
        masks.insert(masks.end(), regions.begin(), regions.end());
      }
      it->mThresh.clear();
    }

    return masks;
  }

  template <class PixelT>
  std::vector<Array<int, 2>> SegmentExtrema<PixelT>::growRegionMask(ExtremaRegion &region)
  {
    std::vector<Array<int, 2>> ret;
    //cerr << " thresholds=" << region.nThresh << "\n";
    for(int i = 0; i < region.mThresh.size(); i++) {
      Array<int, 2> mask;
      if(mFlood.GrowRegion(region.mMinat, FloodRegionLessThanThresholdC<PixelT>(PixelT(region.mThresh[i].mThresh)), mask, 1))
        ret.push_back(mask);
    }
    return ret;
  }

}// namespace Ravl2
