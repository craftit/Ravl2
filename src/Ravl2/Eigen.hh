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
  [[nodiscard]] constexpr auto asArrayView(Matrix<DataT, N, M> &mat, IndexRange<2> range)
  {
    assert(range.size(0) == size_t(mat.cols()));
    assert(range.size(1) == size_t(mat.rows()));
    std::array<int, 2> strides = { int(mat.rows()), 1};
    return ArrayView<DataT, 2>(mat.data() + ArrayView<DataT, 2>::compute_origin_offset(range,strides), range, strides);
  }

  //! Create a view of a matrix as a Ravl array.
  //! Note, by default Eigen is column major, so the array will be in column major order.
  template <typename DataT, int N, int M>
    requires (M != 1)
  [[nodiscard]] constexpr auto asArrayView(Matrix<DataT, N, M> &mat)
  {
    std::array<int, 2> strides = { int(mat.rows()), 1};
    return ArrayView<DataT, 2>(mat.data(), IndexRange<2>({size_t(mat.cols()), size_t(mat.rows())}), strides);
  }

  //! Create a view of a matrix as a Ravl array.
  template <typename DataT, int N>
  [[nodiscard]] constexpr auto asArrayView(Matrix<DataT, N, 1> &mat, IndexRange<1> range = IndexRange<1>({N}))
  {
    assert(range.size(0) == N);
    return ArrayView<DataT, 1>(mat.data() + ArrayView<DataT, 1>::compute_origin_offset(range), range);
  }

  //! Create a view of a matrix as a Ravl array.
  //! Note, by default Eigen is column major, so the array will be in column major order.
  template <typename DataT, int N, int M>
    requires (M != 1)
  [[nodiscard]] constexpr auto asArrayView(const Matrix<DataT, N, M> &mat,IndexRange<2> range)
  {
    assert(range.size(0) == size_t(mat.cols()));
    assert(range.size(1) == size_t(mat.rows()));
    std::array<int, 2> strides = { int(mat.rows()), 1};
    return ArrayView<const DataT, 2>(&mat.data() + ArrayView<DataT, 2>::compute_origin_offset(range,strides), range, strides);
  }

  //! Create a view of a matrix as a Ravl array.
  //! Note, by default Eigen is column major, so the array will be in column major order.
  template <typename DataT, int N, int M>
    requires (M != 1)
  [[nodiscard]] constexpr auto asArrayView(const Matrix<DataT, N, M> &mat)
  {
    IndexRange<2> range({size_t(mat.cols()), size_t(mat.rows())});
    std::array<int, 2> strides = { int(mat.rows()), 1};
    return ArrayView<const DataT, 2>(mat.data() + ArrayView<DataT, 2>::compute_origin_offset(range,strides), range, strides);
  }

  //! Create a view of a matrix as a Ravl array.
  template <typename DataT, int N>
  [[nodiscard]] constexpr auto asArrayView(const Matrix<DataT, N, 1> &mat)
  {
    return ArrayView<const DataT, 1>(mat.data(), IndexRange<1>({N}));
  }

}