
#include "Ravl2/Image/ImageExtend.hh"

namespace Ravl2
{

  template bool resizeArray<CopyModeT::Auto>(Array<uint8_t , 2> &, const IndexRange<2> &);
  template void extendImageFill(Array<uint8_t, 2> &, const Array<uint8_t, 2> &, int, const uint8_t &);

}