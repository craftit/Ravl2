// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include <cstdint>
#include <limits>

#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Image/SquareIter.hh"
#include "Ravl2/Image/Matching.hh"

namespace Ravl2
{

  //! @brief Search for 'patch' in 'img' by creeping downhill from 'start', with a sub-pixel estimate of the minimum.
  //! Positions are the pixel in 'img' corresponding to 0,0 in 'patch'.
  //! @param rat Sub-pixel position of the best match, or (-1,-1) if 'start' is outside the search area.
  //! @param rminScore Sum of absolute differences at the best integer position.
  //! @return The same score as rminScore.
  template <typename DataT, typename SumT, typename RealT = float>
  SumT searchMinAbsDifferenceCreep(const Array<DataT, 2> &patch, const Array<DataT, 2> &img, const Index<2> &start, Point<RealT, 2> &rat, SumT &rminScore, int searchSize = 50)
  {
    // Positions at which the whole patch lies inside the image, near 'start'.
    IndexRange<2> sarea = IndexRange<2>(start, start).expand(searchSize + 8);
    IndexRange<2> fits(IndexRange<1>(img.range(0).min() - patch.range(0).min(), img.range(0).max() - patch.range(0).max()),
                       IndexRange<1>(img.range(1).min() - patch.range(1).min(), img.range(1).max() - patch.range(1).max()));
    if(!sarea.clipBy(fits) || !sarea.contains(start)) {
      rat = toPoint<RealT>(-1, -1);
      rminScore = std::numeric_limits<SumT>::max();
      return rminScore;
    }

    Array<SumT, 2> scoreMap(sarea, SumT(0));
    Array<uint8_t, 2> evaluated(sarea, uint8_t(0));// SumT may be unsigned, so no sentinel score
    auto scoreAt = [&](const Index<2> &p) -> SumT {
      if(!evaluated[p]) {
        const ArrayView<DataT, 2> window(img.origin_address() + img.stride(0) * p[0] + p[1], patch.range(), img.strides());
        scoreMap[p] = matchSumAbsDifference<DataT, SumT>(patch, window);
        evaluated[p] = 1;
      }
      return scoreMap[p];
    };

    Index<2> minAt = start;
    SumT minScore = scoreAt(minAt);
    Index<2> lastMin;
    int maxLoop = 10;
    do {
      lastMin = minAt;
      for(SquareIterC it(searchSize, lastMin); it; it++) {
        if(!sarea.contains(*it))
          continue;
        const SumT score = scoreAt(*it);
        if(score < minScore) {
          minScore = score;
          minAt = *it;
        }
      }
    } while(minAt != lastMin && (maxLoop-- > 0));

    rminScore = minScore;
    // The sub-pixel fit needs the 3x3 neighbourhood; at the edge of the search area use the integer position.
    if(!sarea.shrink(1).contains(minAt)) {
      rat = toPoint<RealT>(RealT(minAt[0]), RealT(minAt[1]));
      return minScore;
    }
    for(int dr = -1; dr <= 1; ++dr)
      for(int dc = -1; dc <= 1; ++dc)
        (void)scoreAt(minAt + Index<2>(dr, dc));
    // (The sub-pixel estimate used to be overwritten by the integer position straight away.)
    const Point2f sub = LocatePeakSubPixel(scoreMap, minAt, 0.25f);
    rat = toPoint<RealT>(RealT(sub[0]), RealT(sub[1]));
    return minScore;
  }

}// namespace Ravl2

