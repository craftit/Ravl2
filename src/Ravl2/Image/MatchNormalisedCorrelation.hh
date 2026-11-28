// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="28/11/2002"

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Image/SummedAreaTable2.hh"

namespace Ravl2
{

  //: Do a grid search for the position of the best match using normalised correlation.

  class MatchNormalisedCorrelation
  {
  public:
    using RealT = float;

    //! 'img' is the image to search.
    explicit MatchNormalisedCorrelation(const Array<uint8_t, 2> &img);

    //! Default constructor.
    MatchNormalisedCorrelation();

    //! Setup search image.
    // This precomputes some information about the image we're doing tracking in.
    bool SetSearchImage(const Array<uint8_t, 2> &img);

    //! The location in the image most likely to match the template.
    //!param: templ - Template to search.
    //!param: searchArea - Bounds within which to search. Top left of this rectangle is the top left of the template rectangle.
    //!param: score - Variable to hold the maximum correlation score.
    //!param: at - Position of maximum value.
    // Returns false if no likely match is found.
    //! A match is reported when the best score exceeds the threshold.
    bool Search(const Array<uint8_t, 2> &templ,
                const IndexRange<2> &searchArea,
                RealT &score,
                Index<2> &at) const;

    //! Minimum normalised correlation (in [-1, 1]) for Search() to report a match. Default 0.5.
    void SetThreshold(RealT newThreshold)
    {
      threshold = newThreshold;
    }

  protected:
    RealT threshold = 0.5f;
    Array<uint8_t, 2> searchImg;
    SummedAreaTable2C<int64_t> sums;// Sums for searchImg; 64-bit so the sum of squares doesn't overflow
  };
}// namespace Ravl2
