//
// Created by charles galambos on 04/08/2024.
//

#include "Ravl2/Qt/Geometry.hh"

namespace Ravl2
{
  template auto toQPolygon<int>(const Polygon<int> &polygon);
  template auto toQPolygon<float>(const Polygon<float> &polygon);
  template auto toQPolygon<double>(const Polygon<double> &polygon);

}// namespace Ravl2