//
// Created by charles galambos on 13/08/2024.
//

#include "Ravl2/Math/LinearAlgebra.hh"

namespace Ravl2
{
  template std::optional<Matrix<float, 2, 2>> inverse(const Matrix<float, 2, 2> &data);
  template std::optional<Matrix<float, 3, 3>> inverse(const Matrix<float, 3, 3> &data);

}// namespace Ravl2