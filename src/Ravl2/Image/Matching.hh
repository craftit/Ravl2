// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once
//! author="Charles Galambos"

#include <tuple>
#include "Ravl2/Array.hh"
#include "Ravl2/ScanWindow.hh"

namespace Ravl2 {

  //! Compute the sum of absolute differences between two images.
  //! \param imgTemplate is the template to match.
  //! \param img is the image we're searching.
  //! \param origin is the position in the image to check.
  template<class DataT,class SumT>
  SumT matchSumAbsDifference(const Array<DataT,2> &imgTemplate,
                             const Array<DataT,2> &img)
  {
    SumT diff = 0;
    for(auto it = begin(imgTemplate,img); it.valid();) {
      do {
        diff += std::abs(SumT(it.template data<0>()) - SumT(it.template data<1>()));
      } while (it.next());
    }
    return diff;
  }

#if RAVL_USE_MMX
  //! Compute the sum of absolute differences between two images.
  //! Use some MMX code to speed this up.
  IntT matchSumAbsDifference(const Array<ByteT,2> &imgTemplate,
			     const Array<ByteT,2> &img
			     );
#endif

  //! Search img for template 'tmpl' with the minimum absolute difference.
  //! The position is the pixel in 'img' corresponding to 0,0 in 'imgTemplate'.

  template<class DataT,class SumT>
  std::tuple<Index<2>,SumT> searchMinAbsDifference(const Array<DataT,2> &tmpl, const Array<DataT,2> &img)
  {
    SumT minScore = std::numeric_limits<SumT>::max();
    Index<2> at;
    for(ScanWindow<DataT,2> tmplWin(img, tmpl.range());tmplWin.valid();) {
      do {
        auto score = matchSumAbsDifference(tmpl, img);
        if (score < minScore) {
          minScore = score;
          at = tmplWin.index();
        }
      } while (tmplWin.next());
    }
    return std::make_tuple(at, minScore);
  }

}

