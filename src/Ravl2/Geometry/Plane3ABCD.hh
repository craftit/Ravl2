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
#include "Ravl2/Geometry/Line3PV.hh"
#include "Ravl2/Geometry/FitVectorOffset.hh"

namespace Ravl2
{

  //! @brief Plane in 3D space - equation Ax+By+Cz+D = 0

  template <typename RealT>
  class Plane3ABCD : public VectorOffset<RealT, 3>
  {
  public:
    //: The non-existing plane (0,0,0,0).
    Plane3ABCD() = default;

    //: The plane determined by the equation aa*x+bb*y+cc*z+dd = 0.
    Plane3ABCD(RealT aa, RealT bb, RealT cc, RealT dd)
        : VectorOffset<RealT, 3>(toVector<RealT>(aa, bb, cc), dd)
    {}

    //: Creates the plane determined by the vector equation n*x = p.
    Plane3ABCD(const Vector<RealT, 3> &n, RealT p)
        : VectorOffset<RealT, 3>(n, p)
    {}

    //: Creates the plane with normal 'n' passing through the point 'p'.
    Plane3ABCD(const Vector<RealT, 3> &n, const Point<RealT, 3> &p)
        : VectorOffset<RealT, 3>(n, p)
    {}

    //: The plane [p; v1; v2].
    Plane3ABCD(const Point<RealT, 3> &p,
               const Vector<RealT, 3> &v1,
               const Vector<RealT, 3> &v2)
        : VectorOffset<RealT, 3>(cross(v1, v2), -cross(v1, v2).dot(p))
    {}

    //: The plane passing through three points 'p1', 'p2', and 'p3'.
    static inline Plane3ABCD<RealT> fromPoints(const Point<RealT, 3> &p1,
                                               const Point<RealT, 3> &p2,
                                               const Point<RealT, 3> &p3)
    {
      return Plane3ABCD<RealT>(p1, p2 - p1, p3 - p1);
    }

    //: Returns parameter a.
    [[nodiscard]] inline RealT A() const
    {
      return (this->mNormal[0]);
    }

    //: Returns parameter b.
    [[nodiscard]] inline RealT B() const
    {
      return (this->mNormal[1]);
    }

    //: returns parameter c.
    [[nodiscard]] inline RealT C() const
    {
      return (this->mNormal[2]);
    }

    //: Returns the normal of the plane.
    [[nodiscard]] inline Vector<RealT, 3> normal() const
    {
      return this->mNormal;
    }

    //: Returns parameter d.
    [[nodiscard]] RealT D() const
    {
      return (this->mD);
    }

    //: Returns the plane parallel to this plane and passing through
    //: the point 'p'.
    [[nodiscard]] inline Plane3ABCD<RealT> parallelPlane(const Point<RealT, 3> &p) const
    {
      return Plane3ABCD(this->normal(), p);
    }

    //! @brief Returns the point which is the intersection of this plane with the line 'l'.
    //! If the intersection does not exist the function throw an ExceptionNumericalC
    [[nodiscard]] Point<RealT, 3> intersection(const LinePV<RealT, 3> &l) const
    {
      RealT nu = this->mNormal.dot(l.Direction());
      if(isNearZero(nu))
        throw std::runtime_error("PlaneABCD3dC::intersection(): the line is almost parallel to the plane.");
      return l.at(-this->residuum(l.FirstPoint()) / nu);
    }

    //! @brief Returns the point which is the intersection of three planes.
    //! If the intersection does not exist the function throw an ExceptionNumericalC
    [[nodiscard]] Point<RealT, 3> intersection(const Plane3ABCD<RealT> &planeB,
                                               const Plane3ABCD<RealT> &planeC) const
    {
      Vector<RealT, 3> n1xn2(cross(this->normal(), planeB.normal()));
      RealT tripleSP = n1xn2.dot(planeC.normal());
      if(isNearZero(tripleSP))
        throw std::runtime_error("PlaneABCD3dC::intersection(): the planes are almost parallel");
      Vector<RealT, 3> n2xn3(cross(planeB.normal(), planeC.normal()));
      Vector<RealT, 3> n3xn1(cross(planeC.normal(), this->normal()));
      return Point<RealT, 3>(n2xn3 * D() + n3xn1 * planeB.D() + n1xn2 * planeC.D()) / (-tripleSP);
    }

    //: Returns the line which is the intersection of this plane with
    // the plane 'plane'.
    // If the intersection does not exist the function throw an ExceptionNumericalC

    Line3PV<RealT> intersection(const Plane3ABCD<RealT> &plane) const
    {
      Vector<RealT, 3> direction(cross(this->normal(), plane.normal()));
      RealT den = sumOfSqr(direction);
      if(isNearZero(den))
        throw std::runtime_error("PlaneABCD3dC::intersection(): the planes are almost parallel");
      Vector<RealT, 3> n212(cross(plane.normal(), direction));
      Vector<RealT, 3> n121(cross(direction, this->normal()));
      return Line3PV<RealT>((n212 * D() + n121 * plane.D()) / (-den), direction);
    }
  };

  //! Least squares fit of a plane to a set of points in 3d
  //! At least 3 points are needed.
  template <typename RealT>
  bool fit(Plane3ABCD<RealT> &plane, const std::vector<Point<RealT, 3>> &points)
  {
    return fit(static_cast<VectorOffset<RealT, 3> &>(plane), points);
  }

  extern template class Plane3ABCD<float>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT>
struct fmt::formatter<Ravl2::Plane3ABCD<RealT>> : fmt::ostream_formatter {
};
#endif

