// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_PIXSTACKSELECT_HEADER
#define RAVLIMAGE_PIXSTACKSELECT_HEADER 1
//////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! example=exSegmentation.cc
//! file="Ravl/Image/Processing/Segmentation/PixelSelectStack.hh"
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Ratna Rambaruth"
//! date="12/06/1998"

#include "Ravl/Stack.hh"
#include "Ravl/Image/Image.hh"

namespace RavlImageN
{

  //! userlevel=Normal
  //: Class for selecting next pixel on which some processing needs to be done.

  class PixelSelectStackC
  {
  public:
    PixelSelectStackC()
    {}
    //: Default constructor

    PixelSelectStackC(const ImageRectangleC &rect);
    //: Constructor form the bounding rectangle of possible pixels

    void Initialise();
    //: Resets the pixel selector

    inline Index2dC Next()
    {
      return pxls.Pop();
    }
    //: Next pixel to be processed

    inline void Include(const Index2dC &pxl)
    {
      pxls.Push(pxl);
      push[pxl] = label;
    }
    //: Add a pixel for later processing

    inline bool IsEmpty()
    {
      return pxls.IsEmpty();
    }
    //: Returns true if no pixels need to be processed

    inline bool IsInside(const Index2dC &pxl)
    {
      return push[pxl] == label;
    }
    //: returns true if pxl has already been included for processing

  protected:
    UIntT label;
    StackC<Index2dC> pxls;
    ImageC<UIntT> push;
  };

}// namespace RavlImageN
#endif
