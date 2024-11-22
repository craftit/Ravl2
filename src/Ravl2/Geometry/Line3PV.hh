// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! docentry="Ravl.API.Math.Geometry.3D"
//! date="06/08/1995"

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/LinePV.hh"

namespace Ravl2
{

  //: Line determined by one point and a vector in 3D space
  // The Line3PV class represents the line in 3 dimensional Euclidean
  // space. The line is represented by one point and its direction vector.

  template <typename RealT>
  class Line3PV : public LinePV<RealT, 3>
  {
  public:
    //:----------------------------------------------
    // Constructors, assigment, copy, and destructor.

    //: Creates the non-existing line (0,0,0) [0,0,0].
    inline Line3PV() = default;

    //: Creates the line passing through the point 'a' and with
    //: the direction 'v'.
    inline Line3PV(const Point<RealT, 3> &a, const Vector<RealT, 3> &v)
        : LinePV<RealT, 3>(a, v)
    {}
    
    //: Returns the line which passes through the closest points
    //: of both lines.
    // The returned line has the first point on this line and
    // the second point on the 'line'.
    [[nodiscard]] LinePV<RealT, 3> shortestLine(const LinePV<RealT, 3> &line) const
    {
      auto axb = cross(this->Direction(), line.Direction());
      RealT axbNorm = sumOfSqr(axb);

      if(isNearZero(axbNorm))
        throw std::runtime_error("LinePV<RealT,3>::ShortestLine(): the lines are almost parallel");
      Vector<RealT, 3> pmr(this->FirstPoint() - line.FirstPoint());
      Point<RealT, 3> p1(this->FirstPoint()
                         + this->Direction() * ((xt::linalg::dot(axb, cross(line.Direction(), pmr))) / axbNorm));
      Point<RealT, 3> p2(line.FirstPoint()
                         + line.Direction() * ((xt::linalg::dot(axb, cross(this->Direction(), pmr))) / axbNorm));
      return LinePV<RealT, 3>::fromPoints(p1, p2);
    }

    //: Returns the point which belongs to both lines.
    // If the lines have no intersection, the function returns the point which
    // lies in the middle of the shortest line segment between both lines.
    [[nodiscard]] Point<RealT, 3> intersection(const LinePV<RealT, 3> &l) const
    {
      return ShortestLine(l).MiddlePoint();
    }

  };
  
  //! Construct a line from two points
  template <typename RealT>
  [[nodiscard]] inline constexpr Line3PV<RealT> toLine(Point<RealT, 3> const &start, Point<RealT, 3> const &end)
  {
    return Line3PV<RealT>(start, end - start);
  }
  
}// namespace Ravl2

#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<Ravl2::Line3PV<float>> : fmt::ostream_formatter {
};
#endif
