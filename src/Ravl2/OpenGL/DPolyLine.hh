//
// Created by charles galambos on 28/01/2025.
//

#pragma once

#include "Ravl2/Geometry/PointSet.hh"
#include "Ravl2/OpenGL/DObject.hh"

namespace Ravl2 {

  //! A 3D polyline.
  //!

  class DPolyLine
    : public DObject
  {
  public:
    DPolyLine() = default;


  private:
    std::vector<PointSet<float,3> > mPoints; //!< The points.
    std::vector<PixelRGB8> mColours; //!< The colours for each line
  };

} // Ravl2
