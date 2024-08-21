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
#include "Ravl2/Geometry/VectorOffset.hh"

namespace Ravl2
{

  //: Plane in 3D space - equation Ax+By+Cz+D = 0

  template <typename RealT>
  class PlaneABCD3dC
  {
  public:
    //: The non-existing plane (0,0,0,0).
    PlaneABCD3dC() = default;

    //: The plane determined by the equation aa*x+bb*y+cc*z+dd = 0.
    PlaneABCD3dC(RealT aa, RealT bb, RealT cc, RealT dd)
      : mNormal(toVector<RealT>(aa, bb, cc)),
        mD(dd)
    {}

    //: Creates the plane determined by the vector equation n*x = p.
    PlaneABCD3dC(const Vector<RealT, 3> &n, RealT p)
        : mNormal(n),
          mD(-p)
    {}

    //: Creates the plane with normal 'n' passing through the point 'p'.
    PlaneABCD3dC(const Vector<RealT, 3> &n, const Point<RealT, 3> &p)
        : mNormal(n),
          mD(-(xt::linalg::dot(n,p)()))
    {}

    //: The plane [p; v1; v2].
    PlaneABCD3dC(const Point<RealT, 3> &p,
                const Vector<RealT, 3> &v1,
                const Vector<RealT, 3> &v2)
        : mNormal(cross(v1,v2))
    {
      mD = RealT(-xt::linalg::dot(mNormal,p)());
    }

    //: The plane passing through three points 'p1', 'p2', and 'p3'.
    static inline PlaneABCD3dC<RealT> fromPoints(const Point<RealT, 3> &p1,
                                                const Point<RealT, 3> &p2,
                                                const Point<RealT, 3> &p3)
    {
      return PlaneABCD3dC<RealT>(p1, p2 - p1, p3 - p1);
    }

    //: Returns the normal of the plane.
    inline Vector<RealT, 3> Normal() const
    {
      return (mNormal);
    }

    //: Returns parameter a.
    inline RealT A() const
    {
      return (mNormal[0]);
    }

    //: Returns parameter b.
    inline RealT B() const
    {
      return (mNormal[1]);
    }

    //: returns parameter c.
    inline RealT C() const
    {
      return (mNormal[2]);
    }

    //: Returns parameter d.
    RealT D() const
    {
      return (mD);
    }

    //: Normalizes the normal vector to be unit.
    inline PlaneABCD3dC &UnitNormal()
    {
      RealT mag = RealT(norm_l2(mNormal)());
      mNormal /= mag;
      mD /= mag;
      return (*this);
    }

    //: Returns the value of the expression 'normal.Dot(p) + d'
    // This is often used in analytical geometry.
    [[nodiscard]] inline RealT Value(const Point<RealT, 3> &p) const
    {
      return xt::linalg::dot(mNormal,p)() + mD;
    }

    //: Find the closest point to 'p' that lies on the plane.
    [[nodiscard]] Point<RealT, 3> ClosestPoint(const Point<RealT, 3> &p) const
    {
      return p + mNormal * (-(xt::linalg::dot(mNormal,p) + mD) / sumOfSqr(mNormal));
    }

    //: Returns the directed Euclidean distance of the point 'p' from
    //: this plane.
    // If returned value is positive the point is in the direction of the
    // plane normal.
    [[nodiscard]] inline RealT DirectedEuclideanDistance(const Point<RealT, 3> &p) const
    {
      return Value(p) / norm_l2(mNormal);
    }

    //: Returns the directed Euclidian distance of the point 'p' from
    //: this plane.
    // If returned value is positive the point is in the direction of the
    // plane normal.
    // Obsolete, use DirectedEuclideanDistance()
    [[nodiscard]] inline RealT DirEuclidDistance(const Point<RealT, 3> &p) const
    {
      return DirectedEuclideanDistance(p);
    }

    //: Returns the Euclidean distance of the point 'p' from this plane.
    [[nodiscard]] inline RealT EuclideanDistance(const Point<RealT, 3> &p) const
    {
      return std::abs(DirEuclidDistance(p));
    }

    //: Returns the Euclidean distance of the point 'p' from this plane.
    inline RealT EuclidDistance(const Point<RealT, 3> &p) const
    {
      return EuclideanDistance(p);
    }

    //: Returns the plane parallel to this plane and passing through
    //: the point 'p'.
    inline PlaneABCD3dC ParallelPlane(const Point<RealT, 3> &p) const
    {
      return PlaneABCD3dC(Normal(), p);
    }

#if 0

    Point<RealT, 3> Intersection(const LinePV3dC &l) const;
    //: Returns the point which is the intersection of this plane with
    //: the line 'l'.
    // If the intersection does not exist the function throw an ExceptionNumericalC

    Point<RealT, 3> Intersection(const PlaneABCD3dC &planeB,
                                 const PlaneABCD3dC &planeC) const;
    //: Returns the point which is the intersection of three planes.
    // If the intersection does not exist the function throw an ExceptionNumericalC

    LinePV3dC<RealT> Intersection(const PlaneABCD3dC &plane) const;
    //: Returns the line which is the intersection of this plane with
    // the plane 'plane'.
    // If the intersection does not exist the function throw an ExceptionNumericalC
#endif
  private:
    Vector<RealT, 3> mNormal;
    RealT mD;
  };

  //: Least squares fit of a plane to a set of points in 3d
  // At least 3 points are needed.
  template <typename RealT>
  bool FitPlane(const std::vector<Point<RealT, 3>> &points, PlaneABCD3dC<RealT> &plane);

  template <typename RealT>
  std::ostream &operator<<(std::ostream &outS, const PlaneABCD3dC<RealT> &plane)
  {
    outS << plane.Normal() << ' ' << plane.D();
    return (outS);
  }

  template <typename RealT>
  std::istream &operator>>(std::istream &inS, PlaneABCD3dC<RealT> &plane)
  {
    Vector<RealT, 3> dir;
    RealT d;
    inS >> dir >> d;
    plane = PlaneABCD3dC<RealT>(dir, d);
    return (inS);
  }

  extern template class PlaneABCD3dC<float>;


}// namespace Ravl2
