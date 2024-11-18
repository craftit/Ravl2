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
  enum class BoundaryOrientationT
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

  using EmbeddingMatrixT = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>;
  using VectorViewT = std::span<float>;
  using ConstVectorViewT = std::span<const float>;

  template <typename DataT, size_t N>
  using Point = Eigen::Matrix<DataT, N, 1>;

  template <typename DataT, size_t N>
  using Vector = Eigen::Matrix<DataT, N, 1>;

  template <typename DataT, size_t N, size_t M>
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
  inline bool is16ByteAligned(const DataT *data)
  {
    return (reinterpret_cast<uintptr_t>(data) & static_cast<uintptr_t>(0xf)) == 0;
  }

  //! This is a hack to prevent the compiler optimizing away benchmark code
  void doNothing();

  //! Get a human-readable name for a type.
  std::string typeName(const std::type_info &type);

  //! Get a human-readable name for a type.
  std::string typeName(const std::type_index &type);

}// namespace Ravl2

#if 0
#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<xt::xarray<float>> : fmt::ostream_formatter {
};
template <typename RealT, size_t N>
struct fmt::formatter<Ravl2::Point<RealT, N>> : fmt::ostream_formatter {
};
template <typename RealT, size_t N>
struct fmt::formatter<xt::xtensor<RealT, N>> : ostream_formatter {
};
template <typename RealT, size_t N, size_t M>
struct fmt::formatter<Ravl2::Matrix<RealT, N, M>> : ostream_formatter {
};
template <>
struct fmt::formatter<xt::xarray<float>::shape_type> : fmt::ostream_formatter {
};
template <>
struct fmt::formatter<xt::xtensor_container<xt::uvector<float>, 2, xt::layout_type::row_major>> : fmt::ostream_formatter {
};
//template <> struct fmt::formatter<std::span<float> > : ostream_formatter{};
#endif

namespace xt
{
  //! Serialization support
  template <class Archive, typename DataT, size_t N>
  void serialize(Archive &archive, xt::xfixed_container<DataT, xt::fixed_shape<N>, xt::layout_type::row_major, false, xt::xtensor_expression_tag> &pnt)
  {
    cereal::size_type size = N;
    archive(cereal::make_size_tag(size));
    if(size != N) {
      throw std::runtime_error("Size mismatch");
    }
    (void)pnt;
    for(auto &it : pnt) {
      archive(it);
    }
  }

  //! Serialization support
  template <class Archive, typename DataT, size_t N, size_t M>
  void serialize(Archive &archive, Ravl2::Matrix<DataT, N, M> &mat)
  {
    cereal::size_type size = N * M;
    archive(cereal::make_size_tag(size));
    if(size != N * M) {
      throw std::runtime_error("Size mismatch");
    }
    for(auto &it : mat) {
      archive(it);
    }
  }
}// namespace xt
#endif