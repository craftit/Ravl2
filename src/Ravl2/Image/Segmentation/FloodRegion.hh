// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once
//! author="Charles Galambos, based on code by Jiri Matas."

#include <queue>
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
    FloodRegionLineC(Index<2> nstart, int colEnd, int ndr)
        : start(nstart),
          end(colEnd),
          dr(ndr)
    {}
    //: Constructor.

    FloodRegionLineC(int row, int col, int colEnd, int ndr)
        : start(row, col),
          end(colEnd),
          dr(ndr)
    {}
    //: Constructor.

    [[nodiscard]] const Index<2> &Start() const
    {
      return start;
    }
    //: Access start location

    int End() const
    {
      return end;
    }
    //: Get end of line.

    int DR() const
    {
      return dr;
    }
    //: Row direction.

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

  protected:
    PixelT value {};
  };

  template <class PixelT>
  class FloodRegionGreaterThanThresholdC
  {
  public:
    //! Default constructor.
    FloodRegionGreaterThanThresholdC() = default;

    //! Construct from pixel value.
    explicit FloodRegionGreaterThanThresholdC(const PixelT &pix)
        : value(pix)
    {}

    //! Should pixel be included in the region ?
    bool operator()(const PixelT &pix) const
    {
      return pix >= value;
    }

  protected:
    PixelT value {};
  };

  //! Flood based region growing.
  //! Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a boundary as the result.

  template <class PixelT, class InclusionTestT = FloodRegionLessThanThresholdC<PixelT>>
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
    const Array<PixelT, 2> &Image() const
    {
      return img;
    }

    //! Setup new image for processing.
    bool SetupImage(const Array<PixelT, 2> &nimg)
    {
      img = nimg;
      IndexRange<2> rng = img.range().expand(1);
      if(!marki.range().contains(rng)) {
        marki = Array<int, 2>(rng, 0);
        id = 1;
      }
      return true;
    }

    //! Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a mask as the result.
    //! Returns true if the boundary has a non zero area.
    bool GrowRegion(const Index<2> &seed, const InclusionTestT &inclusionCriteria, Boundary &boundary, size_t maxSize = 0);

    //! Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a mask as the result.
    // The mask images are generated with a boundary
    // Returns the region size.
    template <typename MaskT>
    size_t GrowRegion(const Index<2> &seed, const InclusionTestT &inclusionCriteria, Array<MaskT, 2> &mask, unsigned padding = 0, size_t maxSize = 0);

    Array<int, 2> &MarkImage()
    {
      return marki;
    }
    //: Access marked pixel image.

    [[nodiscard]] int MarkId() const
    {
      return id;
    }
    //: Access current region id.

  protected:
    bool BaseGrowRegion(const Index<2> &seed, const InclusionTestT &inclusionCriteria, IndexRange<2> &rng);
    //: Base grow region routine.

    Array<PixelT, 2> img;
    Array<int, 2> marki;
    int id = 0;
    InclusionTestT inclusionTest;
    std::queue<FloodRegionLineC> pixQueue;
  };

  //! Base grow region routine.
  // A rewrite of code from: A Seed Fill Algorithm by Paul Heckbert from "Grahics Gems", Academic Press, 1990

  template <class PixelT, class InclusionTestT>
  bool FloodRegionC<PixelT, InclusionTestT>::BaseGrowRegion(const Index<2> &seed, const InclusionTestT &inclusionCriteria, IndexRange<2> &rng)
  {
    // Check seed.
    inclusionTest = inclusionCriteria;
    RavlAssert(img.range().contains(seed));
    if(!inclusionTest(img[seed]))
      return false;// Empty region.

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

  template <class PixelT, class InclusionTestT>
  bool FloodRegionC<PixelT, InclusionTestT>::GrowRegion(const Index<2> &seed, const InclusionTestT &inclusionCriteria, Boundary &boundary, [[maybe_unused]] size_t maxSize)
  {
    IndexRange<2> rng;
    if(!BaseGrowRegion(seed, inclusionCriteria, rng) || rng.area() <= 0) {
      boundary = Boundary();
      return false;
    }
    rng = rng.expand(1);
    rng.clipBy(marki.range());
    boundary = Boundary::traceBoundary(clip(marki, rng), id);
    return true;
  }

  template <class PixelT, class InclusionTestT>
  template <typename MaskT>
  size_t FloodRegionC<PixelT, InclusionTestT>::GrowRegion(const Index<2> &seed, const InclusionTestT &inclusionCriteria, Array<MaskT, 2> &mask, unsigned padding, size_t maxSize)
  {
    IndexRange<2> rng;

    if(!BaseGrowRegion(seed, inclusionCriteria, rng)) {
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
