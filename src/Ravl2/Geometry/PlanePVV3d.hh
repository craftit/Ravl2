// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26/02/1994"
//! docentry="Ravl.API.Math.Geometry.3D"

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/PlaneABCD3d.hh"

namespace Ravl2
{

  //! @brief Plane in 3D space, represented by one point and two vectors.

  template <typename RealT>
  class PlanePVV3dC
  {
  public:
    //! Creates the plane P:(0,0,0),V1:[0,0,0],V2:[0,0,0].
    PlanePVV3dC() = default;

    //! Copy constructor.
    PlanePVV3dC(const PlanePVV3dC &plane) = default;

    //! Creates the plane [p; v1; v2].
    inline PlanePVV3dC(const Point<RealT, 3> &p,
                       const Vector<RealT, 3> &v1,
                       const Vector<RealT, 3> &v2)
        : mOrigin(p),
          mVector1(v1),
          mVector2(v2)
    {}

    //! Creates the plane determined by three points 'p1', 'p2', and 'p3'.
    //! The first vector is equal to p2-p1, the second one to p3-p1.
    static PlanePVV3dC<RealT> fromPoints(
      const Point<RealT, 3> &p1,
      const Point<RealT, 3> &p2,
      const Point<RealT, 3> &p3)
    {
      return PlanePVV3dC<RealT>(p1, p2 - p1, p3 - p1);
    }
    
    //! Access to the first vector of the constant object.
    inline const Vector<RealT, 3> &Vector1() const
    {
      return mVector1;
    }

    //! Access to the second vector of the constant object.
    inline const Vector<RealT, 3> &Vector2() const
    {
      return mVector2;
    }

    //! Access to the point.
    inline Point<RealT, 3> &origin()
    {
      return mOrigin;
    }

    //! Access to the first vector.
    inline Vector<RealT, 3> &Vector1()
    {
      return mVector1;
    }

    //! Access to the second vector.
    inline Vector<RealT, 3> &Vector2()
    {
      return mVector2;
    }

    //! Returns the normal of the plane.
    [[nodiscard]] Vector<RealT, 3> Normal() const
    {
      return cross(mVector1, mVector2);
    }

    //! Normalizes the vectors to be unit.
    inline PlanePVV3dC &makeUnitVectors()
    {
      mVector1 /= xt::norm_l2(mVector1);
      mVector2 /= xt::norm_l2(mVector2);
      return *this;
    }

    //! Converts this plane representation to PlaneABCD3dC.
    [[nodiscard]] PlaneABCD3dC<RealT> PlaneABCD3d() const
    {
      return PlaneABCD3dC(Normal(), this->mOrigin);
    }

    //! Returns the point of intersection of this plane with the line 'l'.
    [[nodiscard]] Point<RealT, 3> Intersection(const LinePV3dC<RealT> &l) const
    {
      return PlaneABCD3d().Intersection(l);
    }

    //! Get the euclidean distance of the point 'point' from this plane.
    [[nodiscard]] RealT distance(const Point<RealT, 3> &point) const
    {
      return PlaneABCD3d().distance(point);
    }

    Point<RealT, 3> ClosestPoint(const Point<RealT, 3> &p) const
    {
      return PlaneABCD3d().projection(p);
    }

    //! Returns the coordinates (t1,t2) of the point projected onto the plane.
    //! The coordinate system is determined by the point of
    //! the plane and its two vectors.
    [[nodiscard]] Point<RealT, 2> Projection(const Point<RealT, 3> &pointOnPlane) const
    {
      Matrix<RealT, 3, 2> a;
      at(0, 0) = mVector1[0];
      at(1, 0) = mVector1[1];
      at(2, 0) = mVector1[2];
      at(0, 1) = mVector2[0];
      at(1, 1) = mVector2[1];
      at(2, 1) = mVector2[2];
      Point<RealT, 3> tmp = pointOnPlane;
      tmp -= mOrigin;
      auto [sol, residual, rank, s] = xt::linalg::lstsq(a, tmp);
      return toPoint<RealT>(sol(0, 0), sol(1, 0));
    }

    //! Returns the coordinates (t1,t2) of the point of intersection
    //! of this plane with the line 'l'. The coordinate system of the returned
    //! point is determined by the point of the plane and its two vectors.
    [[nodiscard]] Point<RealT, 2> ProjectedIntersection(const LinePV3dC<RealT> &l) const
    {
      return Projection(Intersection(l));
    }

    //! Returns the point of the plane: point + t1 * mVector1 + t2 * mVector2.
    inline Point<RealT, 3> at(const RealT t1, const RealT t2) const
    {
      return mOrigin + mVector1 * t1 + mVector2 * t2;
    }

    //! Returns the point of the plane: point + t1 * mVector1 + t2 * mVector2.
    inline Point<RealT, 3> at(const Point<RealT, 2> &par) const
    {
      return mOrigin + mVector1 * par[0] + mVector2 * par[1];
    }

    //! IO Handling
    template <class Archive>
    void serialize(Archive &archive)
    {
      archive(cereal::make_nvp("origin", mOrigin),
            cereal::make_nvp("vector1", mVector1),
            cereal::make_nvp("vector2", mVector2));
    }

  private:
    Point<RealT, 3> mOrigin;
    Vector<RealT, 3> mVector1;
    Vector<RealT, 3> mVector2;
  };

  template <typename RealT>
  std::ostream &operator<<(std::ostream &outS, const PlanePVV3dC<RealT> &plane)
  {
    const Point<RealT, 3> &p = plane.Origin();
    const Vector<RealT, 3> &v1 = plane.Vector1();
    const Vector<RealT, 3> &v2 = plane.Vector2();
    outS << p << ' ' << v1 << ' ' << v2;
    return (outS);
  }

  template <typename RealT>
  std::istream &operator>>(std::istream &inS, PlanePVV3dC<RealT> &plane)
  {
    Point<RealT, 3> &p = plane.Origin();
    Vector<RealT, 3> &v1 = plane.Vector1();
    Vector<RealT, 3> &v2 = plane.Vector2();
    inS >> p >> v1 >> v2;
    return (inS);
  }

  //! Least squares fit of a plane to a set of points in 3d
  //! At least 3 points are needed.
  template <typename RealT>
  bool FitPlane(PlanePVV3dC<RealT> &plane, const std::vector<Point<RealT, 3>> &points);

  //! Instantiate the template for float and double.
  extern template class PlanePVV3dC<float>;
  extern template class PlanePVV3dC<double>;
  
}// namespace Ravl2
