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
      inline LinePV() = default;


      //! Creates the line passing through the point 'a' and with
      //! the direction 'v'.
      inline LinePV(const Point<RealT,N> &a, const Vector<RealT,N> &v)
          : mPoint(a), mDirection(v)
      {}

      //: Creates the line passing through the points 'first' and
      //: second.
      static LinePV<RealT,N> fromPoints(const Point<RealT,N> &first, const Point<RealT,N> &second)
      {
        return LinePV<RealT,N>(first, second - first);
      }

      //: Returns the point of the line.

      [[nodiscard]] inline const Point<RealT,N> &FirstPoint() const
      {
        return mPoint;
      }

      //: Returns the point which is the end point of the direction
      //: vector placed in the first point of the line.
      [[nodiscard]] inline Point<RealT,N> SecondPoint() const
      {
        return mPoint + mDirection;
      }

      //: Returns the point which is in the middle of the FirstPoint()
      //: and the SecondPoint() of the line segment.
      [[nodiscard]] inline Point<RealT,N> MiddlePoint() const
      {
        return PointT(0.5);
      }

      //: Returns the point FirstPoint() + lambda * direction.
      // Obsolete, use Point(l)
      [[nodiscard]] Point<RealT,N> PointT(RealT lambda) const
      {
        return mPoint + mDirection * lambda;
      }

      //: Returns the point FirstPoint() + lambda * direction.
      [[nodiscard]] Point<RealT,N> at(RealT lambda) const
      {
        return mPoint + mDirection * lambda;
      }

      //: Return the paramiter of the closest point on the line to 'pnt'.
      // Use at() to get to the corresponding point on the line.
      [[nodiscard]] RealT ParClosest(const Point<RealT,N> &pnt) const
      {
        return (xt::linalg::dot(mDirection,(pnt - mPoint)) / xt::sum(mDirection * mDirection))();
      }

      //: Returns the direction vector of this line.
      inline const Vector<RealT,N> &lineDirection() const
      {
        return mDirection;
      }

      //: Returns the direction vector of this line.
      [[nodiscard]] inline const Vector<RealT,N> &Direction() const
      {
        return mDirection;
      }

      void MakeDirectionUnitDirection()
      {
        mDirection /= norm_l2(mDirection);
      }
      //: Make the direction part of the line a unit vector.

//      LinePP<RealT,N> LinePP() const;
//      //: Returns the line segment represented by the start point and the end
//      //: point.
//      // The start point is equal to the start point of this line.
//      // The end point of the returned line is determined by the sum of the
//      // start point and the direction vector of this line.


      //: Returns the shortest distance between the lines.
      [[nodiscard]] RealT Distance(const LinePV<RealT,N> &line) const
      {
        if constexpr (N==3) {
          // more information in Rektorys:
          // Prehled uzite matematiky, SNTL, Praha 1988, p. 205
          auto axb = cross(Direction(),line.Direction());
          auto modul = Ravl2::norm_l2(axb);
          if (isNearZero(modul)) {
            return line.Distance(FirstPoint());
          }
          return std::abs(RealT(xt::linalg::dot(line.FirstPoint() - FirstPoint(), axb)())) / modul;
        } else {
          return 0;
        }
      }

      //: Returns the distance of the point 'p' from this line.
      [[nodiscard]] RealT Distance(const Point<RealT,N> &p) const
      {
        return RealT(norm_l2(cross(Direction(),(FirstPoint() - p)) / norm_l2(Direction()))());
      }

#if 0
      //: Returns the line which passes through the closest points
      //: of both lines.
      // The returned line has the first point on this line and
      // the second point on the 'line'.
      [[nodiscard]] LinePV<RealT,N> ShortestLine(const LinePV<RealT,N> & line) const {
        auto axb = cross(Direction(),line.Direction());
        RealT axbNorm = sumOfSqr(axb);

        if (isNearZero(axbNorm))
          throw std::runtime_error("LinePV<RealT,N>::ShortestLine(): the lines are almost parallel");
        Vector<RealT,N> pmr(FirstPoint() - line.FirstPoint());
        Point<RealT,N> p1(FirstPoint()
                           + Direction() * ((xt::linalg::dot(axb,cross(line.Direction(),pmr))) / axbNorm);
        Point<RealT,N> p2(line.FirstPoint()
                           + line.Direction() * ((xt::linalg::dot(axb,cross(Direction(),pmr))) / axbNorm));
        return LinePV<RealT,N>::fromPoints(p1, p2);
      }

      //: Returns the point which belongs to both lines.
      // If the lines have no intersection, the function returns the point which
      // lies in the middle of the shortest line segment between both lines.
      [[nodiscard]] Point<RealT,N> Intersection(const LinePV<RealT,N> &l) const
      { return ShortestLine(l).MiddlePoint(); }
#endif

#if 0
    LinePV<RealT,N> ProjectionInto(const PlaneABCD3dC & p) const;
    // Returns the line which is the orthogonal projection of this
    // line into the plane 'p'.
#endif
    private:
      Point<RealT,N> mPoint;     // the first point of the line
      Vector<RealT,N> mDirection;// the direction of the line
    };


#if 0
LinePV<RealT,N> LinePV<RealT,N>::ProjectionInto(const PlaneABCD3dC & p) const {
  return LinePV<RealT,N>(FirstPoint(),
                   Direction() - Direction().ProjectionInto(p.Normal()));
}
#endif

    // Saves the 'line' into the output stream.
    template <typename RealT, unsigned int N>
    std::ostream & operator<<(std::ostream & outS, const LinePV<RealT,N> & line) {
      outS << line.FirstPoint() << ' ' << line.SecondPoint();
      return outS;
    }

    // Sets the 'line' according to data read from the input stream.
    template <typename RealT, unsigned int N>
    std::istream & operator>>(std::istream & inS, LinePV<RealT,N> & line) {
      Point<RealT,N> p1;
      Point<RealT,N> p2;
      inS >> p1 >> p2;
      line = LinePV<RealT,N>::fromPoints(p1,p2);
      return inS;
    }

    // Let everyone know there's an implementation already generated for common cases
    extern template class LinePV<float, 2>;
    extern template class LinePV<float, 3>;


}
