// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_SEGMENTREGION_HEADER
#define RAVLIMAGE_SEGMENTREGION_HEADER
//////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Develop
//! file="Ravl/Image/Processing/Segmentation/SegmentRegion.hh"
//! rcsid="$Id$"
//! lib=RavlImageProc
//! example=exSegmentation.cc
//! author="Ratna Rambaruth"
//! date="12/06/1998"

#include "Ravl2/Array.hh"

namespace Ravl2 {

  template<class StatT> class RegionSetC;
  
  //! userlevel=Develop
  //: Virtual segmentation class (provides interface for real classes)
  
  template<class PixelT, class StatT>
  class SegmentRegionBodyC 
    : public RCBodyVC 
  { 
  public: 
    SegmentRegionBodyC() {}
    //: Default constructor.
    
    virtual RegionSetC<StatT> Apply(const ArrayAccess<PixelT,2> &in) {
      std::cout << "This function cannot be used\n";
      return RegionSetC<StatT>();
    }
    //: Perform segmentation on "in"
  };
  

}
#endif




