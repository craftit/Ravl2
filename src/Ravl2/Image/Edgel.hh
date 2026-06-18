// This file is part of RAVL2, Recognition And Vision Library 2
// Clean-room reimplementation (MIT) of the original RAVL edgel/edge-linking
// design by George Matas, Radek Marik and Charles Galambos.

#pragma once

#include <cmath>
#include "Ravl2/Types.hh"

namespace Ravl2
{
  //! @brief Edge element: image location, intensity gradient and magnitude.
  //! @details Location is (row, col). The gradient is (dRow, dCol); the edge tangent is
  //! perpendicular to it. Produced by detectEdgeChains() as ordered, spatially-contiguous chains.
  struct Edgel
  {
    Point<float, 2> at = Point<float, 2>::Zero();    //!< location (row, col)
    Vector<float, 2> grad = Vector<float, 2>::Zero();//!< intensity gradient (dRow, dCol)
    float magnitude = 0;                             //!< gradient magnitude

    //! Gradient orientation in radians, atan2(dRow, dCol). The edge runs perpendicular to this.
    [[nodiscard]] float gradientAngle() const { return std::atan2(grad[0], grad[1]); }
  };

}// namespace Ravl2
