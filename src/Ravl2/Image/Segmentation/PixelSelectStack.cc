// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Segmentation/PixelSelectStack.cc"

#include "Ravl/Image/PixelSelectStack.hh"

namespace RavlImageN {
  
  PixelSelectStackC::PixelSelectStackC(const ImageRectangleC & rect) 
    : label(0)
  { 
    push = ImageC<UIntT>(rect);
    push.Fill(0); 
  }
  //: Constructor form the bounding rectangle of possible pixels
  
  void PixelSelectStackC::Initialise()
  { label++; pxls.Empty(); }
  //: Resets the pixel selector
  
}  
