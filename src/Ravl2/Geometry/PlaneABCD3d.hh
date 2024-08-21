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
#include "Ravl2/Geometry/VectorOffset.hh"

namespace Ravl2
{

  //: Plane in 3D space - equation Ax+By+Cz+D = 0

  template <typename RealT>
  class PlaneABCD3dC
  {
  public:
    PlaneABCD3dC()
        : normal(0.0, 0.0, 0.0),
          d(0.0)
    {}
    //: The non-existing plane (0,0,0,0).

    PlaneABCD3dC(RealT aa, RealT bb, RealT cc, RealT dd)
        : normal(aa, bb, cc),
          d(dd)
    {}
    //: The plane determined by the equation aa*x+bb*y+cc*z+dd = 0.

    PlaneABCD3dC(const Vector<RealT, 3> &n, RealT p)
        : normal(n),
          d(-p)
    {}
    //: Creates the plane determined by the vector equation n*x = p.

    PlaneABCD3dC(const Vector<RealT, 3> &n, const Point<RealT, 3> &p)
        : normal(n),
          d(-(n.Dot(p)))
    {}
    //: Creates the plane with normal 'n' passing through the point 'p'.

    inline PlaneABCD3dC(const Point<RealT, 3> &p,
                        const Vector<RealT, 3> &v1,
                        const Vector<RealT, 3> &v2)
        : normal(v1.Cross(v2))
    {
      d = -normal.Dot(p);
    }
    //: The plane [p; v1; v2].

    inline PlaneABCD3dC(const Point<RealT, 3> &p1,
                        const Point<RealT, 3> &p2,
                        const Point<RealT, 3> &p3)
        : normal(Vector<RealT, 3>(p2 - p1).Cross(Vector<RealT, 3>(p3 - p1)))
    {
      d = -normal.Dot(p1);
    }
    //: The plane passing through three points 'p1', 'p2', and 'p3'.

    inline Vector<RealT, 3> Normal() const
    {
      return (normal);
    }
    //: Returns the normal of the plane.

    inline RealT A() const
    {
      return (normal[0]);
    }
    //: Returns parameter a.

    inline RealT B() const
    {
      return (normal[1]);
    }
    //: Returns parameter b.

    inline RealT C() const
    {
      return (normal[2]);
    }
    //: returns parameter c.

    RealT D() const
    {
      return (d);
    }
    //: Returns parameter d.

    inline PlaneABCD3dC &UnitNormal()
    {
      RealT size = normal.Magnitude();
      normal /= size;
      d /= size;
      return (*this);
    }
    //: Normalizes the normal vector to be unit.

    inline RealT Value(const Point<RealT, 3> &p) const
    {
      return normal.Dot(p) + d;
    }
    //: Returns the value of the expression 'normal.Dot(p) + d'
    // This is often used in analytical geometry.

    Point<RealT, 3> ClosestPoint(const Point<RealT, 3> &p) const
    {
      return p + normal * (-(normal.Dot(p) + d) / normal.SumOfSqr());
    }
    //: Find closest point to 'p' that lies on the plane.

    inline RealT DirectedEuclideanDistance(const Point<RealT, 3> &p) const
    {
      return Value(p) / normal.Magnitude();
    }
    //: Returns the directed Euclidian distance of the point 'p' from
    //: this plane.
    // If returned value is positive the point is in the direction of the
    // plane normal.

    inline RealT DirEuclidDistance(const Point<RealT, 3> &p) const
    {
      return DirectedEuclideanDistance(p);
    }
    //: Returns the directed Euclidian distance of the point 'p' from
    //: this plane.
    // If returned value is positive the point is in the direction of the
    // plane normal.
    // Obsolete, use DirectedEuclideanDistance()

    inline RealT EuclideanDistance(const Point<RealT, 3> &p) const
    {
      return std::abs(DirEuclidDistance(p));
    }
    //: Returns the Euclidean distance of the point 'p' from this plane.

    inline RealT EuclidDistance(const Point<RealT, 3> &p) const
    {
      return EuclideanDistance(p);
    }
    //: Returns the Euclidean distance of the point 'p' from this plane.

    inline PlaneABCD3dC ParallelPlane(const Point<RealT, 3> &p) const
    {
      return PlaneABCD3dC(Normal(), p);
    }
    //: Returns the plane parallel to this plane and passing through
    //: the point 'p'.

    Point<RealT, 3> Intersection(const LinePV3dC &l) const;
    //: Returns the point which is the intersection of this plane with
    //: the line 'l'.
    // If the intersection does not exist the function throw an ExceptionNumericalC

    Point<RealT, 3> Intersection(const PlaneABCD3dC &planeB,
                                 const PlaneABCD3dC &planeC) const;
    //: Returns the point which is the intersection of three planes.
    // If the intersection does not exist the function throw an ExceptionNumericalC

    LinePV3dC Intersection(const PlaneABCD3dC &plane) const;
    //: Returns the line which is the intersection of this plane with
    // the plane 'plane'.
    // If the intersection does not exist the function throw an ExceptionNumericalC

  private:
    Vector<RealT, 3> normal;
    RealT d;

    friend std::istream &operator>>(std::istream &inS, PlaneABCD3dC &plane);
  };

  //: Least squares fit of a plane to a set of points in 3d
  // At least 3 points are needed.
  template <typename RealT>
  bool FitPlane(const std::vector<Point<RealT, 3>> &points, PlaneABCD3dC &plane);

  template <typename RealT>
  std::ostream &operator<<(std::ostream &outS, const PlaneABCD3dC &plane)
  {
    outS << plane.Normal() << ' ' << plane.D();
    return (outS);
  }

  template <typename RealT>
  std::istream &operator>>(std::istream &inS, PlaneABCD3dC &plane)
  {
    inS >> plane.normal >> plane.d;
    return (inS);
  }

}// namespace Ravl2
