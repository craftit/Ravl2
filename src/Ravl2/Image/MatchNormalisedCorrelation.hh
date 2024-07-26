// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_MATCHNORMALISEDCORRELATION_HEADER
#define RAVLIMAGE_MATCHNORMALISEDCORRELATION_HEADER 1
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Charles Galambos"
//! date="28/11/2002"
//! docentry="Ravl.API.Images.Tracking;Ravl.API.Images.Correlation"
//! file="Ravl/Image/Processing/Tracking/MatchNormalisedCorrelation.hh"

#include "Ravl/Image/Image.hh"
#include "Ravl/Image/SummedAreaTable2.hh"

namespace RavlImageN {
  
  //! userlevel=Normal
  //: Do a grid search for the position of the best match using normalised correlation.
  
  class MatchNormalisedCorrelationC {
  public:
    MatchNormalisedCorrelationC(const ImageC<ByteT> &img);
    //: 'img' is the image to search.
    
    MatchNormalisedCorrelationC();
    //: Default constructor.
    
    bool SetSearchImage(const ImageC<ByteT> &img);
    //: Setup search image.
    // This precomputes some information about the image we're doing tracking in.
    
    bool Search(const Array2dC<ByteT> &templ,
                const IndexRange2dC &searchArea,
		RealT &score,
                Index2dC &at
                ) const;
    //: The location in the image most likely to match the template.
    //!param: templ - Template to search.
    //!param: searchArea - Bounds within which to search. Top left of this rectangle is the top left of the template rectangle.
    //!param: score - Variable to hold the maximum correlation score.
    //!param: at - Position of maximum value.
    // Returns false if no likely match is found.
    
  protected:
    RealT threshold;
    ImageC<ByteT> searchImg;
    SummedAreaTable2C<IntT> sums; // Sums for searchImg
  };
}





#endif
