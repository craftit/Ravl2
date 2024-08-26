//
// Created by charles galambos on 04/08/2024.
//

#include "Ravl2/Qt/Geometry.hh"

namespace Ravl2
{
  template auto toQPolygon<int>(const Polygon2dC<int> &polygon);
  template auto toQPolygon<float>(const Polygon2dC<float> &polygon);
  template auto toQPolygon<double>(const Polygon2dC<double> &polygon);

}// namespace Ravl2