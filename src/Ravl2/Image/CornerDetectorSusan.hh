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

namespace Ravl2
{

  //: Susan corner detector.
  // Based on code from S.M.Smith

  class CornerDetectorSusan
  {
  public:
    using RealT = CornerC::RealT;
    using ByteT = uint8_t;

    //! Constructor.
    //! threshold = Minimum level of cornerness to accept. <br>
    explicit CornerDetectorSusan(int threshold = 20);

    [[nodiscard]] std::vector<CornerC> apply(const Array<ByteT, 2> &img) const;
    //: Get a list of corners from 'img'

  protected:
    //! Setup LUT.
    void SetupLUT(int form);

    //! Generate a corner map and a list of non zero components.
    [[nodiscard]] std::vector<CornerC> Corners(const Array<ByteT,2> &img,Array<int,2> &cornerMap) const;

    //! Remove non-maximal peaks.
    static void Peaks(std::vector<CornerC> &list,const Array<int,2> &cornerMap) ;

  private:
    std::array<uint8_t,516> Lut; // Brightness LUT.
    const uint8_t *bp = nullptr;
    int threshold = 20;
  };

}

