//
// Created by charles galambos on 06/08/2024.
//

#include "Ravl2/Image/DrawCircle.hh"

namespace Ravl2
{
  //! Instance some common types.

  template void DrawCircle<Array<uint8_t ,2>>(Array<uint8_t ,2> &, const uint8_t &value, const Index<2> &center, unsigned radius);
  //template void DrawFilledCircle<Array<uint8_t ,2>, uint8_t, float>(Array<uint8_t ,2> &, const uint8_t &value, const Point<float, 2> &center, float radius);
  //template void DrawCircle<Array<uint8_t ,2>>(Array<uint8_t ,2> &, const uint8_t &value, const Point<float, 2> &center, float radius);

}