// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.06.1994"

#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  //! Line in 2D space - equation Ax+By+C = 0
  // The class LineABC2dC represents a line embedded in the 2D plane.
  // The line is represented by the equation aa*x+bb*y+cc = 0.

  template<typename RealT>
  class LineABC2dC {
  public:
    //: Creates a degenerate line (0,0,0).
    inline LineABC2dC()
      : normal({0.0, 0.0}), d(0.0)
    {}

    //! Creates the line determined by the equation a*x+b*y+c = 0.
    inline LineABC2dC(RealT a, RealT b, RealT c)
      : normal(a, b), d(c)
    {}

    //! Creates the line determined by the equation norm[0]*x+norm[1]*y+c = 0.
    inline LineABC2dC(Vector<RealT,2> norm, RealT vd)
        : normal(norm), d(vd)
    {}

    //! Creates the line passing through two points 'end' and 'start'.
    inline LineABC2dC(const Point<RealT,2>  & start, const Point<RealT,2> & end)
      : normal(Vector<RealT,2>(end-start).Perpendicular()), d(-dot(normal,start))
    {}

    //! Creates the line passing through two points 'end' and 'start'.
    static LineABC2dC<RealT> fromPoints(const Point<RealT,2> & start, const Point<RealT,2> & end)
    { return LineABC2dC<RealT>(start, end); }

    //! Creates the line passing through 'pt' with the normal 'norm'.
    static LineABC2dC<RealT> fromNormal(const Vector<RealT,2> & norm, const Point<RealT,2> & pt)
    { return LineABC2dC<RealT>(norm, -norm.dot(pt)); }

    //! Creates the line passing through 'pt' with the direction 'vec'
    static LineABC2dC<RealT> fromDirection(const Point<RealT,2> & pt, const Vector<RealT,2> & vec)
    {
      auto normal = perpendicular(vec);
      return LineABC2dC<RealT>(normal, -dot(normal,pt));
    }

    bool FitLSQ(const std::vector<Point<RealT,2>> &points,RealT &residual);
    //: Fit points to a line.
    // 'residual' is from the least squares fit and can be used to assess 
    // the quality of the fit.  Returns false if fit failed.
    
    //:-------------------------------------
    //: Access to the elements of the object.
    
    inline Vector<RealT,2> Normal() const
    { return normal; }
    //: Returns the normal of the line.
    
    inline Vector<RealT,2> UnitNormal() const
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
    { return (A() == 0) ? 0.0 : (-B()*y - C()) / A(); }
    //: Returns the value of x coordinate if the y coordinate is known.
    // If the parameter A() is zero, the zero is returned.
    
    inline RealT ValueY(const RealT x) const
    { return (B() == 0) ? 0.0 : (-A()*x - C()) / B(); }
    //: Returns the value of y coordinate if the x coordinate is known.
    // If the parameter B() is zero, the zero is returned.
    
    //:--------------------------
    //: Geometrical constructions.
    
    inline RealT Residuum(const Point<RealT,2> & p) const
    { return (normal[0] * p[0] + normal[1] * p[1]) + d; }
    //: Returns the value of the function A()*p[0]+B()*p[1]+C() often
    //: used in geometrical computations.
    
    inline LineABC2dC & MakeUnitNormal();
    //: Normalizes the equation so that the normal vector is unit.
    
    inline bool AreParallel(const LineABC2dC & line) const;
    //: Returns true if the lines are parallel.
    
    inline bool Intersection(const LineABC2dC & line,Point<RealT,2> &here) const;
    //: Find the intersection of two lines.
    // If the intersection doesn't exist, the function returns false.
    // The intersection is assigned to 'here'.
    
    inline Point<RealT,2> Intersection(const LineABC2dC & line) const;
    //: Returns the intersection of both lines. 
    // If the intersection
    // doesn't exist, the function returns Point<RealT,2>(0,0).
    
    inline RealT SqrEuclidDistance(const Point<RealT,2> & point) const;
    //: Returns the squared Euclidian distance of the 'point' from the line.
    
    inline RealT SignedDistance(const Point<RealT,2> & point) const
    { return Residuum(point) / normal.Magnitude(); }
    //: Returns the signed distance of the 'point' from the line.
    // The return value is greater than 0 if the point is on the left
    // side of the line. The left side of the line is determined
    // by the direction of the normal.
    
    inline RealT Distance(const Point<RealT,2> & point) const
    { return std::abs(SignedDistance(point)); }
    //: Returns the distance of the 'point' from the line.
    
    inline Point<RealT,2> Projection(const Point<RealT,2> & point) const
    { return point - normal *(Residuum(point)/normal.SumOfSqr()); }
    //: Returns the point which is the orthogonal projection of the 'point'
    //: to the line. 
    // It is the same as intersection of this line with
    // the perpendicular line passing through the 'point'.
    
  private:
        
    Vector<RealT,2> normal;
    // The normal of the line.
  
    RealT     d = 0.0;
    // The distance of the line from the origin of the coordinate system
    // multiplied by the size of the normal vector of the line.

  };

  template<typename RealT>
  std::ostream & operator<<(std::ostream & outS, const LineABC2dC<RealT> & line);

  template<typename RealT>
  std::istream & operator>>(std::istream & inS, LineABC2dC<RealT> & line);

  template<typename RealT>
  inline LineABC2dC<RealT> & LineABC2dC<RealT>::MakeUnitNormal() {
    RealT size = normal.Magnitude();
    normal /= size;
    d      /= size;
    return *this;
  }

  template<typename RealT>
  inline bool LineABC2dC<RealT>::AreParallel(const LineABC2dC<RealT> & line) const {
    RealT crossSize = Normal().Cross(line.Normal());
    return  IsAlmostZero(crossSize);
  }

  template<typename RealT>
  inline Point<RealT,2> LineABC2dC<RealT>::Intersection(const LineABC2dC<RealT> & line) const {
    RealT crossSize = Normal().Cross(line.Normal());
    if ( IsAlmostZero(crossSize) )
      return Point<RealT,2>(0.0, 0.0);
    return Point<RealT,2>((line.C()*B() - line.B()*C())/crossSize,
		    (line.A()*C() - line.C()*A())/crossSize);
  }

  template<typename RealT>
  inline  bool LineABC2dC<RealT>::Intersection(const LineABC2dC<RealT> & line,Point<RealT,2> &here) const  {
    RealT crossSize = Normal().Cross(line.Normal());
    if ( IsAlmostZero(crossSize) )
      return false;
    here = Point<RealT,2>((line.C()*B() - line.B()*C())/crossSize,
		    (line.A()*C() - line.C()*A())/crossSize);
    return true;
  }

  template<typename RealT>
  inline  RealT LineABC2dC<RealT>::SqrEuclidDistance(const Point<RealT,2> & point) const {
    RealT t = Residuum(point);
    return t*t/normal.SumOfSqr();
  }
  
}
