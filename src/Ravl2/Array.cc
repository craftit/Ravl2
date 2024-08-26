
#include "Ravl2/Array.hh"

namespace Ravl2
{
  template class Array<uint8_t, 2>;
  template class ArrayIter<uint8_t, 2>;
  template class ArrayAccess<uint8_t, 2>;

  template class Array<uint16_t, 2>;
  template class ArrayAccess<uint16_t, 2>;
  template class ArrayIter<uint16_t, 2>;

  template class Array<int32_t, 2>;
  template class Array<int64_t, 2>;

  template class Array<float, 2>;
  template class ArrayAccess<float, 2>;
  template class ArrayIter<float, 2>;

}// namespace Ravl2
