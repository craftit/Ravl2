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

  template<typename RealT>
  class Quad
    : std::array<Point<RealT,2>,4>
  {
  public:
    //! Default constructor, points are not initialised.
    Quad() = default;

    //! Construct a rectangle from a range.
    Quad(const Range<RealT,2> &range,BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT)
    {
      (*this)[0] = range.origin();
      (*this)[2] = range.max();
      if(orientation == BoundaryOrientationT::INSIDE_LEFT) {
        // Clockwise
        (*this)[1] = toPoint<RealT>(range.max(0),range.min(1));
        (*this)[3] = toPoint<RealT>(range.min(0),range.max(1));
      } else {
        // Counterclockwise
        (*this)[1] = toPoint<RealT>(range.min(0),range.max(1));
        (*this)[3] = toPoint<RealT>(range.max(0),range.min(1));
      }
    }

    //! Construct a quad from 4 points
    Quad(const Point<RealT,2> &p0, const Point<RealT,2> &p1, const Point<RealT,2> &p2, const Point<RealT,2> &p3)
    {
      (*this)[0] = p0;
      (*this)[1] = p1;
      (*this)[2] = p2;
      (*this)[3] = p3;
    }

    //! From initializer list
    Quad(std::initializer_list<Point<RealT,2>> list)
    {
      if(list.size() != 4)
            throw std::invalid_argument("Quad<RealT> initializer list must have 4 points");
      std::copy(list.begin(),list.end(),this->begin());
    }

    //! Access the first point
    [[nodiscard]] const Point<RealT,2> &p0() const
    { return (*this)[0]; }

    //! Access the second point
    [[nodiscard]] const Point<RealT,2> &p1() const
    { return (*this)[1]; }

    //! Access the third point
    [[nodiscard]] const Point<RealT,2> &p2() const
    { return (*this)[2]; }

    //! Access the fourth point
    [[nodiscard]] const Point<RealT,2> &p3() const
    { return (*this)[3]; }


  };

  template <typename RealT>
  [[nodiscard]] inline Quad<RealT> toQuad(const Range<RealT, 2> &range, BoundaryOrientationT orientation = BoundaryOrientationT::INSIDE_LEFT)
  {
    return Quad<RealT>(range, orientation);
  }


  // Instantiate the class for float and double
  extern template class Quad<float>;
  extern template class Quad<double>;

}// namespace Ravl2

