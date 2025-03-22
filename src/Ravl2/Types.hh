//
// Created by charles on 05/08/24.
//

#pragma once

#ifndef CEREAL_THREAD_SAFE
#define CEREAL_THREAD_SAFE 1
#endif

#include <span>
#include <fmt/ostream.h>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Geometry>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>

#include "Ravl2/Index.hh"
#include "Ravl2/Math.hh"

namespace Ravl2
{
  //! @brief Define allowed copy modes for converting between Arrays
  //! If auto is selected data will be copied / converted if the format is
  //! not compatible with the target type
  enum class CopyModeT : uint8_t
  {
    Never,
    Auto,
    Always
  };

  //! Is the inside of a boundary on the left or right side of the boundary?
  //! This is used for pixel boundaries and polygons.
  enum class BoundaryOrientationT : uint8_t
  {
    INSIDE_LEFT,
    INSIDE_RIGHT
  };

  inline BoundaryOrientationT reverse(BoundaryOrientationT orient)
  {
    return (orient == BoundaryOrientationT::INSIDE_LEFT) ? BoundaryOrientationT::INSIDE_RIGHT : BoundaryOrientationT::INSIDE_LEFT;
  }

  template <typename DataT>
  using VectorT = Eigen::Matrix<DataT, Eigen::Dynamic, 1>;

  //template <typename DataT> using TensorT = Eigen::Tensor<DataT>;

  template <typename DataT>
  using MatrixT = Eigen::Matrix<DataT, Eigen::Dynamic, Eigen::Dynamic>;

  using IndexSizeT = Eigen::Index;
  using IndexT = Eigen::Index;

  using EmbeddingMatrixT = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>;
  using VectorViewT = std::span<float>;
  using ConstVectorViewT = std::span<const float>;

  template <typename DataT, IndexSizeT N>
  using Point = Eigen::Matrix<DataT, N, 1>;

  template <typename DataT, IndexSizeT N>
  using Vector = Eigen::Matrix<DataT, N, 1>;

  template <typename DataT, IndexSizeT N, IndexSizeT M>
  using Matrix = Eigen::Matrix<DataT, N, M>;

  //template <typename DataT, unsigned N> using Tensor = xt::xtensor<DataT, N>;

  using Vector4f = Vector<float, 4>;
  using Vector3f = Vector<float, 3>;
  using Vector3d = Vector<double, 3>;
  using Vector2f = Vector<float, 2>;
  using Vector2d = Vector<double, 2>;
  using Point2f = Point<float, 2>;
  using Point2d = Point<double, 2>;
  using AngleAxisf = Vector4f;

  using Matrix2f = Matrix<float, 2, 2>;
  using Matrix3f = Matrix<float, 3, 3>;

  //! Convert to a string
  [[nodiscard]] std::string toString(Vector3f v);
  [[nodiscard]] std::string toString(Vector3d v);
  [[nodiscard]] std::string toString(Vector2f v);
  [[nodiscard]] std::string toString(Vector2d v);
  //std::string toString(const VectorT &v);

  template <typename DataT>
  [[nodiscard]] bool is16ByteAligned(const DataT *data)
  {
    return (reinterpret_cast<uintptr_t>(data) & static_cast<uintptr_t>(0xf)) == 0;
  }

  //! Check there are no nan's or infinities in an eigen matrix
  template <typename DataT, int N, int M>
  [[nodiscard]] bool isFinite(const Eigen::Matrix<DataT, N, M> &mat)
  {
    return mat.array().isFinite().all();
  }

  //! This is a hack to prevent the compiler optimizing away benchmark code
  void doNothing();

  //! Get a human-readable name for a type.
  [[nodiscard]] std::string typeName(const std::type_info &type);

  //! Get a human-readable name for a type.
  [[nodiscard]] std::string typeName(const std::type_index &type);

  //! Demangle a C++ symbol name
  [[nodiscard]] std::string demangle(const char *name);


  //! Register an alternative name for a type.
  bool registerTypeName(const std::type_info &type, const std::string &name);

  //! Default formatting for converting Eigen matrices to strings
  [[nodiscard]] const Eigen::IOFormat &defaultEigenFormat();
}// namespace Ravl2

template <typename T>
struct fmt::formatter<Eigen::WithFormat<T>> : ostream_formatter {};

#if 0
template <typename T>
requires std::is_base_of_v<Eigen::DenseBase<T>, T>
struct fmt::formatter<T> : ostream_formatter {};
#else
template <class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
struct fmt::formatter<Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>>
{
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> const& mat, FormatContext& ctx) const
  {
    if constexpr(_Rows == 1 && _Cols == 1)
      return fmt::format_to(ctx.out(), "{}", mat(0,0));
    else
      return fmt::format_to(ctx.out(), "{}", Eigen::WithFormat(mat, Ravl2::defaultEigenFormat()));
  }

};

#endif



