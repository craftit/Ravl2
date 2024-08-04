

#include "Ravl2/Pixel/PixelRGB.hh"

namespace Ravl2
{
  static_assert(sizeof(PixelRGB<uint8_t>) == 3, "PixelRGB<uint8_t> is not packed");

  inline std::istream &operator>>(std::istream &strm, PixelRGB<uint8_t> &val)
  {
    int r, g, b;
    strm >> r >> g >> b;
    val.Set(uint8_t(r), uint8_t(g), uint8_t(b));
    return strm;
  }

  inline std::ostream &operator<<(std::ostream &strm, const PixelRGB<uint8_t> &val)
  {
    return strm << int(val.Red()) << ' ' << int(val.Green()) << ' ' << int(val.Blue());
  }

}// namespace Ravl2
