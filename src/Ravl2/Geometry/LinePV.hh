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

namespace Ravl2
{

  //! @brief Line determined by one point and a direction vector in N-D space

  template <typename RealT, unsigned int N>
  class LinePV
  {
  public:
    //! Creates the non-existing line (0,0,0) [0,0,0].
    LinePV() = default;

    //! Creates the line passing through the point 'a' and with
    //! the direction 'v'.
    inline LinePV(const Point<RealT, N> &a, const Vector<RealT, N> &v)
        : mPoint(a), mDirection(v)
    {}

    //: Creates the line passing through the points 'first' and
    //: second.
    [[nodiscard]] static LinePV<RealT, N> fromPoints(const Point<RealT, N> &first, const Point<RealT, N> &second)
    {
      return LinePV<RealT, N>(first, second - first);
    }

    //: Returns the point of the line.
    [[nodiscard]] inline const Point<RealT, N> &FirstPoint() const
    {
      return mPoint;
    }

    //: Returns the point which is the end point of the direction
    //: vector placed in the first point of the line.
    [[nodiscard]] inline Point<RealT, N> SecondPoint() const
    {
      return mPoint + mDirection;
    }

    //: Returns the point which is in the middle of the FirstPoint()
    //: and the SecondPoint() of the line segment.
    [[nodiscard]] inline Point<RealT, N> MiddlePoint() const
    {
      return PointT(0.5);
    }

    //: Returns the point FirstPoint() + lambda * direction.
    // Obsolete, use Point(l)
    [[nodiscard]] Point<RealT, N> PointT(RealT lambda) const
    {
      return mPoint + mDirection * lambda;
    }

    //: Returns the point FirstPoint() + lambda * direction.
    [[nodiscard]] Point<RealT, N> at(RealT lambda) const
    {
      return mPoint + mDirection * lambda;
    }

    //: Return the parameter of the closest point on the line to 'pnt'.
    // Use at() to get to the corresponding point on the line.
    [[nodiscard]] RealT ParClosest(const Point<RealT, N> &pnt) const
    {
      return (xt::linalg::dot(mDirection, (pnt - mPoint)) / xt::sum(mDirection * mDirection))();
    }

    //: Returns the direction vector of this line.
    [[nodiscard]] inline const Vector<RealT, N> &lineDirection() const
    {
      return mDirection;
    }

    //: Returns the direction vector of this line.
    [[nodiscard]] inline const Vector<RealT, N> &Direction() const
    {
      return mDirection;
    }

    //: Make the direction part of the line a unit vector.
    void MakeDirectionUnitDirection()
    {
      mDirection /= norm_l2(mDirection);
    }

    //: Returns the shortest distance between the lines.
    [[nodiscard]] RealT Distance(const LinePV<RealT, N> &line) const
    {
      if constexpr(N == 3) {
        // more information in Rektorys:
        // Prehled uzite matematiky, SNTL, Praha 1988, p. 205
        auto axb = cross(Direction(), line.Direction());
        auto modul = Ravl2::norm_l2(axb);
        if(isNearZero(modul)) {
          return line.Distance(FirstPoint());
        }
        return std::abs(RealT(xt::linalg::dot(line.FirstPoint() - FirstPoint(), axb)())) / modul;
      } else {
        return 0;
      }
    }

    //: Returns the distance of the point 'p' from this line.
    [[nodiscard]] RealT Distance(const Point<RealT, N> &p) const
    {
      return RealT(norm_l2(cross(Direction(), (FirstPoint() - p)) / norm_l2(Direction()))());
    }

    //! IO Handling
    template <class Archive>
    void serialize(Archive &archive)
    {
      archive(cereal::make_nvp("point", mPoint), cereal::make_nvp("direction", mDirection));
    }

  protected:
    Point<RealT, N> mPoint;     // the first point of the line
    Vector<RealT, N> mDirection;// the direction of the line
  };

#if 0
LinePV<RealT,N> LinePV<RealT,N>::ProjectionInto(const PlaneABCD3dC & p) const {
  return LinePV<RealT,N>(FirstPoint(),
                   Direction() - Direction().ProjectionInto(p.Normal()));
}
#endif

  // Saves the 'line' into the output stream.
  template <typename RealT, unsigned int N>
  std::ostream &operator<<(std::ostream &outS, const LinePV<RealT, N> &line)
  {
    outS << line.FirstPoint() << ' ' << line.SecondPoint();
    return outS;
  }

  // Sets the 'line' according to data read from the input stream.
  template <typename RealT, unsigned int N>
  std::istream &operator>>(std::istream &inS, LinePV<RealT, N> &line)
  {
    Point<RealT, N> p1;
    Point<RealT, N> p2;
    inS >> p1 >> p2;
    line = LinePV<RealT, N>::fromPoints(p1, p2);
    return inS;
  }

  // Let everyone know there's an implementation already generated for common cases
  extern template class LinePV<float, 2>;
  extern template class LinePV<float, 3>;

}// namespace Ravl2
