// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once
//! author="Radek Marik"
//! date="26/02/1994"
//! docentry="Ravl.API.Math.Geometry.3D"

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/PlaneABCD3d.hh"

namespace Ravl2 {


  //: Plane in 3D space
  // The PlanePVV3dC class represents the plane in 3 dimensional Euclidian
  // space. The plane is represented by one point and 2 vectors.

  template<typename RealT>
  class PlanePVV3dC
  {
  public:
    //! Creates the plane P:(0,0,0),V1:[0,0,0],V2:[0,0,0].
    PlanePVV3dC() = default;

    //! Copy constructor.
    PlanePVV3dC(const PlanePVV3dC & plane) = default;


    //! Creates the plane [p; v1; v2].
    inline PlanePVV3dC(const Point<RealT,3>  & p,
                       const Vector<RealT,3> & v1,
                       const Vector<RealT,3> & v2)
        : mOrigin(p),
          vector1(v1),
          vector2(v2)
    {}

    // Creates the plane determined by three points 'p1', 'p2', and 'p3'.
    // The first vector is equal to p2-p1, the second one to p3-p1.
    static PlanePVV3dC<RealT> fromPoints(
                       const Point<RealT,3>  & p1,
		       const Point<RealT,3>  & p2,
		       const Point<RealT,3>  & p3)    
    {
      return PlanePVV3dC<RealT>(p1,p2-p1,p3-p1);
    }
    

    //:-=============================================-
    //: Access to the plane elements and conversions.
    
    inline const Point<RealT,3> & Point() const
    { return mOrigin; }
    // Access to the point of the constant object.
    
    inline const Vector<RealT,3> & Vector1() const
    { return vector1; }
    // Access to the first vector of the constant object.
    
    inline const Vector<RealT,3> & Vector2() const
    { return vector2; }
    // Access to the second vector of the constant object.
    
    inline Point<RealT,3> &origin()
    { return mOrigin; }
    // Access to the point.
    
    inline Vector<RealT,3> & Vector1()
    { return vector1; }
    // Access to the first vector.
    
    inline Vector<RealT,3> & Vector2()
    { return vector2; }
    // Access to the second vector.
    
    inline Vector<RealT,3> Normal() const
    { return vector1.Cross(vector2); }
    // Returns the normal of the plane.
    
    PlaneABCD3dC PlaneABCD3d() const;
    // Converts this plane representation.
    
    inline PlanePVV3dC & UnitVectors() {
      vector1.Unit();
      vector2.Unit();
      return *this;
    }  
    // Normalizes the vectors to be unit.
    
    //:-=============================================-
    //: Geometrical constructions.
    
    RealT EuclideanDistance(const Point<RealT,3> &point) const;
    // Measure the euclidean distance between 'point' and the plane.
    
    Point<RealT,3> ClosestPoint(const Point<RealT,3> &p) const;
    // Find the closest point on the plane to 'p'
    
    Point<RealT,3> Intersection(const LinePV3dC & l) const;
    // Returns the point of intersection of this plane with the line 'l'.
    
    Point<RealT,2> Projection(const Point<RealT,3> & point) const;
    //: Returns the coordinates (t1,t2) of the point projected onto
    //: the plane. 
    // The coordinate system is determined by the point of
    // the plane and its two vectors.
    
    Point<RealT,2> ProjectedIntersection(const LinePV3dC & l) const;
    // Returns the coordinates (t1,t2) of the point of intersection
    // of this plane with the line 'l'. The coordinate system of the returned
    // point is determined by the point of the plane and its two vectors.
    
    inline Point<RealT,3> Point(const RealT t1, const RealT t2) const
    { return Point() + vector1 * t1 + vector2 * t2; }
    // Returns the point of the plane: point + t1 * vector1 + t2 * vector2.
    
    inline Point<RealT,3> Point(const Point<RealT,2> &par) const
    { return Point() + vector1 * par[0] + vector2 * par[1]; }
    // Returns the point of the plane: point + t1 * vector1 + t2 * vector2.
    
  private:
    
    Point<RealT,3> mOrigin;
    Vector<RealT,3> vector1;
    Vector<RealT,3> vector2;
    
    friend std::istream & operator>>(std::istream & inS, PlanePVV3dC & plane);
  };
  
  template<typename RealT>
  std::ostream & operator<<(std::ostream & outS, const PlanePVV3dC<RealT> & plane){
    const Point<RealT,3>  & p  = plane.Point();
    const Vector<RealT,3> & v1 = plane.Vector1();
    const Vector<RealT,3> & v2 = plane.Vector2();
    outS << p << ' ' << v1 << ' ' << v2;
    return(outS);
  }

  template<typename RealT>
  std::istream & operator>>(std::istream & inS, PlanePVV3dC<RealT> & plane){
    Point<RealT,3>  & p  = plane.Point();
    Vector<RealT,3> & v1 = plane.Vector1();
    Vector<RealT,3> & v2 = plane.Vector2();
    inS >> p >> v1 >> v2;
    return(inS);
  }

  //: Least squares fit of a plane to a set of points in 3d
  // At least 3 points are needed.
  template<typename RealT>
  bool FitPlane(const std::vector<Point<RealT,3>> &points,PlanePVV3dC<RealT> &plane);
  
  
}



