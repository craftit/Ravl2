
#include "Ravl2/Geometry/Translate.hh"

namespace Ravl2
{
  static_assert(PointTransform<Translate<float, 2>>, "Translate<int,2> does not satisfy PointTransform");
  static_assert(PointTransform<Translate<float, 3>>, "Translate<int,3> does not satisfy PointTransform");

  template class Translate<float, 2>;
  template class Translate<float, 3>;
}// namespace Ravl2