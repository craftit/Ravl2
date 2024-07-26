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
//! rcsid="$Id$"
//! lib=RavlImageProc
//! docentry="Ravl.API.Images.Tracking"
//! file="Ravl/Image/Processing/Tracking/MatchPatch.hh"

#include "Ravl/Image/PeakDetector.hh"
#include "Ravl/SquareIter.hh"
#include "Ravl/Image/Matching.hh"

namespace RavlImageN {
  
  template<class DataT,class SumT>
  SumT SearchMinAbsDifferenceCreep(const Array2dC<DataT> &patch,const Array2dC<DataT> &img,const Index2dC &start,Point2dC &rat,SumT &rminScore,int searchSize = 50) {
    SumT minScore;
    IndexRange2dC sarea(start,searchSize+8,searchSize+8);
    sarea.ClipBy(img.Frame() - patch.Frame());
    //cerr << "Img=" << img.Frame() << " SArea=" << sarea << "\n";
    if(!sarea.Contains(start)) {
      rat = Point2dC(-1,-1);
      minScore = rminScore = 100000000;
      return minScore;
    }
    Array2dC<SumT> scoreMap(sarea);
    scoreMap.Fill(-1);
    MatchSumAbsDifference(patch,img,start,minScore);
    Index2dC minAt = start;
    scoreMap[minAt] = minScore;
    
    Index2dC lastMin;
    int maxLoop = 10;
    do {
      //cerr << "Center at " << minAt << "\n";
      lastMin = minAt;
      for(SquareIterC it(searchSize,minAt);it;it++) {
	if(!sarea.Contains(*it) || scoreMap[*it] > 0)
	  continue;
	SumT score;
	//cerr << "Checking " << *it << "\n";
	MatchSumAbsDifference(patch,img,*it,score);
	scoreMap[*it] = score;
	if(score < minScore) {
	  minScore = score;
	  minAt = *it;
	}
      }
    } while(minAt != lastMin && (maxLoop-- > 0));
    rat = LocatePeakSubPixel(scoreMap,minAt,0.25);
    rat = minAt;
    rminScore = minScore;
    return minScore;
  }
  
  
  
}


#endif
