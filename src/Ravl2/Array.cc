
#include "Ravl2/Array.hh"

namespace Ravl2
{
  const int gStride1 = 1;

  void doNothing()
  {}

  template class Array<uint8_t, 1>;
  template class Array<uint8_t, 2>;
  template class ArrayIter<uint8_t, 1>;
  template class ArrayIter<uint8_t, 2>;
  template class ArrayAccess<uint8_t, 1>;
  template class ArrayAccess<uint8_t, 2>;

  template class Array<float, 1>;
  template class Array<float, 2>;
  template class ArrayAccess<float, 1>;
  template class ArrayAccess<float, 2>;
  template class ArrayIter<float, 1>;
  template class ArrayIter<float, 2>;

}// namespace Ravl2
