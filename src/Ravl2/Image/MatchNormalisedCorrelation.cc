// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#include "Ravl2/Image/MatchNormalisedCorrelation.hh"
#include "Ravl2/Math/Sums1d2.hh"

namespace Ravl2
{

  //: 'img' is the image to search.

  MatchNormalisedCorrelationC::MatchNormalisedCorrelationC(const Array<ByteT, 2> &img)
      : threshold(10)
  {
    SetSearchImage(img);
  }

  //: Default constructor.

  MatchNormalisedCorrelationC::MatchNormalisedCorrelationC()
      : threshold(10)
  {}

  //: Setup search image.

  bool MatchNormalisedCorrelationC::SetSearchImage(const Array<ByteT, 2> &img)
  {
    searchImg = img;
    sums.BuildTable(img);
    return true;
  }

  int SumOfProducts(const RangeBufferAccess2dC<ByteT> &templ, const RangeBufferAccess2dC<ByteT> &subImg)
  {
    int sumxy = 0;
    // The following loop could probably be speeded up with some MMX code.
    for(BufferAccess2dIter2C<ByteT, ByteT> it2(templ, templ.range(1),
                                               subImg, subImg.range(1));
        it2; it2++)
      sumxy += (int)it2.data<0>() * it2.data<1>();
    return sumxy;
  }

  //: The location in the image most likely to match the template.
  // Returns false if no likely match is found.

  bool MatchNormalisedCorrelationC::Search(const Array<ByteT, 2> &templ,
                                           const IndexRange<2> &searchArea,
                                           RealT &score, Index<2> &at) const
  {
    score = 0;
    Sums1d2C tsums;
    for(Array2dIterC<ByteT> it(templ); it; it++)
      tsums += *it;
    RealT tarea = (RealT)tsums.size();
    MeanVariance smv = tsums.MeanVariance();
    RealT tmean = smv.Mean();
    RealT tvar = smv.Variance();
    RealT z = std::sqrt(tvar) * tarea;
    RealT tsum = tsums.Sum();

    IndexRange<2> clippedSearchArea = searchArea;
    clippedSearchArea.clipBy(searchImg.range());

    for(Rectangle2dIterC itr(clippedSearchArea, templ.range()); itr; itr++) {
      IndexRange<2> rect = itr.Window();
      RangeBufferAccess2dC<ByteT> subImg(searchImg, rect);
      int sumxy = SumOfProducts(templ, subImg);

      // Calculate mean and variance for search image.
      Vector<int, 2> rs = sums.Sum(rect);
      RealT ssum = rs[0];
      RealT smean = (RealT)rs[0] / tarea;
      RealT svar = ((RealT)rs[1] - sqr((RealT)rs[0]) / tarea) / (tarea - 1);

      // Compute correlation score.
      RealT curScore =
        (sumxy - smean * tsum - tmean * ssum + smean * tmean * tarea)
        / (std::sqrt(svar) * z);
      //cerr << " " << curScore;
      // Compair it to pervious scores.
      if(curScore > score) {// Best so far ?
        score = curScore;
        at = itr.Window().Center();
      }
    }

    return score > threshold;
  }

}// namespace Ravl2
