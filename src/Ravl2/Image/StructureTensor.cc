//
// Created by charles galambos on 12/06/2026.
//

#include "Ravl2/Image/StructureTensor.hh"

namespace Ravl2
{
  template struct StructureTensorWorkspace<float>;
  template void structureTensor(Array<float, 2> &, Array<float, 2> &, Array<float, 2> &,
                                const Array<float, 2> &, float, float, StructureTensorWorkspace<float> &);
  template void harrisResponse(Array<float, 2> &, const Array<float, 2> &, const Array<float, 2> &,
                               const Array<float, 2> &, float);
}// namespace Ravl2
