//
// Created by charles on 16/07/24.
//

#pragma once

#include <cmath>
#include <span>

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
#include <xtensor-blas/xlinalg.hpp>
#pragma GCC diagnostic pop

#include "Ravl2/Types.hh"

namespace Ravl2
{
  //! Define the concept of a point transform
  //! It is any class/function that takes a point and returns a point
  template <typename TransformT, typename RealT = typename TransformT::value_type, unsigned N = TransformT::dimension>
  concept PointTransform = requires(TransformT a, Point<RealT, N> pnt) {
    { a(pnt) } -> std::convertible_to<Point<RealT, N>>;
  };


  //! Get a perpendicular vector in 2d space
  template <typename DataT>
  inline constexpr Vector<DataT, 2> perpendicular(const Vector<DataT, 2> &v)
  {
    return {-v(1), v(0)};
  }

  //! Convert to a span
  template <typename T>
  constexpr auto span(T t)
  {
    return std::span(t);
  }

  //! \brief Make a span of a tensor
  //! This ensures that the span is constructed correctly from a view
  //! \param view The view to make a span
  template <typename CT, typename... S>
  constexpr auto span(xt::xview<CT, S...> view)
  {
    assert(view.is_contiguous());
    return std::span(view.begin(), view.size());
  }

  //! Compute the l2 norm of a vector
  template <typename RealT, unsigned N>
  [[nodiscard]] RealT norm_l2(const Vector<RealT, N> &v)
  {
    return RealT(xt::linalg::norm(v, 2)());
  }

  //! Compute the angle between two vectors
  template <typename RealT, unsigned long N>
  [[nodiscard]] constexpr RealT angle(const Vector<RealT, N> &a, const Vector<RealT, N> &b)
  {
    return RealT(std::acos((xt::linalg::dot(a, b) / (norm_l2(a) * norm_l2(b)))()));
  }

  template <typename RealT, unsigned long N>
  constexpr RealT squaredEuclidDistance(const Point<RealT, N> &a, const Point<RealT, N> &b)
  {
    RealT sum = 0;
    for(unsigned i = 0; i < N; i++) {
      sum += sqr(a(i) - b(i));
    }
    return sum;
  }

  template <typename RealT, unsigned N>
  constexpr auto euclidDistance(const Point<RealT, N> &a, const Point<RealT, N> &b)
  {
    RealT sum = 0;
    for(unsigned i = 0; i < N; i++) {
      sum += sqr(a(i) - b(i));
    }
    return std::sqrt(sum);
  }

  template <typename Pnt1T, typename Pnt2T>
  constexpr auto euclidDistance(Pnt1T a, Pnt2T b)
  {
    return xt::linalg::norm(a - b, 2);
  }

  template <typename RealT = float, unsigned N>
  constexpr auto euclidDistance(const Index<N> &a, const Index<N> &b)
  {
    int sum = 0;
    for(unsigned i = 0; i < N; i++) {
      sum += sqr(a[i] - b[i]);
    }
    return std::sqrt(RealT(sum));
  }

  template <typename DataTypeT>
  constexpr auto sumOfSqr(const DataTypeT &a)
  {
    return xt::sum(a * a);
  }

  template <typename A, typename B>
  constexpr auto cityBlockDistance(xt::xexpression<A> a, xt::xexpression<B> b)
  {
    return xt::sum(xt::abs(a - b));
  }

  template <unsigned N>
  constexpr auto cityBlockDistance(const Index<N> &a, const Index<N> &b)
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
  //!
  //! It implements the following:
  //!   Point<RealT,2> aa(second - *this);   // O'Rourke 1.2
  //!   Point<RealT,2> bb(third  - *this);
  //!   return aa[0]*bb[1] - aa[1]*bb[0];
  template <typename RealT>
  constexpr RealT triangleArea2(const Point<RealT, 2> &first, const Point<RealT, 2> &second, const Point<RealT, 2> &third)
  {
    return (second[0] - first[0]) * (third[1] - first[1]) - (second[1] - first[1]) * (third[0] - first[0]);
  }

  using xt::linalg::dot;
  using xt::linalg::cross;

  //! Cross product of two 2d vectors
  template <typename RealT>
  RealT cross(const Point<RealT, 2> &a, const Point<RealT, 2> &b)
  {
    return a[0] * b[1] - a[1] * b[0];
  }

  //! Convert a point to the closest integer index
  template <size_t N, typename RealT>
    requires std::is_floating_point<RealT>::value
  constexpr inline Index<N> toIndex(const Point<RealT, N> &pnt)
  {
    Index<N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = int_round(pnt[i]);
    }
    return ret;
  }

  //! Get the closest integer index from an integer point
  template <size_t N, typename NumberT>
    requires std::is_integral<NumberT>::value
  constexpr inline Index<N> toIndex(const Point<NumberT, N> &pnt)
  {
    Index<N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = NumberT(pnt[i]);
    }
    return ret;
  }

  //! Convert an index to a point
  template <typename RealT, unsigned N>
  constexpr inline Point<RealT, N> toPoint(const Index<N> &idx)
  {
    Point<RealT, N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = RealT(idx[i]);
    }
    return ret;
  }

  //! Convert a parameter list of RealT to a point
  template <typename RealT, typename... DataT, unsigned N = sizeof...(DataT)>
  constexpr inline Point<RealT, N> toPoint(DataT... data)
  {
    return Point<RealT, N>({RealT(data)...});
  }

  //! Convert a parameter list of RealT to a point
  template <typename RealT, typename... DataT, unsigned N = sizeof...(DataT)>
  constexpr inline Vector<RealT, N> toVector(DataT... data)
  {
    return Vector<RealT, N>({RealT(data)...});
  }

}// namespace Ravl2
