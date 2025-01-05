//
// Created by charles on 16/07/24.
//

#pragma once

#include <cmath>
#include <span>

#include "Ravl2/Types.hh"

namespace Ravl2
{
  //! Define the concept of a point transform
  //! It is any class/function that takes a point and returns a point
  template <typename TransformT, typename RealT = typename TransformT::value_type, unsigned N = TransformT::dimension>
  concept PointTransform = requires(TransformT a, Point<RealT, N> pnt) {
    { a(pnt) } -> std::convertible_to<Point<RealT, N>>;
  };

  //! Define the concept of a transform
  //! It is any class/function that takes a point and returns a point
  template <typename TransformT, typename RealT = typename TransformT::value_type, unsigned N = TransformT::dimension>
  concept GeometryTransform = requires(TransformT a, Point<RealT, N> pnt) {
    { a(pnt) } -> std::convertible_to<Point<RealT, N>>;
    { a.projectiveMatrix() } -> std::convertible_to<Matrix<RealT, N+1, N+1>>;
    { a.isReal() } -> std::convertible_to<bool>;
    { a.inverse() } -> PointTransform; // We use a PointTransform here to avoid a recursive concept definition.
    { TransformT::identity() } -> PointTransform;
  };

  //! Operator to transform a point
  template <typename TransformT, typename RealT = typename TransformT::value_type, unsigned N = TransformT::dimension>
   requires PointTransform<TransformT, RealT, N>
  [[nodiscard]] constexpr auto operator*(const TransformT &a, const Point<RealT, N> &pnt)
  {
    return a(pnt);
  }
  
  //! Get a perpendicular vector in 2d space
  template <typename DataT>
  [[nodiscard]] inline constexpr Vector<DataT, 2> perpendicular(const Vector<DataT, 2> &v)
  {
    return {-v(1), v(0)};
  }

  //! Convert to a span
  template <typename T>
  [[nodiscard]] constexpr auto span(T t)
  {
    return std::span(t);
  }

  template <typename RealT, int N>
  auto dot(const Vector<RealT, N> &a, const Vector<RealT, N> &b)
  {
    return a.dot(b);
  }

  template <typename RealT, int N, int M>
    requires (N > 1) && (M > 1)
  auto dot(const Matrix<RealT, N, M> &a, const Vector<RealT, N> &b)
  {
    return a * b;
  }


  template <typename RealT, int N>
  auto sum(const Vector<RealT, N> &a)
  {
    return a.sum();
  }

  //! Compute the angle between two vectors
  template <typename RealT, IndexSizeT N>
  [[nodiscard]] constexpr RealT angle(const Vector<RealT, N> &a, const Vector<RealT, N> &b)
  {
    return std::acos((a.dot(b) / (a.norm() * b.norm())));
  }

  template <typename RealT>
    requires std::is_floating_point<RealT>::value
  [[nodiscard]] RealT norm_l2(RealT v)
  {
    return std::abs(v);
  }

  //! Compute the l2 norm of a vector
  template <typename RealT, IndexSizeT N>
  [[nodiscard]] RealT norm_l2(const Vector<RealT, N> &v)
  {
    RealT sum = 0;
    for(unsigned i = 0; i < N; i++) {
      sum += sqr(v(i));
    }
    return std::sqrt(sum);
  }

  template <typename RealT, IndexSizeT N>
  [[nodiscard]] constexpr RealT squaredEuclidDistance(const Point<RealT, N> &a, const Point<RealT, N> &b)
  { return (a - b).squaredNorm(); }

  template <typename RealT, IndexSizeT N>
  [[nodiscard]] constexpr auto euclidDistance(const Point<RealT, N> &a, const Point<RealT, N> &b)
  {
    RealT sum = 0;
    for(unsigned i = 0; i < N; i++) {
      sum += sqr(a(i) - b(i));
    }
    return std::sqrt(sum);
  }

  template <typename Pnt1T>
  [[nodiscard]] constexpr auto norm(Pnt1T a)
  { return a.norm(); }

  template <typename Pnt1T, typename Pnt2T>
  [[nodiscard]] constexpr auto euclidDistance(Pnt1T a, Pnt2T b)
  {
    return Ravl2::norm(a - b);
  }

  template <typename RealT = float, IndexSizeT N>
  [[nodiscard]] constexpr auto euclidDistance(const Index<N> &a, const Index<N> &b)
  {
    int sum = 0;
    for(unsigned i = 0; i < N; i++) {
      sum += sqr(a[i] - b[i]);
    }
    return std::sqrt(RealT(sum));
  }

  template <typename DataTypeT, typename RealT = DataTypeT::value_type>
  [[nodiscard]] constexpr RealT sum(const DataTypeT &a)
  {
    return a.sum();
  }

  template <typename DataTypeT, typename RealT = DataTypeT::value_type>
  [[nodiscard]] constexpr RealT sumOfSqr(const DataTypeT &ae)
  {
    return ae.cwiseProduct(ae).sum();
  }

  template <typename RealT>
    requires std::is_floating_point<RealT>::value
  [[nodiscard]] constexpr RealT sumOfSqr(RealT a)
  {
    return std::abs(a);
  }

  template <unsigned N>
  [[nodiscard]] constexpr auto cityBlockDistance(const Index<N> &a, const Index<N> &b)
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
  [[nodiscard]] constexpr RealT triangleArea2(const Point<RealT, 2> &first, const Point<RealT, 2> &second, const Point<RealT, 2> &third)
  {
    return (second[0] - first[0]) * (third[1] - first[1]) - (second[1] - first[1]) * (third[0] - first[0]);
  }

  //! Cross product of two 2d vectors
  template <typename RealT>
  [[nodiscard]] RealT cross(const Point<RealT, 2> &a, const Point<RealT, 2> &b)
  {
    return a[0] * b[1] - a[1] * b[0];
  }

  //! Cross product of two 2d vectors
  template <typename RealT>
  [[nodiscard]] Point<RealT, 3> cross(const Point<RealT, 3> &a, const Point<RealT, 3> &b)
  {
    return {a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]};
  }

  //! Convert a point to the closest integer index
  template <IndexSizeT N, typename RealT>
    requires std::is_floating_point<RealT>::value
  [[nodiscard]] constexpr Index<N> toIndex(const Point<RealT, N> &pnt)
  {
    Index<N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = intRound(pnt[i]);
    }
    return ret;
  }

  //! Get the closest integer index from an integer point
  template <IndexSizeT N, typename NumberT>
    requires std::is_integral<NumberT>::value
  [[nodiscard]] constexpr Index<unsigned (N)> toIndex(const Point<NumberT, N> &pnt)
  {
    Index<unsigned (N)> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = NumberT(pnt[IndexT(i)]);
    }
    return ret;
  }

  //! Convert an index to a point
  template <typename RealT, unsigned N>
    requires std::is_convertible<int, RealT>::value
  [[nodiscard]] constexpr Point<RealT, N> toPoint(const Index<N> &idx)
  {
    Point<RealT, N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[IndexT(i)] = RealT(idx[i]);
    }
    return ret;
  }

  //! Convert a parameter list of RealT to a point
  template <typename RealT, typename... DataT, IndexSizeT N = sizeof...(DataT)>
    requires(std::is_convertible_v<DataT, RealT> && ...)
  [[nodiscard]] constexpr Point<RealT, N> toPoint(DataT... data)
  {
    return Point<RealT, N>({RealT(data)...});
  }

  //! Convert a parameter list of RealT to a point
  template <typename RealT, typename SourceT, int N>
    requires(std::is_convertible_v<SourceT, RealT>)
  [[nodiscard]] constexpr Point<RealT, N> toPoint(const Point<SourceT, N> &pnt)
  {
    Point<RealT, N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = RealT(pnt[i]);
    }
    return ret;
  }

  //! Convert a parameter list of RealT to a point
  template <typename RealT, typename... DataT, IndexSizeT N = sizeof...(DataT)>
    requires(std::is_convertible_v<DataT, RealT> && ...)
  [[nodiscard]] constexpr Vector<RealT, N> toVector(DataT... data)
  {
    return Vector<RealT, N>({RealT(data)...});
  }

  //! Convert a parameter list of RealT to a point
  template <typename RealT, typename SourceT, IndexSizeT N>
    requires(std::is_convertible_v<SourceT, RealT>)
  [[nodiscard]] constexpr Point<RealT, N> toVector(const Point<SourceT, N> &pnt)
  {
    Vector<RealT, N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = RealT(pnt[i]);
    }
    return ret;
  }

  //! Linear interpolation between two points
  template <typename RealT, IndexSizeT N>
  [[nodiscard]] constexpr Point<RealT, N> lerp(const Point<RealT, N> &a, const Point<RealT, N> &b, RealT t)
  {
    Point<RealT, N> ret;
    for(unsigned i = 0; i < N; i++) {
      ret[i] = std::lerp(a[i], b[i], t);
    }
    return ret;
  }

  //! Check all elements of a matrix are real
  //! That is not nan or infinite
  template <typename RealT, IndexSizeT N, IndexSizeT M>
  [[nodiscard]] bool isReal(Matrix<RealT, N, M> const &m)
  {
    for(auto x : m) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    return true;
  }

  //! Check all elements of a vector are real
  //! That is not nan or infinite
  template <typename RealT, IndexSizeT N>
  [[nodiscard]] bool isReal(Vector<RealT, N> const &v)
  {
    for(auto x : v) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    return true;
  }

  //! Convert to homogeneous coordinates
  template <typename RealT, int N>
  [[nodiscard]] constexpr Point<RealT, N + 1> toHomogeneous(const Point<RealT, N> &pnt)
  {
    Point<RealT, N + 1> ret;
    ret.template head<N>() = pnt;
    ret[N] = RealT(1);
    return ret;
  }

  //! Convert from homogeneous coordinates
  template <typename RealT, int N>
  [[nodiscard]] constexpr Point<RealT, N - 1> fromHomogeneous(const Point<RealT, N> &pnt)
  {
    return pnt.template head<N - 1>() / pnt[N-1];
  }


}// namespace Ravl2
