// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#include "Ravl2/Image/MatchNormalisedCorrelation.hh"
#include "Ravl2/ScanWindow.hh"
#include "Ravl2/Math/Sums1d2.hh"

namespace Ravl2
{

  //: 'img' is the image to search.

  MatchNormalisedCorrelationC::MatchNormalisedCorrelationC(const Array<uint8_t, 2> &img)
      : threshold(10)
  {
    SetSearchImage(img);
  }

  //: Default constructor.

  MatchNormalisedCorrelationC::MatchNormalisedCorrelationC()
      : threshold(10)
  {}

  //: Setup search image.

  bool MatchNormalisedCorrelationC::SetSearchImage(const Array<uint8_t, 2> &img)
  {
    searchImg = img;
    sums.BuildTable(img);
    return true;
  }

  int SumOfProducts(Array<uint8_t, 2> templ, const ArrayAccess<uint8_t, 2> &subImg)
  {
    int sumxy = 0;
    // The following loop could probably be speeded up with some MMX code.
    for(auto it2 = zip(templ,subImg); it2.valid(); ) {
      do {
	sumxy += it2.data<0>() * it2.data<1>();
      } while(it2.next());
    }
    return sumxy;
  }

  //: The location in the image most likely to match the template.
  // Returns false if no likely match is found.

  bool MatchNormalisedCorrelationC::Search(const Array<uint8_t, 2> &templ,
                                           const IndexRange<2> &searchArea,
                                           RealT &score, Index<2> &at) const
  {
    score = 0;
    Sums1d2C<int32_t> tsums;
    for(auto it : templ)
      tsums += it;
    RealT tarea = RealT(tsums.size());
    MeanVariance smv = toMeanVariance<float>(tsums,SampleStatisticsT::POPULATION);
    RealT tmean = smv.mean();
    RealT tvar = smv.variance();
    RealT z = std::sqrt(tvar) * tarea;
    RealT tsum = RealT(tsums.sum());

    IndexRange<2> clippedSearchArea = searchArea;
    clippedSearchArea.clipBy(searchImg.range());

    for(ScanWindow itr(searchImg, templ.range()); itr.valid(); ++itr) {
      auto sumxy = SumOfProducts(templ, itr.window());

      // Calculate mean and variance for search image.
      Vector<int, 2> rs = sums.Sum(itr.windowRange());
      auto ssum = RealT(rs[0]);
      RealT smean = RealT(rs[0]) / tarea;
      RealT svar = (RealT(rs[1]) - sqr(RealT(rs[0])) / tarea) / (tarea - 1);

      // Compute correlation score.
      RealT curScore =
        (RealT(sumxy) - smean * tsum - tmean * ssum + smean * tmean * tarea)
        / (std::sqrt(svar) * z);
      //cerr << " " << curScore;
      // Compair it to pervious scores.
      if(curScore > score) {// Best so far ?
        score = curScore;
        at = itr.index();
      }
    }

    return score > threshold;
  }

}// namespace Ravl2
