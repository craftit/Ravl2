//
// Created by charles on 12/09/24.
//

#include "Ravl2/Geometry/FitAffine.hh"

namespace Ravl2
{
  template Affine<float, 2> meanScaleToAffine<float, 2>(const Point<float, 2> &mean, float scale);
  template bool fit<float>(Affine<float, 2> &affine,
			  const Point<float, 2> &p1b, const Point<float, 2> &p1a,
			  const Point<float, 2> &p2b, const Point<float, 2> &p2a,
			  const Point<float, 2> &p3b, const Point<float, 2> &p3a);


}