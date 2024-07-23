
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

    std::string toString(Vector3d v)
    {
      return fmt::format("({},{},{})",v[0],v[1],v[2]);
    }

    std::string toString(Vector3f v)
    {
      return fmt::format("({},{},{})",v[0],v[1],v[2]);
    }

    std::string toString(Vector2f v)
    {
      return fmt::format("({},{})",v[0],v[1]);
    }

    std::string toString(Vector2d v)
    {
      return fmt::format("({},{})",v[0],v[1]);
    }

//    std::string toString(const VectorT &v)
//    {
//      std::string ret = fmt::format("{}: ",v.shape(0));
//      for(auto &val : v) {
//        ret += fmt::format("{},",val);
//      }
//      return ret;
//    }


}