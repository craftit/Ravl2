
#include "Ravl2/Geometry/ScaleTranslate.hh"

namespace Ravl2
{
  static_assert(PointTransform<ScaleTranslate<float, 2>>, "ScaleTranslate<int,2> does not satisfy PointTransform");
  static_assert(PointTransform<ScaleTranslate<float, 3>>, "ScaleTranslate<int,3> does not satisfy PointTransform");

  template class ScaleTranslate<float, 2>;
  template class ScaleTranslate<float, 3>;
}// namespace Ravl2