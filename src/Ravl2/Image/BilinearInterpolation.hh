// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_BILINEAR_HEADER
#define RAVLIMAGE_BILINEAR_HEADER
//! author="Charles Galambos"
//! date="24/01/2001"

#include <cstdint>
#include "Ravl2/Math.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  //! @brief Pixel coordinate system
  //! There are two types of pixel coordinates one uses the center of the pixel as the
  //! coordinate and the other uses the top left corner of the pixel as the coordinate. These
  //! have a 0.5 pixel offset.  The enum tells us which one to use.
  enum class PixelCoordinateSystemT
  {
    Center,
    TopLeft
  };

  //! @brief Bilinear interpolation
  //! This function performs bi-linear interpolation on a 2D image, it does not perform any bounds checking.
  //! It also assumes the image is continuous in memory, with the first index being the row and the second index being the column.
  //! \param img - The input image
  //! \param pnt - The point in the i/p image coord system for which the interpolated value is required.
  //! \return The interpolated value.

  template <typename ArrayT, typename Point2T>
    requires WindowedArray<ArrayT, typename ArrayT::value_type, ArrayT::dimensions>
  [[nodiscard]] constexpr auto interpolateBilinear(const ArrayT &img, Point2T pnt)
  {
    const auto px = pnt[0];
    const auto py = pnt[1];
    const auto fx = std::floor(px);// Row
    const auto fy = std::floor(py);// Col
    const auto u = px - fx;
    const auto t = py - fy;
    const int ix = int(fx);
    const int iy = int(fy);
    const auto *pixel1 = &((img)[ix][iy]);
    const auto *pixel2 = &((img)[ix + 1][iy]);
    const auto onemu = (1.0f - u);
    const auto onemt = (1.0f - t);
    return (pixel1[0] * (onemt * onemu)) + (pixel1[1] * (t * onemu)) + (pixel2[0] * (onemt * u)) + (pixel2[1] * (t * u));
  }

}// namespace Ravl2

#endif
