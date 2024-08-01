
#include "Ravl2/Geometry/Affine.hh"

namespace Ravl2
{
  static_assert(PointTransform<Affine<float, 2>>, "Affine<int,2> does not satisfy PointTransform");
  static_assert(PointTransform<Affine<float, 3>>, "Affine<int,3> does not satisfy PointTransform");

  template class Affine<float, 2>;
  template class Affine<float, 3>;
}// namespace Ravl2