// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include "Ravl2/Array.hh"
#include "Ravl2/Image/Array2Sqr2Iter.hh"
#include "Ravl2/Image/Array2Sqr2Iter2.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Moments2.hh"

namespace Ravl2
{
  //: Segmentation map.

  class Segmentation
  {
  public:
    Segmentation()
        : labels(0)
    {}
    //: Default constructor.

    Segmentation(const Array<unsigned, 2> &nsegmap, unsigned nlabels)
        : segmap(nsegmap),
          labels(nlabels)
    {}
    //: Constructor.

    explicit Segmentation(const Array<int, 2> &nsegmap);
    //: Construct from an IntT image.
    // Negative values will be labeled as region 0.

    std::vector<std::set<unsigned>> Adjacency(bool biDir = false);
    //: Generate a table of 4 connected region adjacencies.
    // For each region, a set of adjacent regions is
    // generated.   If biDir is false, only adjacency from
    // regions with a smaller id to those with a larger ID are
    // generated, otherwise both direction are registered.

    std::vector<std::map<unsigned, unsigned>> BoundaryLength(bool biDir = false);
    //: Generate a table of boundary lengths with 4 connected adjacent regions
    // if biDir is false only adjacency from regions with a smaller id to those
    // with a larger ID are generated

    std::vector<unsigned> LocalBoundaryLength();
    //: Generate a table of the length of the 4 connected boundary for each region
    // Note, boundary pixels at the edge of the image are NOT counted.

    unsigned RemoveSmallComponents(unsigned thrSize);
    //: Remove small components from map, label them as 0.

    unsigned CompressAndRelabel(std::vector<unsigned> &newLabs);
    //: Compress newlabs and re-label segmentation.
    // this correctly resolves multilevel mappings.
    // Note: newLabs will be changed to contain a mapping
    // from the original labels to their new values.

    [[nodiscard]] std::vector<std::tuple<IndexRange<2>, unsigned>> BoundsAndArea() const;
    //: Compute the bounding box and area of each region in the segmentation.

    [[nodiscard]] std::vector<unsigned> Areas() const;
    //: Compute the areas of all the segmented regions.

    std::vector<unsigned> RedoArea(std::vector<unsigned> area, std::vector<unsigned> map);
    //: recompute the areas from the original areas and a mapping.

    std::vector<unsigned> IdentityLabel();
    //: Make an array of labels mapping to themselves.
    // This is useful for making merge tables which can
    // the be passed to CompressAndRelabel.

    template <class PixelT, class CmpT, typename RealT>
    unsigned MergeComponents(Array<PixelT, 2> &dat, unsigned thrSize, RealT maxDist, CmpT &cmp, unsigned iter = 1);
    //: Merge simlar components smaller than 'thrSize'.
    // This just looks for the difference between adjacent pixels from different regions.
    // FIXME :- It maybe better to look at the average difference.

    auto &SegMap()
    {
      return segmap;
    }
    //: Access segmentation map.

    auto Labels() const
    {
      return labels;
    }
    //: Access number of labels.

    [[nodiscard]] std::vector<Moments2<float>> ComputeMoments(bool ignoreZero = false);
    //: Compute moments for each of the segmented regions.
    // if ignoreZero is true, region labeled 0 is ignored.

    [[nodiscard]] Array<uint8_t, 2> ByteImage() const;
    //: Returns the segmentation map in the form of a ByteImageC
    // Note: if there are more than 255 labels in the image, some may be used twice.

    //    Array<ByteRGBValueC,2> RandomImage() const;
    //    //: Returns the segmentation map in the form of a colour random image; this means that segmentation maps with more than 255 labels can be saved to disk
    //
    //    Array<ByteYUVValueC,2> RandomTaintImage(ByteT max=100) const;
    //    //: Returns the segmentation map in the form of a colour random image.
    //    // The Y channel is left blank (e.g., for displaying the original data).
    //    // The labels in the U and V channels are in the range 0 to 'max'.

  protected:
    static unsigned RelabelTable(std::vector<unsigned> &labelTable, unsigned currentMaxLabel);
    //: Compress labels.

    Array<unsigned, 2> segmap;// Segmentation map.
    unsigned labels = 0;      // Number of labels in map.
  };

  //: Merge similar components smaller than 'thrSize'.

  template <class PixelT, class CmpT, typename RealT>
  unsigned Segmentation::MergeComponents(Array<PixelT, 2> &dat,
                                         unsigned thrSize,
                                         RealT maxDist,
                                         CmpT &cmp,
                                         unsigned iter)
  {
    if(labels <= 1)
      return labels;
    std::vector<RealT> minDist(labels);
    std::vector<unsigned> area = Areas();

    for(; iter > 0; iter--) {
      std::fill(minDist.begin(), minDist.end(), maxDist);// Fill with maximum merge threshold./
      std::vector<unsigned> minLab = IdentityLabel();

      // Find closest neigbour of small regions.

      // Each 4-connected pair of different labels is considered once; either side may be small.
      auto consider = [&](unsigned la, unsigned lb, const PixelT &pa, const PixelT &pb) {
        if(area[la] < thrSize) {
          RealT d = cmp(pa, pb);
          if(minDist[la] > d) {
            minDist[la] = d;
            minLab[la] = lb;
          }
        }
        if(area[lb] < thrSize) {
          RealT d = cmp(pa, pb);
          if(minDist[lb] > d) {
            minDist[lb] = d;
            minLab[lb] = la;
          }
        }
      };
      const IndexRange<2> &rng = segmap.range();
      for(int r = rng.min(0); r <= rng.max(0); ++r) {
        for(int c = rng.min(1); c <= rng.max(1); ++c) {
          const unsigned here = segmap[r][c];
          if(c < rng.max(1) && segmap[r][c + 1] != here)
            consider(here, segmap[r][c + 1], dat[r][c], dat[r][c + 1]);
          if(r < rng.max(0) && segmap[r + 1][c] != here)
            consider(here, segmap[r + 1][c], dat[r][c], dat[r + 1][c]);
        }
      }
      CompressAndRelabel(minLab);
      if(iter > 1)
        area = RedoArea(area, minLab);
    }
    return labels;
  }
}// namespace Ravl2
