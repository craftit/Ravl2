// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_LINE2ABC_HEADER
#define RAVL_LINE2ABC_HEADER 1
/////////////////////////////////////////////////////////////////////////////
//! userlevel=Normal
//! author="Radek Marik"
//! date="26.06.1994"
//! docentry="Ravl.API.Math.Geometry.2D"
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Geometry/Euclidean/2D/LineABC2d.hh"

#include "Ravl/Vector2d.hh"
#include "Ravl/Point2d.hh"

namespace RavlN {
  template<class DataT> class Array1dC;
  
  //: Line in 2D space - equation Ax+By+C = 0
  // The class LineABC2dC represents a line embedded in the 2D plane.
  // The line is represented by the equation aa*x+bb*y+cc = 0.
  
  class LineABC2dC {
  public:
    
    inline LineABC2dC()
      : normal(0.0, 0.0), d(0.0)
    {}
    //: Creates the non-existing line (0,0,0).
    
    inline LineABC2dC(RealT a, RealT b, RealT c)
      : normal(a, b), d(c)
    {}
    //: Creates the line determined by the equation a*x+b*y+c = 0.
    
    inline LineABC2dC(const Point2dC & end, const Point2dC  & start)
      : normal(Vector2dC(end-start).Perpendicular()), d(-normal.Dot(start))
    {}
    //: Creates the line passing through two points 'end' and 'start'.
    //!bug: These args are the wrong way round. Will be fixed in future RAVL versions
    
    inline LineABC2dC(const Vector2dC & norm, const Point2dC  & pt)
      : normal(norm), d(-norm.Dot(pt))
    {}
    //: Creates the line passing through 'pt' with the normal 'norm'.
    
    inline LineABC2dC(const Point2dC  & pt, const Vector2dC & vec)
      : normal(vec.Perpendicular())
    { d = -normal.Dot(pt); }
    //: Creates the line passing through 'pt' with the direction 'vec'
    
    bool FitLSQ(const Array1dC<Point2dC> &points,RealT &residual);
    //: Fit points to a line.
    // 'residual' is from the least squares fit and can be used to assess 
    // the quality of the fit.  Returns false if fit failed.
    
    //:-------------------------------------
    //: Access to the elements of the object.
    
    inline Vector2dC Normal() const
    { return normal; }
    //: Returns the normal of the line.
    
    inline Vector2dC UnitNormal() const
    { return normal / normal.Magnitude(); }
    //: Returns the normal of the line normalized to have unit size.
    
    inline RealT Rho() const
    { return d / normal.Magnitude(); }
    //: Returns the distance of the line from the origin of the coordinate
    //: system.
    
    inline RealT A() const
    { return normal[0]; }
    //: Returns parameter a.
    
    inline RealT B() const
    { return normal[1]; }
    //: Returns parameter b.
    
    inline RealT C() const
    { return d; }
    //: Returns parameter c.
    
    inline RealT ValueX(const RealT y) const
    { return (A() == 0) ? (RealT) 0 : (-B()*y - C()) / A(); }
    //: Returns the value of x coordinate if the y coordinate is known.
    // If the parameter A() is zero, the zero is returned.
    
    inline RealT ValueY(const RealT x) const
    { return (B() == 0) ? (RealT) 0 : (-A()*x - C()) / B(); }
    //: Returns the value of y coordinate if the x coordinate is known.
    // If the parameter B() is zero, the zero is returned.
    
    //:--------------------------
    //: Geometrical constructions.
    
    inline RealT Residuum(const Point2dC & p) const
    { return (normal[0] * p[0] + normal[1] * p[1]) + d; }
    //: Returns the value of the function A()*p[0]+B()*p[1]+C() often
    //: used in geometrical computations.
    
    inline LineABC2dC & MakeUnitNormal();
    //: Normalizes the equation so that the normal vector is unit.
    
    inline bool AreParallel(const LineABC2dC & line) const;
    //: Returns true if the lines are parallel.
    
    inline bool Intersection(const LineABC2dC & line,Point2dC &here) const;
    //: Find the intersection of two lines.
    // If the intersection doesn't exist, the function returns false.
    // The intersection is assigned to 'here'.
    
    inline Point2dC Intersection(const LineABC2dC & line) const;
    //: Returns the intersection of both lines. 
    // If the intersection
    // doesn't exist, the function returns Point2dC(0,0).
    
    inline RealT SqrEuclidDistance(const Point2dC & point) const;
    //: Returns the squared Euclidian distance of the 'point' from the line.
    
    inline RealT SignedDistance(const Point2dC & point) const
    { return Residuum(point) / normal.Magnitude(); }
    //: Returns the signed distance of the 'point' from the line.
    // The return value is greater than 0 if the point is on the left
    // side of the line. The left side of the line is determined
    // by the direction of the normal.
    
    inline RealT Distance(const Point2dC & point) const
    { return Abs(SignedDistance(point)); }
    //: Returns the distance of the 'point' from the line.
    
    inline Point2dC Projection(const Point2dC & point) const
    { return point - normal *(Residuum(point)/normal.SumOfSqr()); }
    //: Returns the point which is the orthogonal projection of the 'point'
    //: to the line. 
    // It is the same as intersection of this line with
    // the perpendicular line passing through the 'point'.
    
  private:
        
    Vector2dC normal;
    // The normal of the line.
  
    RealT     d;
    // The distance of the line from the origin of the coordinate system
    // multiplied by the size of the normal vector of the line.
    
    friend istream & operator>>(istream & inS, LineABC2dC & line);
  };
  
  ostream & operator<<(ostream & outS, const LineABC2dC & line);
  istream & operator>>(istream & inS, LineABC2dC & line);
  
  inline LineABC2dC & LineABC2dC::MakeUnitNormal() {
    RealT size = normal.Magnitude();
    normal /= size;
    d      /= size;
    return *this;
  }
  
  inline bool LineABC2dC::AreParallel(const LineABC2dC & line) const {
    RealT crossSize = Normal().Cross(line.Normal());
    return  IsAlmostZero(crossSize);
  }
  
  inline Point2dC LineABC2dC::Intersection(const LineABC2dC & line) const {
    RealT crossSize = Normal().Cross(line.Normal());
    if ( IsAlmostZero(crossSize) )
      return Point2dC(0.0, 0.0);
    return Point2dC((line.C()*B() - line.B()*C())/crossSize,
		    (line.A()*C() - line.C()*A())/crossSize);
  }

  inline  bool LineABC2dC::Intersection(const LineABC2dC & line,Point2dC &here) const  {
    RealT crossSize = Normal().Cross(line.Normal());
    if ( IsAlmostZero(crossSize) )
      return false;
    here = Point2dC((line.C()*B() - line.B()*C())/crossSize,
		    (line.A()*C() - line.C()*A())/crossSize);
    return true;
  }
  
  inline  RealT LineABC2dC::SqrEuclidDistance(const Point2dC & point) const {
    RealT t = Residuum(point);
    return t*t/normal.SumOfSqr();
  }
  
}
#endif
