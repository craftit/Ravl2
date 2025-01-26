//
// Created by charles on 04/01/25.
//

#pragma once

#include <array>
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{

  //! 4 points in 2D space.

  template<typename RealT,unsigned N,BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT>
   requires (N > 1)
  class Quad
      : public std::array<Point<RealT,N>,4>
  {
  public:
    //! Default constructor, points are not initialised.
    Quad() = default;

    //! Construct a rectangle from a range.
    explicit Quad(const Range<RealT,2> &range)
    {
      auto makePoint = [](const Point<RealT,2> &p) -> Point<RealT,N>
      {
        if constexpr (N == 2) {
          return p;
        } else {
          Point<RealT,N> ret {};
          for(unsigned i = 0; i < 2; i++) {
            ret[i] = p[i];
          }
          return ret;
        }
      };

      (*this)[0] = makePoint(range.origin());
      (*this)[2] = makePoint(range.max());
      if constexpr(orientation == BoundaryOrientationT::INSIDE_LEFT) {
        // Clockwise
        (*this)[1] = makePoint(toPoint<RealT>(range.max(0),range.min(1)));
        (*this)[3] = makePoint(toPoint<RealT>(range.min(0),range.max(1)));
      } else {
        // Counterclockwise
        (*this)[1] = makePoint(toPoint<RealT>(range.min(0),range.max(1)));
        (*this)[3] = makePoint(toPoint<RealT>(range.max(0),range.min(1)));
      }
    }

    //! Construct a quad from 4 points
    Quad(const Point<RealT,N> &p0, const Point<RealT,N> &p1, const Point<RealT,N> &p2, const Point<RealT,N> &p3)
    {
      (*this)[0] = p0;
      (*this)[1] = p1;
      (*this)[2] = p2;
      (*this)[3] = p3;
    }

    //! From initializer list
    Quad(std::initializer_list<Point<RealT,N>> list)
    {
      if(list.size() != 4) {
        throw std::invalid_argument("Quad<RealT> initializer list must have 4 points");
      }
      std::copy(list.begin(),list.end(),this->begin());
    }

    //! Access the first point
    [[nodiscard]] const Point<RealT,N> &p0() const
    { return (*this)[0]; }

    //! Access the second point
    [[nodiscard]] const Point<RealT,N> &p1() const
    { return (*this)[1]; }

    //! Access the third point
    [[nodiscard]] const Point<RealT,N> &p2() const
    { return (*this)[2]; }

    //! Access the fourth point
    [[nodiscard]] const Point<RealT,N> &p3() const
    { return (*this)[3]; }


  };

  //! Convert a range to a quad
  template <typename RealT,unsigned N,BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT>
  [[nodiscard]] inline Quad<RealT,N> toQuad(const Range<RealT, N> &range)
  {
    return Quad<RealT,N>(range);
  }

  //! Convert an index range to a quad
  template <typename RealT,unsigned N,BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT>
  [[nodiscard]] inline Quad<RealT,N> toQuad(const IndexRange<N> &range)
  {
    return Quad<RealT,N>(toRange<RealT>(range));
  }

  //! Put a polygon through a transformation
  template <typename RealT,unsigned N,typename TransformT>
    requires PointTransform<TransformT,RealT,N>
  [[nodiscard]] Quad<RealT,N> operator*(const TransformT &transform, const Quad<RealT,N> &quad)
  {
    Quad<RealT,N> ret;
    for(unsigned i = 0; i < 4; i++) {
      ret[i] = transform(quad[i]);
    }
    return ret;
  }

  template <typename RealT, unsigned N>
  std::ostream &operator<<(std::ostream &s, const Quad<RealT, N> &dat)
  {
    for(auto it : dat) {
      s << ' ' << Eigen::WithFormat(it, defaultEigenFormat()) << std::endl;
    }
    return s;
  }

  // Instantiate the class for float and double
  extern template class Quad<float,2>;
  extern template class Quad<double,2>;
  extern template class Quad<float,3>;
  extern template class Quad<double,3>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
 template<typename RealT,unsigned N>
struct fmt::formatter<Ravl2::Quad<RealT,N>> : fmt::ostream_formatter {
};
#endif
