//
// Created by charles on 16/07/24.
//

#pragma once

#define USE_OPENCV 0

#include <cmath>
#include <span>
#include <fmt/ostream.h>

#if USE_OPENCV
#include <opencv2/core.hpp>
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif
#include <xtensor/xio.hpp>
#include <xtensor/xtensor.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xrandom.hpp>
#include <xtensor/xadapt.hpp>
#include <xtensor/xsort.hpp>
#include <xtensor/xnorm.hpp>
#include <xtensor/xnpy.hpp>
#pragma GCC diagnostic pop
#endif

namespace Ravl2
{
#if USE_OPENCV
    using Point2f = cv::Vec2f;
    using Point2d = cv::Vec2d;
    using Matrix22f = cv::Matx22f;
    using Vector2f = cv::Vec2f;
    using Vector2d = cv::Vec2d;
    using Vector3f = cv::Vec3f;
    using Vector3d = cv::Vec3d;
    using Vector3d = cv::Vec3d;
    //using VectorT = cv::Vec;
#else
    using VectorT = xt::xtensor<float,1>;
    using TensorT = xt::xarray<float>;
    using EmbeddingMatrixT = xt::xtensor<float, 2>;
    using VectorViewT = std::span<float>;
    using ConstVectorViewT = std::span<const float>;

    template<typename DataT, size_t N>
    using Vector = xt::xtensor_fixed<DataT, xt::xshape<N>>;

    template<typename DataT, size_t N, size_t M>
    using Matrix = xt::xtensor_fixed<DataT, xt::xshape<N,M>>;

    using Vector4f = xt::xtensor_fixed<float, xt::xshape<4>>;
    using Vector3f = xt::xtensor_fixed<float, xt::xshape<3>>;
    using Vector3d = xt::xtensor_fixed<double, xt::xshape<3>>;
    using Vector2f = xt::xtensor_fixed<float, xt::xshape<2>>;
    using Vector2d = xt::xtensor_fixed<double, xt::xshape<2>>;
    using Point2f = xt::xtensor_fixed<float, xt::xshape<2>>;
    using Point2d = xt::xtensor_fixed<double, xt::xshape<2>>;
    using AngleAxisf = Vector4f;

    using Matrix2f = xt::xtensor_fixed<float, xt::xshape<2,2>>;
    using Matrix3f = xt::xtensor_fixed<float, xt::xshape<3,3>>;

    //! Get a perpendicular vector in 2d.
    inline Vector2f perpendicular(const Vector2f &v)
    { return {-v(1),v(0)}; }

    template<typename T>
    auto span(T t)
    { return std::span(t); }

    //! \brief Make a span of a tensor
    //! This ensures that the span is constructed correctly from a view
    //! \param view The view to make a span
    template<typename CT,typename ... S>
    auto span(xt::xview<CT,S...> view)
    {
      assert(view.is_contiguous());
      return std::span(view.begin(),view.size());
    }
#endif
    //! Convert to a string
    [[nodiscard]] std::string toString(Vector3f v);
    [[nodiscard]] std::string toString(Vector3d v);
    [[nodiscard]] std::string toString(Vector2f v);
    [[nodiscard]] std::string toString(Vector2d v);
    //std::string toString(const VectorT &v);
}

#if !USE_OPENCV
#if FMT_VERSION >= 90000
template <> struct fmt::formatter<xt::xarray<float> > : fmt::ostream_formatter{};
//template <> struct fmt::formatter<xt::xtensor<float,1> > : ostream_formatter{};
template <> struct fmt::formatter<xt::xarray<float>::shape_type> : fmt::ostream_formatter{};
template <> struct fmt::formatter<xt::xtensor_container<xt::uvector<float>, 2, xt::layout_type::row_major> > : fmt::ostream_formatter{};
//template <> struct fmt::formatter<std::span<float> > : ostream_formatter{};
#endif
#endif
