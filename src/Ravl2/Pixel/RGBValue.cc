

#include "Ravl2/Pixel/RGBValue.hh"

namespace Ravl2
{
  static_assert(sizeof(RGBValueC<uint8_t>) == 3,"RGBValueC<uint8_t> is not packed");

  inline std::istream &operator>>(std::istream &strm,RGBValueC<uint8_t> &val) {
    int r,g,b;
    strm >> r >> g >> b;
    val.Set(uint8_t(r),uint8_t(g),uint8_t(b));
    return strm;
  }

  inline std::ostream &operator<<(std::ostream &strm,const RGBValueC<uint8_t> &val)
  { return strm << int(val.Red()) << ' ' << int(val.Green()) << ' ' << int(val.Blue()); }


}
