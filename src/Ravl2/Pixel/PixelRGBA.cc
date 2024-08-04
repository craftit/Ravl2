
#include "Ravl2/Pixel/PixelRGBA.hh"

namespace Ravl2
{
  static_assert(sizeof(PixelRGBA<uint8_t>) == 4, "PixelRGBA<uint8_t> is not packed");

  inline std::istream &operator>>(std::istream &strm, PixelRGBA<uint8_t> &val)
  {
    int r, g, b, a;
    strm >> r >> g >> b >> a;
    val= PixelRGBA<uint8_t>(uint8_t(r), uint8_t(g), uint8_t(b), uint8_t(a));
    return strm;
  }

  inline std::ostream &operator<<(std::ostream &strm, const PixelRGBA<uint8_t> &val)
  {
    return strm << int(val.Red()) << ' ' << int(val.Green()) << ' ' << int(val.Blue()) << ' ' << int(val.Alpha());
  }

}// namespace Ravl2
