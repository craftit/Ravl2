// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_PIXELCLASSIFYGREY_HEADER
#define RAVLIMAGE_PIXELCLASSIFYGREY_HEADER
//////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! example=exSegmentation.cc
//! file="Ravl/Image/Processing/Segmentation/PixelClassifyGrey.hh"
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Ratna Rambaruth"
//! date="12/06/1998"

#include "Ravl/Index2d.hh"
#include "Ravl/Math.hh"

namespace RavlN
{

  //! userlevel=Normal
  //: Class to determine whether a pixel belongs to a region based on its intensity.

  template <class ValT, class StatT>
  class PixelClassifyGreyC
  {
  public:
    PixelClassifyGreyC()
    {
      threshold = 0;
    }
    //: default constructor

    PixelClassifyGreyC(ValT thr)
    {
      threshold = thr;
    }
    //: Constructor

    inline void Initialise()
    {
      mean = 0, count = 0;
      total = 0;
    }
    //: Resets the statistics for the region

    inline bool Contains(const Index2dC &pxl, const ValT &val) const
    {
      return (Abs((int)val - mean) < threshold);
    }
    //: Return true if pxl belongs to the region

    inline void Include(const Index2dC &pxl, const ValT &val)
    {
      count++;
      total += val;
      mean = total / count;
    }
    //: Update the statistics of the region for the included pixel

    inline StatT Stat()
    {
      return mean;
    }
    //: Return the statistics about the region
  protected:
    int total, count;
    int mean;
    ValT threshold;
  };

}// namespace RavlImageN
#endif
