// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="24/01/2001"

#pragma once

#include <cstdint>
#include "Ravl2/Math.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{

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

  //! @brief Bilinear interpolation
  //! This operation is used to assign the pixel value from the source to the target
  //! This does not perform any bounds checking.
  //! @param Source The target pixel to assign to
  //! @param pnt The source pixel to assign from
  template <typename SourceT, typename PointT>
    requires WindowedArray<SourceT, typename SourceT::value_type, SourceT::dimensions>
  struct InterpolateBilinear
  {
    constexpr auto operator()(const SourceT &source, const PointT &pnt) const
    {
      return interpolateBilinear(source, pnt);
    }
  };

  //! @brief Compute the bounds we can actually interpolate over
  //! @param targetRange The range iterate over in a grid
  //! @param interpolator The interpolator to use
  //! @return The range of real points we can sample from
  template <typename CoordTypeT, unsigned N, typename SourceT, typename PointT>
  Range<CoordTypeT,N> interpolationBounds(const IndexRange<N> &indexRange, [[maybe_unused]] const InterpolateBilinear<SourceT,PointT> &interpolator)
  {
    Range<CoordTypeT,N> targetRange = toRange<CoordTypeT>(indexRange);
    // We reduce the range by 1 plus the machine epsilon times the maximum image size to avoid sampling
    // outside the image.
    return targetRange.shrinkMax(CoordTypeT(1) + (std::numeric_limits<CoordTypeT>::epsilon() * 4096));
  }


}// namespace Ravl2

