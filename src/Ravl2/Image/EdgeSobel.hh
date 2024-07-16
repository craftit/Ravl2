// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL2_EDGESOBEL_HEADER
#define RAVL2_EDGESOBEL_HEADER 1

#include "Ravl2/Array.hh"
#include <stdexcept>

namespace Ravl2 {
  
  //! Sobel edge detection.
  // The sign convention is: a +ve gradient computed if image intensity is
  // increasing in +ve direction in coordinate system.
  //
  // A vertical (horizontal) edge is defined as an edge in which the
  // <i>gradient is changing</i> in a vertical (horizontal) direction.

  
  //! Apply Sobel operator to 'img', put result in 'out'
  // Output gradient vector is in the order: vertical, horizontal gradients

  template<class DataInT,class DataOutT>
  void edge_sobel(const Array<DataInT,2> &img, Array<DataOutT,3> &out)
  {
    IndexRange<3> outRange(img.range().shrink(1),2);
    if(outRange != out.range())
      out = Array<DataOutT,3>(outRange);
    for(int r : outRange[0]) {
      for(int c : outRange[1]) {
        out[r][c][0] = img[r+1][c-1] + img[r+1][c] * 2 + img[r+1][c-1] - img[r-1][c-1] - img[r-1][c] * 2 - img[r-1][c+1];
        out[r][c][1] = img[r-1][c+1] + img[r][c+1] * 2 + img[r+1][c+1] - img[r-1][c-1] - img[r][c-1] * 2 - img[r+1][c-1];
      }
    }
  }


  //! Apply Sobel operator to 'img', put vertical and horizontal gradients in "Vert" and "Horz" respectively
  template<class DataInT,class DataOutT>
  void edge_sobel(const Array<DataInT,2> &img, Array<DataOutT,2> &outRow, Array<DataOutT,2> &outCol)
  {
    IndexRange<2> outRange = img.range().shrink(1);
    if(outRange != outRow.range())
      outRow = Array<DataOutT,2>(outRange);
    if(outRange != outCol.range())
      outCol = Array<DataOutT,2>(outRange);
    for(int r : outRange[0]) {
      for(int c : outRange[1]) {
        outRow[r][c] = img[r+1][c-1] + img[r+1][c] * 2 + img[r+1][c-1] - img[r-1][c-1] - img[r-1][c] * 2 - img[r-1][c+1];
        outCol[r][c] = img[r-1][c+1] + img[r][c+1] * 2 + img[r+1][c+1] - img[r-1][c-1] - img[r][c-1] * 2 - img[r+1][c-1];
      }
    }
  }
  
}

#endif
