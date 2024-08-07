//
// Created by charles on 07/08/24.
//

#include "Ravl2/Array1.hh"

namespace Ravl2
{
  const int gStride1 = 1;

  template class Array<uint8_t, 1>;
  template class ArrayIter<uint8_t, 1>;
  template class ArrayAccess<uint8_t, 1>;

  template class Array<float, 1>;
  template class ArrayAccess<float, 1>;
  template class ArrayIter<float, 1>;

}