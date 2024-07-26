// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_MATCHING_HEADER
#define RAVLIMAGE_MATCHING_HEADER 1
/////////////////////////////////////////////////////////////
//! rcsid="$Id$"
//! userlevel=Normal
//! author="Charles Galambos"
//! docentry="Ravl.API.Images.Misc"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Filters/Matching/Matching.hh"

#include "Ravl/Image/Image.hh"
#include "Ravl/BfAcc2Iter2.hh"
#include "Ravl/Image/Rectangle2dIter.hh"

namespace RavlImageN {
  
  template<class DataT,class SumT>
  SumT MatchSumAbsDifference(const Array2dC<DataT> &imgTemplate,
			     const Array2dC<DataT> &img,
			     const Index2dC &origin,
			     SumT &diff
			     ) {
    SetZero(diff);
    IndexRange2dC srect(imgTemplate.Frame());
    srect += origin;
    RavlAssert(img.Frame().Contains(srect)); 
    RangeBufferAccess2dC<ByteT> subImg(img,srect); 
    for(BufferAccess2dIter2C<DataT,DataT> it(imgTemplate,imgTemplate.Range2(),
					     subImg,subImg.Range2());it;it++)
      diff += Abs((SumT) it.Data1() - (SumT) it.Data2());
    return diff;
  }
  //: Compute the sum of absolute differences between two images.
  // 'imgTemplate' is the template to match. <br>
  // 'img' is the image we're searching. <br>
  // 'origin' is the position in the image to check.<br>
  // 'diff' is used to accumulate the differences between the images.

#if RAVL_USE_MMX
  IntT MatchSumAbsDifference(const Array2dC<ByteT> &imgTemplate,
			     const Array2dC<ByteT> &img,
			     const Index2dC &origin,
			     IntT &diff
			     );
  //: Compute the sum of absolute differences between two images.
  //: Use some MMX code to speed this up.
#endif
  
  template<class DataT,class SumT>
  SumT SearchMinAbsDifference(const Array2dC<DataT> &tmpl,const Array2dC<DataT> &img,const IndexRange2dC &area,Index2dC &at,SumT &rminScore) {
    SumT minScore = 1000000;
    IndexRange2dC sarea(area);
    sarea.ClipBy(img.Frame());
    SumT score;
    Rectangle2dIterC it(sarea,tmpl.Frame());
    if(!it)
      return minScore;
    Index2dC off = tmpl.Frame().Origin();
    MatchSumAbsDifference(tmpl,img,it.Window().Origin() - off,minScore);
    at = it.Window().Origin() - off;
    for(it++;it;it++) {
      Index2dC tryAt = it.Window().Origin() - off;
      MatchSumAbsDifference(tmpl,img,tryAt,score);
      if(score < minScore) {
	minScore = score;
	at = tryAt;
      }
    }
    rminScore = minScore;
    return minScore;
  }
  //: Search img for template 'tmpl' with the minimum absolute difference.
  // Returns the score minum score found, and sets 'at' to the position
  // it occured at.   The position is the pixel in 'img' corresponding to 
  // 0,0 in 'imgTemplate'.
  

  
}

#endif
