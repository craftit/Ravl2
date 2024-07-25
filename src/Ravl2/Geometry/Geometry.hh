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
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wparentheses"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wimplicit-float-conversion"
#endif
#include <xtensor/xio.hpp>
#include <xtensor/xtensor.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xrandom.hpp>
#include <xtensor/xadapt.hpp>
#include <xtensor/xsort.hpp>
#include <xtensor/xnorm.hpp>
#include <xtensor/xnpy.hpp>
#include <xtensor/xmath.hpp>
#include <xtensor/xreducer.hpp>
#include <xtensor-blas/xlinalg.hpp>
#pragma GCC diagnostic pop
#endif

#include "Ravl2/Index.hh"
#include "Ravl2/Math.hh"

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
    template<typename DataT>
    using VectorT = xt::xtensor<DataT,1>;

    template<typename DataT>
    using TensorT = xt::xarray<DataT>;

    template<typename DataT>
    using MatrixT = xt::xtensor<DataT, 2>;

    using EmbeddingMatrixT = xt::xtensor<float, 2>;
    using VectorViewT = std::span<float>;
    using ConstVectorViewT = std::span<const float>;

    template<typename DataT, size_t N>
    using Point = xt::xtensor_fixed<DataT, xt::xshape<N>>;

    template<typename DataT, size_t N>
    using Vector = xt::xtensor_fixed<DataT, xt::xshape<N>>;

    template<typename DataT, size_t N, size_t M>
    using Matrix = xt::xtensor_fixed<DataT, xt::xshape<N,M>>;

    using Vector4f = Vector<float, 4>;
    using Vector3f = Vector<float, 3>;
    using Vector3d = Vector<double, 3>;
    using Vector2f = Vector<float, 2>;
    using Vector2d = Vector<double, 2>;
    using Point2f = Point<float, 2>;
    using Point2d = Point<double, 2>;
    using AngleAxisf = Vector4f;

    using Matrix2f = Matrix<float, 2,2>;
    using Matrix3f = Matrix<float, 3,3>;

    //! Get a perpendicular vector in 2d.
    template<typename DataT>
    inline Vector<DataT, 2> perpendicular(const Vector<DataT, 2> &v)
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

    // Define the concept of a point transform
    template<typename TransformT,typename RealT = typename TransformT::ValueT,unsigned N = TransformT::dimension>
    concept PointTransform = requires(TransformT a, Point<RealT,N> pnt)
    {
      { a(pnt) } -> std::convertible_to<Point<RealT,N> >;
    };

    //! Convert to a string
    [[nodiscard]] std::string toString(Vector3f v);
    [[nodiscard]] std::string toString(Vector3d v);
    [[nodiscard]] std::string toString(Vector2f v);
    [[nodiscard]] std::string toString(Vector2d v);
    //std::string toString(const VectorT &v);

    template<typename RealT,unsigned N>
    constexpr RealT squaredEuclidDistance(const Point<RealT,N> &a,const Point<RealT,N> &b)
    {
      RealT sum = 0;
      for(unsigned i = 0; i < N; i++) {
        sum += sqr(a(i) - b(i));
      }
      return sum;
    }

    template<typename RealT,unsigned N>
    constexpr auto euclidDistance(const Point<RealT,N> &a,const Point<RealT,N> &b)
    {
      RealT sum = 0;
      for(unsigned i = 0; i < N; i++) {
        sum += sqr(a(i) - b(i));
      }
      return std::sqrt(sum);
    }

    template<typename RealT = float,unsigned N>
    constexpr auto euclidDistance(const Index<N> &a,const Index<N> &b)
    {
      int sum = 0;
      for(unsigned i = 0; i < N; i++) {
        sum += sqr(a[i] - b[i]);
      }
      return std::sqrt(RealT(sum));
    }

  //! City Block Distance

    template<typename A, typename B>
    constexpr auto cityBlockDistance(xt::xexpression<A> a,xt::xexpression<B> b)
    { return xt::sum(xt::abs(a - b)); }

    template<unsigned N>
    constexpr auto cityBlockDistance(const Index<N> &a,const Index<N> &b)
    {
      int sum = 0;
      for(unsigned i = 0; i < N; i++) {
	sum += std::abs(a[i] - b[i]);
      }
      return sum;
    }

    //! Compute twice the area contained by the three 2d points.
    //! Area of triangle (*this, second, third) is equal to the area
    //! of the triangle which the first point represents the origin
    //! of the coordinate system. In fact the points 'aa' and 'bb'
    //! represents two vectors and the computed area is equal to
    //! the size of the cross product of these two vectors.
    // Point2dC aa(second - *this);   // O'Rourke 1.2
    // Point2dC bb(third  - *this);
    // return aa[0]*bb[1] - aa[1]*bb[0];
    template<typename RealT>
    constexpr RealT triangleArea2(const Point<RealT,2>& first,const Point<RealT,2>& second, const Point<RealT,2>& third) {
      return (second[0] - first[0]) * (third[1] - first[1]) - (second[1] - first[1]) * (third[0] - first[0]);
    }

    using xt::linalg::dot;

    template<typename RealT>
    RealT cross(const Point<RealT,2> &a,const Point<RealT,2> &b)
    { return a[0] * b[1] - a[1] * b[0]; }


  //! Convert a point to the closest integer index
    template<unsigned N,typename RealT>
    requires std::is_floating_point<RealT>::value
    constexpr inline Index<N> toIndex(const Point<RealT,N> &pnt)
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = int_round(pnt[i]);
      }
      return ret;
    }

  template<unsigned N,typename RealT>
  requires std::is_integral<RealT>::value
  constexpr inline Index<N> toIndex(const Point<RealT,N> &pnt)
  {
    Index<N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = pnt[i];
    }
    return ret;
  }


  //! Convert an index to a point
  template<typename RealT,unsigned N>
  constexpr inline Point<RealT,N> toPoint(const Index<N> &idx)
  {
    Point<RealT,N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = RealT(idx[i]);
    }
    return ret;
  }

  //! Convert a parameter list of RealT to a point
  template<typename RealT ,typename ...DataT,unsigned N = sizeof...(DataT)>
  constexpr inline Point<RealT,N> toPoint(DataT ...data)
  {
    return Point<RealT,N>({RealT(data)...});
  }

  //! Convert a parameter list of RealT to a point
  template<typename RealT ,typename ...DataT,unsigned N = sizeof...(DataT)>
  constexpr inline Vector<RealT,N> toVector(DataT ...data)
  {
    return Point<RealT,N>({RealT(data)...});
  }

}

#if !USE_OPENCV
#if FMT_VERSION >= 90000
template <> struct fmt::formatter<xt::xarray<float> > : fmt::ostream_formatter{};
template <> struct fmt::formatter<Ravl2::Point2f > : fmt::ostream_formatter{};
//template <> struct fmt::formatter<xt::xtensor<float,1> > : ostream_formatter{};
template <> struct fmt::formatter<xt::xarray<float>::shape_type> : fmt::ostream_formatter{};
template <> struct fmt::formatter<xt::xtensor_container<xt::uvector<float>, 2, xt::layout_type::row_major> > : fmt::ostream_formatter{};
//template <> struct fmt::formatter<std::span<float> > : ostream_formatter{};
#endif
#endif
