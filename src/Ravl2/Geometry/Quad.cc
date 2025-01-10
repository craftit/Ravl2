//
// Created by charles on 04/01/25.
//

#include "Ravl2/Geometry/Quad.hh"

namespace Ravl2
{
  // Instantiate the class for float and double
  template class Quad<float,2>;
  template class Quad<double,2>;
  template class Quad<float,3>;
  template class Quad<double,3>;

}// namespace Ravl2