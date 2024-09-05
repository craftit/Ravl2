// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_MATCHPATCH_HEADER
#define RAVLIMAGE_MATCHPATCH_HEADER 1
///////////////////////////////////////////////////////////////////////////////////////
//! author="Charles Galambos"

#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Image/SquareIter.hh"
#include "Ravl2/Image/Matching.hh"

namespace Ravl2
{

  template <typename DataT, typename SumT, typename RealT = float>
  SumT SearchMinAbsDifferenceCreep(const Array<DataT, 2> &patch, const Array<DataT, 2> &img, const Index<2> &start, Point<RealT, 2> &rat, SumT &rminScore, int searchSize = 50)
  {
    SumT minScore;
    IndexRange<2> sarea = IndexRange<2>(start,start).expand(searchSize + 8);
    sarea.clipBy(img.range() - patch.range());
    //cerr << "Img=" << img.range() << " SArea=" << sarea << "\n";
    if(!sarea.contains(start)) {
      rat = Point<RealT, 2>(-1, -1);
      minScore = rminScore = 100000000;
      return minScore;
    }
    Array<SumT, 2> scoreMap(sarea);
    scoreMap.fill(-1);
    MatchSumAbsDifference(patch, img, start, minScore);
    Index<2> minAt = start;
    scoreMap[minAt] = minScore;

    Index<2> lastMin;
    int maxLoop = 10;
    do {
      //cerr << "Center at " << minAt << "\n";
      lastMin = minAt;
      for(SquareIterC it(searchSize, minAt); it; it++) {
        if(!sarea.contains(*it) || scoreMap[*it] > 0)
          continue;
        SumT score;
        //cerr << "Checking " << *it << "\n";
        MatchSumAbsDifference(patch, img, *it, score);
        scoreMap[*it] = score;
        if(score < minScore) {
          minScore = score;
          minAt = *it;
        }
      }
    } while(minAt != lastMin && (maxLoop-- > 0));
    rat = LocatePeakSubPixel(scoreMap, minAt, 0.25);
    rat = minAt;
    rminScore = minScore;
    return minScore;
  }

}// namespace Ravl2

#endif
