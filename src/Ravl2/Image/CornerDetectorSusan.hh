// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos based on code by S.M.Smith"
#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Image/Corner.hh"
#include "Ravl2/Image/CornerDetector.hh"

namespace Ravl2
{

  //! userlevel=Develop
  //: Susan corner detector.
  // Based on code from S.M.Smith

  class CornerDetectorSusanC
    : public CornerDetectorC
  {
  public:
    CornerDetectorSusanC(int threshold = 20);
    //: Constructor.
    // threshold = Minimum level of cornerness to accept. <br>
    // w = width of filter to use for corners.

    DListC<CornerC> Apply(const ImageC<ByteT> &img);
    //: Get a list of corners from 'img'

  protected:
    void SetupLUT(int form);
    //: Setup LUT.

    DListC<CornerC> Corners(const ImageC<ByteT> &img,ImageC<IntT> &cornerMap);
    //: Generate a corner map and a list of non zero components.

    void Peaks(DListC<CornerC> &list,const ImageC<IntT> &cornerMap);
    //: Remove non-maximal peaks.

  private:
    ByteT Lut[516]; // Brightness LUT.
    ByteT *bp;
    int threshold;
  };

}

