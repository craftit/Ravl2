// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once
//! author="Charles Galambos, based on code by Jiri Matas."

#include <queue>
#include <utility>
#include "Ravl2/Assert.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawFrame.hh"
#include "Ravl2/Image/Segmentation/Boundary.hh"
#include "Ravl2/ArrayIterZip.hh"

namespace Ravl2
{

  //! Scan line info

  class FloodRegionLineC
  {
  public:
    //! Constructor.
    FloodRegionLineC(Index<2> nstart, int colEnd, int ndr)
        : start(nstart),
          end(colEnd),
          dr(ndr)
    {}

    //! Constructor.
    FloodRegionLineC(int row, int col, int colEnd, int ndr)
        : start(row, col),
          end(colEnd),
          dr(ndr)
    {}

    //! Access start location
    [[nodiscard]] const Index<2> &Start() const
    {
      return start;
    }

    //! Get end of line.
    [[nodiscard]] int End() const
    {
      return end;
    }

    //! Row direction.
    [[nodiscard]] int DR() const
    {
      return dr;
    }

  protected:
    Index<2> start;
    int end = 0;
    int dr = 0;
  };

  //! Threshold comparison class.

  template <class PixelT>
  class FloodRegionLessThanThresholdC
  {
  public:
    //! Default constructor.
    FloodRegionLessThanThresholdC() = default;

    //! Construct from pixel value.
    explicit FloodRegionLessThanThresholdC(const PixelT &pix)
        : value(pix)
    {}

    //! Should pixel be included in the region ?
    bool operator()(const PixelT &pix) const
    {
      return pix <= value;
    }

  private:
    PixelT value {};
  };

  //! Flood based region growing.
  //! Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a boundary as the result.

  template <class PixelT>
  class FloodRegionC
  {
  public:
    //! Default constructor.
    FloodRegionC() = default;

    //! Constructor with image to segment.
    explicit FloodRegionC(const Array<PixelT, 2> &nimg)
    {
      SetupImage(nimg);
    }

    //! Access the image we're currently segmenting.
    [[nodiscard]] const Array<PixelT, 2> &Image() const
    {
      return img;
    }

    //! Setup new image for processing.
    void SetupImage(const Array<PixelT, 2> &nimg)
    {
      img = nimg;
      IndexRange<2> rng = img.range().expand(1);
      if(!marki.range().contains(rng)) {
        marki = Array<int, 2>(rng, 0);
        id = 1;
      }
    }

    //! @brief Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a mask as the result.
    //! @param seed The seed pixel to start the region growing from.
    //! @param inclusionTest The test to determine if a pixel is included in the region.
    //! @param boundary The boundary of the region.
    //! @param maxSize The maximum size of the region to grow. (0 = no limit)
    //! @return true if the boundary has a non zero area.
    template<typename InclusionTestT>
    bool GrowRegion(const Index<2> &seed, InclusionTestT &&inclusionTest, Boundary &boundary, size_t maxSize = 0);

    //! @brief Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a mask as the result.
    //! @param seed The seed pixel to start the region growing from.
    //! @param inclusionTest The test to determine if a pixel is included in the region.
    //! @param mask The mask image of the region that will be generated.
    //! @param padding The padding to add to the mask.
    //! @param maxSize The maximum size of the region to grow. (0 = no limit)
    //! @return The size of the region.
    template <typename MaskT,typename InclusionTestT>
    size_t GrowRegion(const Index<2> &seed, InclusionTestT &&inclusionTest, Array<MaskT, 2> &mask, unsigned padding = 0, size_t maxSize = 0);

    //! Access marked pixel image.
    [[nodiscard]] Array<int, 2> &MarkImage()
    {
      return marki;
    }

    //! Access current region id.
    [[nodiscard]] int MarkId() const
    {
      return id;
    }

  private:
    //! Base grow region routine.
    template <typename InclusionTestT>
    bool BaseGrowRegion(const Index<2> &seed, InclusionTestT &&inclusionTest, IndexRange<2> &rng);

    Array<PixelT, 2> img;
    Array<int, 2> marki;
    int id = 0;
    std::queue<FloodRegionLineC> pixQueue;
  };

  //! Base grow region routine.
  // A rewrite of code from: A Seed Fill Algorithm by Paul Heckbert from "Graphics Gems", Academic Press, 1990

  template <class PixelT>
  template <typename InclusionTestT>
  bool FloodRegionC<PixelT>::BaseGrowRegion(const Index<2> &seed, InclusionTestT &&inclusionTest, IndexRange<2> &rng)
  {
    RavlAssert(img.range().contains(seed));
    if(!inclusionTest(img[seed]))
      return false; // Empty region.

    // Make sure queue is empty.
    pixQueue = std::queue<FloodRegionLineC>();

    const auto &imgCols = img.range(1);
    const auto &imgRows = img.range(0);

    if(img.range().range(0).contains(seed[0] + 1))
      pixQueue.emplace(seed[0] + 1, seed[1], seed[1], 1);
    pixQueue.emplace(seed[0], seed[1], seed[1], -1);

    // Misc bits a pieces.
    rng = IndexRange<2>(seed, seed);
    id++;

    // Lookout for id wrap around.
    if(id == 0) {
      fill(marki, 0);
      id++;
    }

    int l = 0;
    while(!pixQueue.empty()) {
      // Pop next line to examine off the stack.
      FloodRegionLineC line = pixQueue.front();
      const int &lsc = line.Start()[1];
      const int &lsr = line.Start()[0];
      pixQueue.pop();
      //cerr << "Line=" << lsr << " " << lsc << " " << line.End() << " DR=" << line.DR() << "\n";
      // Involve scan line in rec.
      rng.range(0).involve(lsr);

      // Do some prep for fast row access.
      const auto irow = img[lsr];
      auto mrow = marki[lsr];

      // segment of scan line lsr - line.DY() for lsc <= c <= line.End() was previously filled,
      // now explore adjacent pixels in scan line lsr
      int c;
      for(c = lsc; c >= imgCols.min() && (mrow[c] != id) && inclusionTest(irow[c]); c--)
        mrow[c] = id;

      if(c >= lsc)
        goto skip;

      rng.range(1).involve(c);
      l = c + 1;
      if(l < lsc) {// Leak on the left ?
        int newRow = lsr - line.DR();
        if(imgRows.contains(newRow))
          pixQueue.push(FloodRegionLineC(newRow, l, lsc - 1, -line.DR()));
      }

      c = lsc + 1;

      do {
        for(; c <= imgCols.max() && (mrow[c] != id) && inclusionTest(irow[c]); c++)
          mrow[c] = id;
        rng.range(1).involve(c);

        {
          int newRow = lsr + line.DR();
          if(imgRows.contains(newRow))
            pixQueue.push(FloodRegionLineC(newRow, l, c - 1, line.DR()));
        }

        if(c > (line.End() + 1)) {// Leak on the right ?
          int newRow = lsr - line.DR();
          if(imgRows.contains(newRow))
            pixQueue.push(FloodRegionLineC(newRow, (line.End() + 1), c - 1, -line.DR()));
        }

      skip:
        for(c++; c <= line.End() && ((mrow[c] == id) || !inclusionTest(irow[c])); c++);
        l = c;
      } while(c <= line.End());
    }
    return true;
  }

  template <class PixelT>
  template <class InclusionTestT>
  bool FloodRegionC<PixelT>::GrowRegion(const Index<2> &seed, InclusionTestT &&inclusionTest, Boundary &boundary, [[maybe_unused]] size_t maxSize)
  {
    IndexRange<2> rng;
    if(!BaseGrowRegion(seed, inclusionTest, rng) || rng.area() <= 0) {
      boundary = Boundary();
      return false;
    }
    rng = rng.expand(1);
    rng.clipBy(marki.range());
    boundary = Boundary::traceBoundary(clip(marki, rng), id);
    return true;
  }

  template <class PixelT>
  template <typename MaskT, class InclusionTestT>
  size_t FloodRegionC<PixelT>::GrowRegion(const Index<2> &seed, InclusionTestT &&inclusionTest, Array<MaskT, 2> &mask, unsigned padding, size_t maxSize)
  {
    IndexRange<2> rng;

    if(!BaseGrowRegion(seed, inclusionTest, rng)) {
      mask = Array<MaskT, 2>();
      return 0;
    }

    // Extract region.
    mask = Array<MaskT, 2>(rng.expand(int(padding)));
    if(padding > 0)
      DrawFrame(mask, MaskT(0), padding, mask.range());
    size_t size = 0;

    for(auto it = begin(clip(mask, rng), clip(marki, rng)); it.valid(); it++) {
      if(it.template data<1>() == id) {
        size++;
        it.template data<0>() = 1;
        if(size > maxSize && maxSize > 0)
          break;
      } else
        it.template data<0>() = 0;
    }
    return size;
  }

}// namespace Ravl2
