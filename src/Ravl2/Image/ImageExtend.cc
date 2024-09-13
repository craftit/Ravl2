
#include "Ravl2/Image/ImageExtend.hh"

namespace Ravl2
{

  template bool resizeArray<CopyModeT::Auto>(Array<uint8_t, 2> &, const IndexRange<2> &);
  template void extendImageFill(Array<uint8_t, 2> &, const Array<uint8_t, 2> &, unsigned, const uint8_t &);
  template void extendImageCopy(Array<uint8_t, 2> &, const Array<uint8_t, 2> &, unsigned);
  template void extendImageMirror(Array<uint8_t, 2> &, const Array<uint8_t, 2> &, unsigned);
  template void mirrorEdges(Array<uint8_t, 2> &, unsigned);

}// namespace Ravl2