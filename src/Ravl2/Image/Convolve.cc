//
// Created by charles galambos on 12/06/2026.
//

#include "Ravl2/Image/Convolve.hh"

namespace Ravl2
{
  template std::vector<float> gaussianKernel(float, float);
  template void convolveHorizontal(Array<float, 2> &, const Array<float, 2> &, std::span<const float>);
  template void convolveVertical(Array<float, 2> &, const Array<float, 2> &, std::span<const float>);
  template void convolveSeparable(Array<float, 2> &, const Array<float, 2> &, std::span<const float>, std::span<const float>, Array<float, 2> &);
  template void gaussianBlur(Array<float, 2> &, const Array<float, 2> &, float, Array<float, 2> &, Array<float, 2> &, float);
  template void gaussianBlur(Array<float, 2> &, const Array<float, 2> &, float, float);
}// namespace Ravl2
