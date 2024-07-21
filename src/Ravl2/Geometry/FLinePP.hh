// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  
  //! userlevel=Normal
  //: Line in N dimensional space.
  
  template<typename RealT, unsigned int N>
  class FLinePPC {
  public:
    FLinePPC()
    {}
    //: Default constructor.
    // The line is undefined.
    
    FLinePPC(const Point<RealT, N> &start,const Point<RealT, N> &end)
    { point[0] = start; point[1] = end; }
    //: Construct from two points.
    
    FLinePPC(const Point<RealT,N> &start,const FVectorC<N> &direction)
    { point[0] =start; point[1] = start + direction; }
    //: Construct from a start point and a direction.
    
    const Point<RealT,N> & FirstPoint() const
    { return point[0]; }
    //: Returns the start point of the line segment.
    
    const Point<RealT,N> & SecondPoint() const
    { return point[1]; }
    //: Returns the end point of the line segment.

    Point<RealT,N> & FirstPoint()
    { return point[0]; }
    //: Returns the start point of the line segment.
    
    Point<RealT,N> & SecondPoint()
    { return point[1]; }
    //: Returns the end point of the line segment.
    
    Point<RealT,N> MidPoint() const
    { return (point[1] + point[0])/2.0; }
    //: Returns the mid point of the line segment.
    
    const Point<RealT,N> & P1() const
    { return point[0]; }
    //: Returns the start point of the line segment. 
    // It is equivalent to the function FirstPoint().
    
    const Point<RealT,N> & P2() const
    { return point[1]; }
    // Returns the start point of the line segment. 
    //: It is equivalent to the function SecondPoint().
    
    Point<RealT,N> & P1()
    { return point[0]; }
    //: Returns the start point of the line segment. 
    // It is equivalent to the function FirstPoint().
    
    Point<RealT,N> & P2()
    { return point[1]; }
    //: Returns the start point of the line segment. 
    // It is equivalent to the function SecondPoint().
    
    const Point<RealT,N> & operator[](const UIntT i) const {
      RavlAssertMsg(i == 0 || i == 1,"Index out of range 0..1"); 
      return point[i];
    }
    //: Returns the ith point.

    Point<RealT,N> & operator[](const UIntT i) {
      RavlAssertMsg(i == 0 || i == 1,"Index out of range 0..1"); 
      return point[i];
    }
    //: Returns the ith point.
    
    //:-------------------------
    // Geometrical computations.
    
    FLinePPC<N> operator+(const FVectorC<N> & v) const
    { return FLinePPC<N>(Point<RealT,N>(P1()+v),Point<RealT,N>(P2()+v)); }
    //: Returns the line segment translated into the new position.
    
    FLinePPC<N> & operator+=(const FVectorC<N> & v) {
      point[0] += v;
      point[1] += v;
      return *this;
    }
    //: Moves the line segment into the new position.
    // The operator is equivalent to the member function Translate().
    
    FLinePPC<N> & Translate(const FVectorC<N> & v)
    { return operator+=(v); }
    //: Moves the line segment into the new position.
    // The member function is equivalent to the operator+=.
    
    void Swap() {
      Point<RealT,N> tmp = point[0];
      point[0] = point[1];
      point[1] = tmp;
    }
    //: Swaps the end points of this
    
    FLinePPC<N> Swapped() const
    { return FLinePPC<N>(P2(), P1()); }
    //: Returns a line with swapped endpoints
    
    FVectorC<N> Vector() const
    { return point[1] - point[0]; } 
    //: Returns the direction of the line segment as a free vector.
    // The magnitude of the vector is the length of the line segment.
    
    FLinePPC<N> & FixStart(const Point<RealT,N> & p) {
      FVectorC<N> vec = point[1] - point[0];
      point[0] = p;
      point[1] = p + vec;
      return *this;
    }
    //: Translates the line segment to start in the point 'p'.
    
    FLinePPC<N> & FixEnd(const Point<RealT,N> & p) {
      FVectorC<N> vec = point[1] - point[0];
      point[0] = p - vec;
      point[1] = p;
      return *this;
    }
    //: Translates the line segment to end in the point 'p'.
    
    RealT Length() const
    { return point[0].EuclidDistance(point[1]); }
    //: Returns the length of the line in euclidian space.
    
    Point<RealT,N> PointAt(const RealT t) const
    { return FirstPoint() + Vector() * t; }
    //: Returns the point of the line: FirstPoint() + t * Vector().
    
    RealT ParClosest(const Point<RealT,N> &pnt) const {
      auto v = Vector();
      RealT l2 = v.SumOfSqr();
      if (l2 == 0.0) throw std::underflow_error("FLinePPC::ParClosest(): Cannot find line parameter for zero-length line");
      return v.dot(pnt - point[0]) / l2;
    }
    //: Returns the parameter of the closest point on the line to 'pnt'.
    // Where 0 is at the start point and 1 is at the end. 
    
  protected:
    Point<RealT,N> point[2];
  };

  template<typename RealT, unsigned int N>
  inline
  std::ostream &operator<<(std::ostream &s,const FLinePPC<RealT, N> &dat) {
    s << dat.P1() << ' ' << dat.P2();
    return s;
  }
  
  template<typename RealT, unsigned int N>
  inline
  std::istream &operator>>(std::istream &s,FLinePPC<RealT, N> &dat) {
    s >> dat.P1() >> dat.P2();
    return s;
  }

  
}


#endif
