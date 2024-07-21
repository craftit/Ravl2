
#include "Ravl2/Array.hh"

namespace Ravl2
{

  static_assert(WindowedArray<ArrayAccess<int,2> ,int,2>, "ArrayAccess<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<ArrayView<int,2> ,int,2>, "ArrayView<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<Array<int,2> ,int,2>, "Array<int,2> does not satisfy WindowedArray");

  static_assert(WindowedArray<ArrayAccess<int,1> ,int,1>, "ArrayAccess<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<ArrayView<int,1> ,int,1>, "ArrayView<int,2> does not satisfy WindowedArray");
  static_assert(WindowedArray<Array<int,1> ,int,1>, "Array<int,2> does not satisfy WindowedArray");

  void doNothing()
  {}
}
