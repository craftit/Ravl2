//
// Created by charles on 13/08/24.
//

#include "Ravl2/Image/Segmentation/BoundaryMoments.hh"

namespace Ravl2
{
  // Some common instantiations
  template Moments2<double> moments2<double>(const Boundary &boundary);
  template Moments2<int64_t> moments2<int64_t>(const Boundary &boundary);

}