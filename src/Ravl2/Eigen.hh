//
// Created by charles on 28/12/24.
//

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! Create a view of a matrix as a Ravl array.
  //! Note, by default Eigen is column major, so the array will be in column major order.
  template <typename DataT, int N, int M>
    requires (M != 1)
  [[nodiscard]] inline constexpr auto asArrayView(Matrix<DataT, N, M> &mat)
  {
    std::array<int, 2> strides = { int(mat.rows()), 1};
    return ArrayView<DataT, 2>(mat.data(), IndexRange<2>({size_t(mat.cols()), size_t(mat.rows())}), strides);
  }

  //! Create a view of a matrix as a Ravl array.
  template <typename DataT, int N>
  [[nodiscard]] inline constexpr auto asArrayView(Matrix<DataT, N, 1> &mat)
  {
    return ArrayView<DataT, 1>(mat.data(), IndexRange<1>({N}));
  }

  //! Create a view of a matrix as a Ravl array.
  //! Note, by default Eigen is column major, so the array will be in column major order.
  template <typename DataT, int N, int M>
    requires (M != 1)
  [[nodiscard]] inline constexpr auto asArrayView(const Matrix<DataT, N, M> &mat)
  {
    std::array<int, 2> strides = { int(mat.rows()), 1};
    return ArrayView<const DataT, 2>(mat.data(), IndexRange<2>({size_t(mat.cols()), size_t(mat.rows())}), strides);
  }

  //! Create a view of a matrix as a Ravl array.
  template <typename DataT, int N>
  [[nodiscard]] inline constexpr auto asArrayView(const Matrix<DataT, N, 1> &mat)
  {
    return ArrayView<const DataT, 1>(mat.data(), IndexRange<1>({N}));
  }

}