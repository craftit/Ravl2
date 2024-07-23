// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="James Smith"
//! date="26/9/2002"

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{

  class Affine2dC;
  class Projection2dC;

  //! A set of points in space

  template<typename RealT,unsigned N>
  class PointSet :
      public std::vector<Point<RealT,N> >
  {
  public:
    using PointArrayT = std::vector<Point<RealT,N>>;

    //! Empty list of points.
    PointSet() = default;

    //! Construct from list of points
    explicit PointSet(const std::vector<Point<RealT,N>> & points)
      : std::vector<Point<RealT,N> >(points)
    {}

    //! Returns the centroid (mean) of the points
    Point<RealT,N> Centroid() const;

    //! Calculates the barycentric coordinate of point
    std::vector<RealT> BarycentricCoordinate(const Point<RealT,N>& point) const;

    //! Compute the bounding rectangle for the point set.
    Range<RealT,N> BoundingRectangle() const;

    //! Translate point set by adding a vector.
    const PointSet &operator+=(const Vector<RealT,N> &offset);

    //! Translate point set by subracting a vector.
    const PointSet &operator-=(const Vector<RealT,N> &offset);

    //! Scale the point set by multiplying the points by 'scale'.
    const PointSet &operator*=(RealT scale);

    //! Transform the points in place
    template<PointTransform transform>
    const PointSet<RealT,N> &operator*=(const transform &trans) {
      for(auto &it : *this)
        *it = trans(*it);
      return *this;
    }

  };


  template<typename RealT,unsigned N>
  Point<RealT,N> PointSet<RealT,N>::Centroid() const {
    // Create return value
    Point<RealT,N> centroid {};
    // Sum
    int size = 0;
    for (auto point : (*this)) {
      centroid += point;
      size++;
    }
    // Divide
    centroid /= (RealT) size;
    // Done
    return centroid;
  }

  template<typename RealT,unsigned N>
  static RealT PCot(const Point<RealT,N>& oPointA, const Point<RealT,N>& oPointB, const Point<RealT,N>& oPointC) {
    auto oBA = oPointA - oPointB;
    auto oBC = oPointC - oPointB;
    RealT cp = std::abs(cross(oBC, oBA));
    if (cp < 1e-6)
      cp = 1e-6;
    return (oBC.dot(oBA) / cp);
  }

  template<typename RealT,unsigned N>
  std::vector<RealT> PointSet<RealT,N>::BarycentricCoordinate(const Point<RealT,N>& point) const
  {
    // Create return array
    std::vector<RealT> oWeights(this->size());
    // Keep track of total
    RealT fTotalWeight = 0;
    // For each polygon vertex
    auto res = oWeights.begin();
    for (auto it : (*this)) {
      RealT sqDist = Vector2dC(point - it).SumOfSqr();
      if (sqDist != 0) {
        RealT fWeight = (PCot(point,it.Data(),it.PrevCrcData()) +
                         PCot(point,it.Data(),it.NextCrcData())) / sqDist;
        res.Data() = fWeight;
        fTotalWeight += fWeight;
      }
      else {
        res = 1;
        fTotalWeight += 1;
      }
      res++;
    }
    // Normalise weights
    oWeights /= fTotalWeight;
    // Done
    return oWeights;
  }

  //: Compute the bounding rectangle for the point set.

  template<typename RealT,unsigned N>
  Range<RealT,N> PointSet<RealT,N>::BoundingRectangle() const {
    Range<RealT,N> ret(0,0);
    auto point = this->begin();
    auto end = this->end();
    if(point == end)
      return ret; // No points in set.
    ret = RealRange<RealT,N>(*point,0);
    for (; point != end; ++point)
      ret.Involve(*point);
    return ret;
  }

  //: Translate point set by vector.

  template<typename RealT,unsigned N>
  const PointSet<RealT,N> &PointSet<RealT,N>::operator+=(const Vector<RealT,N> &offset) {
    for(auto & it : *this)
      it += offset;
    return *this;
  }

  //: Translate point set by subracting a vector.

  template<typename RealT,unsigned N>
  const PointSet<RealT,N> &PointSet<RealT,N>::operator-=(const Vector<RealT,N> &offset) {
    for(auto &it : *this)
      it -= offset;
    return *this;
  }

  //: Scale the point set by multiplying the points by 'scale'.

  template<typename RealT,unsigned N>
  const PointSet<RealT,N> &PointSet<RealT,N>::operator*=(RealT scale) {
    for(auto &it : *this)
      it *= scale;
    return *this;
  }




}
