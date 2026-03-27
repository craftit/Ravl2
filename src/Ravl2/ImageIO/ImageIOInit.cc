//
// Created by charles galambos on 21/11/2025.
//

#include "ImageIOInit.hh"
#include "Ravl2/Pixel/PixelPlane.hh"
#include "Ravl2/Pixel/Colour.hh"

namespace Ravl2
{

  void initImageIO()
  {
    initPngImageIO();
    initJpegTurboImageIO();
    initPlaneConversion();
    initColourConversion();
  }

} // namespace Ravl2