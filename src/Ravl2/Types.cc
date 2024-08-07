//
// Created by charles on 05/08/24.
//
#include "Ravl2/Types.hh"

namespace Ravl2
{
  //  static_assert(sizeof(Vector2f) == 2*sizeof(float),"Vector2f is not packed");
  //  static_assert(sizeof(Vector3f) == 3*sizeof(float),"Vector2f is not packed");

  std::string toString(Vector3d v)
  {
    return fmt::format("({},{},{})", v[0], v[1], v[2]);
  }

  std::string toString(Vector3f v)
  {
    return fmt::format("({},{},{})", v[0], v[1], v[2]);
  }

  std::string toString(Vector2f v)
  {
    return fmt::format("({},{})", v[0], v[1]);
  }

  std::string toString(Vector2d v)
  {
    return fmt::format("({},{})", v[0], v[1]);
  }


  void doNothing()
  {}

}