// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Charles Galambos"
//! file="Ravl/Image/Processing/Tracking/MatchNormalisedCorrelation.cc"

#include "Ravl/Image/MatchNormalisedCorrelation.hh"
#include "Ravl/Sums1d2.hh"
#include "Ravl/Image/Rectangle2dIter.hh"
#include "Ravl/Vector2d.hh"

namespace RavlImageN {
  
  //: 'img' is the image to search.
  
  MatchNormalisedCorrelationC::MatchNormalisedCorrelationC(const ImageC<ByteT> &img) 
    : threshold(10)
  {
    SetSearchImage(img);
  }
  
  //: Default constructor.
  
  MatchNormalisedCorrelationC::MatchNormalisedCorrelationC()
    : threshold(10)
  {}
  
  //: Setup search image.
  
  bool MatchNormalisedCorrelationC::SetSearchImage(const ImageC<ByteT> &img) {
    searchImg = img;
    sums.BuildTable(img);
    return true;
  }

  IntT SumOfProducts(const RangeBufferAccess2dC<ByteT> &templ,const RangeBufferAccess2dC<ByteT> &subImg) {
    IntT sumxy = 0;
    // The following loop could probably be speeded up with some MMX code.
    for(BufferAccess2dIter2C<ByteT,ByteT> it2(templ,templ.Range2(),
					      subImg,subImg.Range2());it2;it2++)
      sumxy += (IntT) it2.Data1() * it2.Data2();
    return sumxy;
  }
  
  //: The location in the image most likely to match the template.
  // Returns false if no likely match is found.
  
  bool MatchNormalisedCorrelationC::Search(const Array2dC<ByteT> &templ,
					   const IndexRange2dC &searchArea,
					   RealT &score,Index2dC &at) const {
    score = 0;
    Sums1d2C tsums;
    for(Array2dIterC<ByteT> it(templ);it;it++) 
      tsums += *it;
    RealT tarea = (RealT) tsums.Size();
    MeanVarianceC smv = tsums.MeanVariance();
    RealT tmean = smv.Mean();
    RealT tvar = smv.Variance();
    RealT z = Sqrt(tvar) * tarea;
    RealT tsum = tsums.Sum();
    
    IndexRange2dC clippedSearchArea = searchArea;
    clippedSearchArea.ClipBy(searchImg.Frame());
    
    for(Rectangle2dIterC itr(clippedSearchArea,templ.Frame());itr;itr++) {
      IndexRange2dC rect = itr.Window();
      RangeBufferAccess2dC<ByteT> subImg(searchImg,rect);
      IntT sumxy = SumOfProducts(templ,subImg);
      
      // Calculate mean and variance for search image.
      TFVectorC<IntT, 2>  rs = sums.Sum(rect);
      RealT ssum = rs[0];
      RealT smean = (RealT) rs[0] / tarea;
      RealT svar = ((RealT) rs[1] - Sqr((RealT) rs[0])/tarea) / (tarea-1);
      
      // Compute correlation score.
      RealT curScore = 
	(sumxy - smean * tsum - tmean * ssum + smean * tmean * tarea)
	/ (Sqrt(svar) * z);
      //cerr << " " << curScore;
      // Compair it to pervious scores.
      if(curScore > score) { // Best so far ?
	score = curScore;
	at = itr.Window().Center();
      }
    }
    
    return score > threshold;
  }

}
