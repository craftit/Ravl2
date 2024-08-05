//
// Created by charles on 04/08/24.
//

#include "Ravl2/Qt/Image.hh"

namespace Ravl2
{
  template QImage toQImage<uint8_t>(const Array<uint8_t, 2> &array);
  template QImage toQImage<uint16_t>(const Array<uint16_t, 2> &array);
  template QImage toQImage<PixelRGB<uint8_t>>(const Array<PixelRGB<uint8_t>, 2> &array);
  template QImage toQImage<PixelRGBA<uint8_t>>(const Array<PixelRGBA<uint8_t>, 2> &array);
  template QImage toQImage<PixelRGBA<uint16_t>>(const Array<PixelRGBA<uint16_t>, 2> &array);
  template QImage toQImage<PixelRGBA<float>>(const Array<PixelRGBA<float>, 2> &array);


  template Array<uint8_t, 2> toArray<uint8_t>(const QImage &image);
  template Array<uint16_t, 2> toArray<uint16_t>(const QImage &image);
  template Array<PixelRGB<uint8_t>, 2> toArray<PixelRGB<uint8_t>>(const QImage &image);
  template Array<PixelRGBA<uint8_t>, 2> toArray<PixelRGBA<uint8_t>>(const QImage &image);

}