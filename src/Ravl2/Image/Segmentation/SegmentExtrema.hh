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

  //! Extrema threshold information.

  class ExtremaThresholdC
  {
  public:
    int thresh;   //!< Threshold value.
    int pos;      //!< Start of margin.  (thresh = pos + margin/2)
    int margin;   //!< Measure of stability.
    uint32_t area;//!< Expected area of region.
  };

  //! Extremal region

  class ExtremaRegionC
  {
  public:
    //! Constructor.
    ExtremaRegionC() = default;

    //! Destructor.
    ~ExtremaRegionC()
    {
      if(hist != nullptr)
        delete[] hist;
    }

    ExtremaRegionC *merge = nullptr;//!< Region to merge with.
    uint32_t *hist = nullptr;       //!< Histogram of pixels values at level.
    uint32_t total = 0;

    std::vector<ExtremaThresholdC> thresh;//!< Thresholds

    int maxValue = 0;
    int minValue = 0;
    Index<2> minat {};
    ExtremaRegionC *closed = nullptr;
  };

  //: Extremal pixel list.

  class ExtremaChainPixelC
  {
  public:
    ExtremaRegionC *region;
    ExtremaChainPixelC *next;
  };

  //: Common parts to segmenting extrema.

  class SegmentExtremaBaseC
  {
  public:
    using RealT = float;

    //! Constructor.
    //! \param minRegionSize - Minimum region size to detect.
    //! \param nMinMargin - Threshold for region stability.
    //! \param nlimitMaxValue - Maximum pixel value.
    //! \param maxRegionSize - Maximum region size.
    SegmentExtremaBaseC(uint32_t minRegionSize, int nMinMargin, int nlimitMaxValue = 255, size_t maxRegionSize = 0)
        : stride(0),
          labelAlloc(1),
          origin(nullptr),
          minSize(minRegionSize),
          maxSize(maxRegionSize),// If 0 this gets set to the image size.
          minMargin(nMinMargin),
          limitMaxValue(std::max(nlimitMaxValue, 3))
    {
      histStack.reserve(100);
    }

    //! Destructor.
    ~SegmentExtremaBaseC();

    //! Setup structures for a given image size.
    void SetupImage(const IndexRange<2> &rect);

    //! Generate regions.
    void GenerateRegions();

    //! Get number of levels being considered.
    [[nodiscard]] unsigned Levels() const
    {
      return unsigned(levels.range().size());
    }

    //! Access minimum size.
    [[nodiscard]] auto MinSize() const
    {
      return minSize;
    }

    //! Access maximum size.
    [[nodiscard]] auto MaxSize() const
    {
      return maxSize;
    }

    //! Access minimum margin
    [[nodiscard]] auto MinMargin() const
    {
      return minMargin;
    }

    //! Access value limit
    [[nodiscard]] auto LimitMaxValue() const
    {
      return limitMaxValue;
    }

    //! Access level set array.
    [[nodiscard]] Array<ExtremaChainPixelC *, 1> &LevelSets()
    {
      return levels;
    }

  protected:
    //! Find matching label.
    static ExtremaRegionC *FindLabel(ExtremaChainPixelC *lab);

    //! Find the labels around the pixel 'pix'
    // put the results into 'labelArray' which must be at least 4 labels long.
    // The number of labels found is returned.
    int ConnectedLabels(ExtremaChainPixelC *pix, ExtremaRegionC **labelArray);

    //! Add a new region.
    void AddRegion(ExtremaChainPixelC *pix, int level);

    //! Add pixel to region.
    void AddPixel(ExtremaChainPixelC *pix, int level, ExtremaRegionC *reg);

    //! Add pixel to region.
    void MergeRegions(ExtremaChainPixelC *pix, int level, ExtremaRegionC **labels, int n);

    //! Generate thresholds
    void Thresholds();

    Array<ExtremaChainPixelC, 2> pixs;
    Array<ExtremaChainPixelC *, 1> levels;
    std::vector<ExtremaRegionC> regionMap;
    int stride = 0;// Image stride.
    unsigned labelAlloc = 0;
    IndexRange<1> valueRange;            // Range of pixel values.
    ExtremaChainPixelC *origin = nullptr;// Origin of image.

    // Parameters.

    size_t minSize = 0;
    size_t maxSize = 0;
    int minMargin = 0;
    int limitMaxValue = 0;// Maximum image value that will be encountered.

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
    uint32_t *PopHist(int level)
    {
      assert(level >= 0);
      if(histStack.empty()) {
        // Should this deal with negative pixel values?
        assert(limitMaxValue > 0);
        auto *val = new uint32_t[size_t(limitMaxValue + 2)];
        memset(&(val[level]), 0, sizeof(uint32_t) * size_t((limitMaxValue + 1) - level));
        return val;
      }
      HistStackC &ret = histStack.back();
      uint32_t *rret = ret.data;
      int clearSize = std::max(ret.max + 1, 3) - level;
      histStack.pop_back();
      if(clearSize > 0)
        memset(&(rret[level]), 0, sizeof(uint32_t) * size_t(clearSize));
      return rret;
    }

    //! Push unused histogram
    void PushHist(uint32_t *stack, int used)
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
  class SegmentExtremaC : public SegmentExtremaBaseC
  {
  public:
    //! @brief Constructor.
    //! @param minRegionSize - Minimum region size in pixels to detect.
    //! @param minMargin - Threshold for region stability.
    //! @param minRegionSize - Maximum pixel value to consider.
    explicit SegmentExtremaC(uint32_t minRegionSize = 12, int theMinMargin = 10, int nLimitMaxPixelValue = 255)
        : SegmentExtremaBaseC(minRegionSize, theMinMargin, nLimitMaxPixelValue)
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
        SortPixelsByte(img);
      } else {
        SortPixels(img);
      }
      GenerateRegions();
      Thresholds();
      return GrowRegionBoundary(img);
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
        SortPixelsByte(img);
      } else {
        SortPixels(img);
      }
      GenerateRegions();
      Thresholds();

      auto end = regionMap.begin();
      end += labelAlloc;
      for(auto it = regionMap.begin(); it != end; ++it) {

        for(const auto &at : it->thresh) {
          callback(it->minat, at.thresh, at.area);
        }
        it->thresh.clear();
      }
    }

    //! @brief Segment image into label images.
    //! Generates labeled images.
    std::vector<Array<int, 2>> applyMask(const Array<PixelT, 2> &img)
    {
      if(typeid(PixelT) == typeid(uint8_t))// Should decided at compile time.
        SortPixelsByte(img);
      else
        SortPixels(img);
      GenerateRegions();
      Thresholds();
      return GrowRegionMask(img);
    }

    //! Apply operation to specified region of an img.
    //! Note: The boundaries generated by this function are not ordered. If you require ordered
    //! boundaries you can use its 'order()' method to sort them.

    std::vector<Boundary> apply(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi)
    {
      SortPixels(img, roi);
      GenerateRegions();
      Thresholds();
      return GrowRegionBoundary(img);
    }

    //! Apply operation to img.
    // Generates labeled images.
    std::vector<Array<int, 2>> applyMask(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi)
    {
      SortPixels(img, roi);
      GenerateRegions();
      Thresholds();
      return GrowRegionMask(img);
    }

  protected:
    //! Do initial sorting of pixels.
    bool SortPixels(const Array<PixelT, 2> &nimg);

    //! Do initial sorting of pixels.
    bool SortPixelsByte(const Array<PixelT, 2> &nimg);

    //! Build a list from a byte image in regions of interest.
    bool SortPixels(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi);

    //! Grow regions.
    std::vector<Boundary> GrowRegionBoundary(const Array<PixelT, 2> &img);

    //! Grow regions associated with a extrema.
    void GrowRegionBoundary(std::vector<Boundary> &boundaries, ExtremaRegionC &region);

    //! Grow regions.
    std::vector<Array<int, 2>> GrowRegionMask(const Array<PixelT, 2> &img);

    //! Grow regions associated with a extrema.
    std::vector<Array<int, 2>> GrowRegionMask(ExtremaRegionC &region);

    FloodRegionC<PixelT> flood;//!< Region fill code.
  };

  template <class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const Array<PixelT, 2> &img)
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
      valueRange.min() = int(lmin);
      valueRange.max() = int(lmax);
    }

    if(valueRange.max() >= limitMaxValue)
      valueRange.max() = limitMaxValue - 1;

    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";

    // Check level list allocation.

    if(!levels.range().contains(valueRange))
      levels = Array<ExtremaChainPixelC *, 1>(valueRange.expand(2));
    fill(levels, nullptr);

    // Sort pixels into appropriate lists.
    RavlAssert(pixs.range().contains(img.range()));

    for(auto it = zip(img, clipUnsafe(pixs, img.range())); it.valid(); it++) {
      it.template data<1>().region = 0;
      PixelT val = it.template data<0>();
      if(val > limitMaxValue) {
        continue;
      }
      auto &tmp = levels[val];
      it.template data<1>().next = tmp;
      tmp = &it.template data<1>();
    }
    SPDLOG_INFO("SegmentExtremaC<PixelT>::SortPixels, Value Range={}", valueRange);

    return true;
  }

  template <class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixelsByte(const Array<PixelT, 2> &img)
  {
    SetupImage(img.range());

    // Check level list allocation.
    IndexRange<1> targetRange(std::numeric_limits<PixelT>::min(), std::numeric_limits<PixelT>::max());
    if(levels.range() != targetRange)
      levels = Array<ExtremaChainPixelC *, 1>(targetRange);
    memset(&(levels[levels.range().min()]), 0, size_t(levels.range().size()) * sizeof(ExtremaChainPixelC *));
    static_assert(std::numeric_limits<PixelT>::max() <= std::numeric_limits<int>::max(), "PixelT must be a byte type.");

    // Sort pixels into appropriate lists.
    valueRange = IndexRange<1>::mostEmpty();
    for(auto it = zip(img, clipUnsafe(pixs, img.range())); it.valid(); ++it) {
      it.template data<1>().region = nullptr;
      int val = it.template data<0>();
      if(val > limitMaxValue)
        continue;
      valueRange.involve(val);
      ExtremaChainPixelC *&tmp = levels[val];
      it.template data<1>().next = tmp;
      tmp = &it.template data<1>();
    }
    //SPDLOG_INFO("SegmentExtremaC<PixelT>::SortPixels, Value Range={}", valueRange);
    return true;
  }

  template <class PixelT>
  bool SegmentExtremaC<PixelT>::SortPixels(const Array<PixelT, 2> &img, const IndexRangeSet<2> &roi)
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
    valueRange.min() = int(lmin);
    valueRange.max() = int(lmax);
    if(valueRange.max() > limitMaxValue)
      valueRange.max() = limitMaxValue;
    //cerr << "SegmentExtremaC<PixelT>::SortPixels, Value Range=" << valueRange << "\n";
    // Check level list allocation.

    if(!levels.range().contains(valueRange))
      levels = Array<ExtremaChainPixelC *, 1>(valueRange.expand(2));
    fill(levels, nullptr);

    // Clear chain image.

    for(auto &it : clipUnsafe(pixs, img.range())) {
      it.Data().region = 0;
      it.Data().next = 0;
    }

    // Sort pixels into appropriate lists.

    for(auto rit : roi) {
      IndexRange<2> rng = rit;
      rng.clipBy(img.range());
      if(rng.area() < 1)
        continue;
      for(auto it = begin(clipUnsafe(img, rng), clipUnsafe(pixs, rng)); it; it++) {
        PixelT val = it.template data<0>();
        if(val > limitMaxValue)
          continue;
        ExtremaChainPixelC *&tmp = levels[val];
        it.template data<1>().next = tmp;
        tmp = &it.template data<1>();
      }
    }
    return true;
  }

  template <class PixelT>
  std::vector<Boundary> SegmentExtremaC<PixelT>::GrowRegionBoundary(const Array<PixelT, 2> &img)
  {
    //cerr << "SegmentExtremaBaseC::GrowRegions() \n";
    flood.SetupImage(img);

    std::vector<Boundary> bounds;
    bounds.reserve(size_t(img.range().area()) / (minSize * 3u));
    if(labelAlloc == 0)
      return bounds;
    auto end = regionMap.begin();
    end += labelAlloc;
    for(auto it = regionMap.begin(); it != end; ++it) {
      if(!it->thresh.empty()) {
        GrowRegionBoundary(bounds, *it);
      }
      it->thresh.clear();
    }

    return bounds;
  }

  template <class PixelT>
  void SegmentExtremaC<PixelT>::GrowRegionBoundary(std::vector<Boundary> &ret, ExtremaRegionC &region)
  {
    for(size_t i = 0; i < region.thresh.size(); i++) {
      Boundary boundary;
      if(!flood.GrowRegion(region.minat, FloodRegionLessThanThresholdC<PixelT>(PixelT(region.thresh[i].thresh)), boundary, maxSize)) {
        SPDLOG_WARN("GrowRegionBoundary, Failed to grow region at:{} Threshold:{} Area:{} SeedValue:{}", region.minat, region.thresh[i].thresh, region.thresh[i].area, flood.Image()[region.minat]);
        continue;
      }
#ifndef NDEBUG
      if(boundary.area() != int(region.thresh[i].area)) {
        SPDLOG_WARN("Area mismatch, At:{} Threshold:{} Boundary Size={} Area:{} Expected:{} ", region.minat, region.thresh[i].thresh, boundary.size(), boundary.area(), region.thresh[i].area);
      }
#endif
      ret.push_back(boundary);
    }
  }

  template <class PixelT>
  std::vector<Array<int, 2>> SegmentExtremaC<PixelT>::GrowRegionMask(const Array<PixelT, 2> &img)
  {
    //cerr << "SegmentExtremaBaseC::GrowRegions() \n";
    flood.SetupImage(img);

    std::vector<Array<int, 2>> masks;
    if(labelAlloc == 0)
      return masks;
    auto end = regionMap.begin() + labelAlloc;
    for(auto it = regionMap.begin(); it != end; ++it) {
      if(!it->thresh.empty()) {
        auto regions = GrowRegionMask(*it);
        masks.insert(masks.end(), regions.begin(), regions.end());
      }
      it->thresh.clear();
    }

    return masks;
  }

  template <class PixelT>
  std::vector<Array<int, 2>> SegmentExtremaC<PixelT>::GrowRegionMask(ExtremaRegionC &region)
  {
    std::vector<Array<int, 2>> ret;
    //cerr << " Thresholds=" << region.nThresh << "\n";
    for(int i = 0; i < region.thresh.size(); i++) {
      Array<int, 2> mask;
      if(flood.GrowRegion(region.minat, FloodRegionLessThanThresholdC<PixelT>(PixelT(region.thresh[i].thresh)), mask, 1))
        ret.push_back(mask);
    }
    return ret;
  }

}// namespace Ravl2
