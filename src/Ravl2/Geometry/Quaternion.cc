//
// Created by charles galambos on 22/10/2023.
//

#include "Ravl2/Geometry/Quaternion.hh"

namespace Ravl2
{
  static_assert(PointTransform<Quaternion<float> >, "Quaternion<float> does not satisfy PointTransform");

  template class Quaternion<float>;

}
