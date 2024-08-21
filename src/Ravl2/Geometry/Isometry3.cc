//
// Created by charles galambos on 21/08/2024.
//

#include "Ravl2/Geometry/Isometry3.hh"

namespace Ravl2
{
  static_assert(PointTransform<Isometry3<float>>, "Isometry3<float> does not satisfy PointTransform");

  template class Isometry3<float>;

}// namespace Ravl2