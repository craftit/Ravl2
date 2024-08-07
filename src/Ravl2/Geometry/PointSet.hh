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
#include "Ravl2/Assert.hh"

namespace Ravl2
{

  //! A set of points in space

  template <typename RealT, unsigned N>
  class PointSet :
      public std::vector<Point<RealT, N>>
  {
  public:
    using PointArrayT = std::vector<Point<RealT, N>>;

    //! Empty list of points.
    PointSet() = default;

    //! Construct from list of points
    explicit PointSet(const std::vector<Point<RealT, N>> &points)
        : std::vector<Point<RealT, N>>(points)
    {}

    //! Returns the centroid (mean) of the points
    [[nodiscard]] Point<RealT, N> PointCentroid() const;

    //! Calculates the barycentric coordinate of point
    [[nodiscard]] std::vector<RealT> BarycentricCoordinate(const Point<RealT, N> &point) const;

    //! Compute the bounding rectangle for the point set.
    [[nodiscard]] Range<RealT, N> BoundingRectangle() const;

    //! translate point set by adding a vector.
    const PointSet &operator+=(const Vector<RealT, N> &offset);

    //! translate point set by subracting a vector.
    const PointSet &operator-=(const Vector<RealT, N> &offset);

    //! scale the point set by multiplying the points by 'scale'.
    const PointSet &operator*=(RealT scale);

    //! Transform the points in place
    template <PointTransform transform>
    const PointSet<RealT, N> &operator*=(const transform &trans)
    {
      for(auto &it : *this)
        *it = trans(*it);
      return *this;
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &archive)
    {
      cereal::size_type numPnts = this->size();
      archive(cereal::make_size_tag(numPnts));
      if(numPnts != this->size()) {
        this->resize(numPnts);
      }
      for(auto &it : *this) {
        archive(it);
      }
    }

  protected:
    static RealT pCot(const Point<RealT, 2> &oPointA, const Point<RealT, 2> &oPointB, const Point<RealT, 2> &oPointC)
    {
      Point<RealT, 2> oBA = (oPointA - oPointB);
      Point<RealT, 2> oBC = (oPointC - oPointB);
      auto cr = std::abs(cross(oBC, oBA));
      if(cr < RealT(1e-6))
        cr = RealT(1e-6);
      return (xt::linalg::dot(oBC, oBA) / cr)();
    }
  };

  template <typename RealT, unsigned N>
  Point<RealT, N> PointSet<RealT, N>::PointCentroid() const
  {
    // Create return value
    Point<RealT, N> centroid {};
    // Sum
    int count = 0;
    for(auto point : (*this)) {
      centroid += point;
      count++;
    }
    // Divide
    centroid /= RealT(count);
    // Done
    return centroid;
  }

  template <typename RealT, unsigned N>
  static RealT PCot(const Point<RealT, N> &oPointA, const Point<RealT, N> &oPointB, const Point<RealT, N> &oPointC)
  {
    auto oBA = oPointA - oPointB;
    auto oBC = oPointC - oPointB;
    RealT cp = std::abs(cross(oBC, oBA));
    if(cp < 1e-6)
      cp = 1e-6;
    return (oBC.dot(oBA) / cp);
  }

  template <typename RealT, unsigned N>
  std::vector<RealT> PointSet<RealT, N>::BarycentricCoordinate(const Point<RealT, N> &point) const
  {
    // Create return array
    std::vector<RealT> oWeights(this->size());
    if(this->size() == 0)
      return oWeights;
    // Keep track of total
    RealT fTotalWeight = 0;
    // For each polygon vertex
    auto res = oWeights.begin();
    const auto end = this->end();
    auto previous = *(this->end() - 1);
    auto next = this->begin() + 1;
    for(auto it : (*this)) {
      RealT sqDist = sumOfSqr(point - it)();
      if(sqDist != 0) {
        RealT fWeight = (pCot(point, it, previous) + pCot(point, it, *next)) / sqDist;
        *res = fWeight;
        fTotalWeight += fWeight;
      } else {
        *res = 1;
        fTotalWeight += 1;
      }
      previous = it;
      ++next;
      if(next == end) {
        next = this->begin();
      }
      ++res;
    }
    // Normalise weights
    for(auto &it : oWeights)
      it /= fTotalWeight;
    // Done
    return oWeights;
  }

  //: Compute the bounding rectangle for the point set.

  template <typename RealT, unsigned N>
  Range<RealT, N> PointSet<RealT, N>::BoundingRectangle() const
  {
    Range<RealT, N> ret;
    auto point = this->begin();
    auto end = this->end();
    if(point == end)
      return ret;// No points in set.
    ret = Range<RealT, N>(*point, 0);
    for(; point != end; ++point)
      ret.involve(*point);
    return ret;
  }

  //: Translate point set by vector.

  template <typename RealT, unsigned N>
  const PointSet<RealT, N> &PointSet<RealT, N>::operator+=(const Vector<RealT, N> &offset)
  {
    for(auto &it : *this)
      it += offset;
    return *this;
  }

  //: Translate point set by subracting a vector.

  template <typename RealT, unsigned N>
  const PointSet<RealT, N> &PointSet<RealT, N>::operator-=(const Vector<RealT, N> &offset)
  {
    for(auto &it : *this)
      it -= offset;
    return *this;
  }

  //: Scale the point set by multiplying the points by 'scale'.

  template <typename RealT, unsigned N>
  const PointSet<RealT, N> &PointSet<RealT, N>::operator*=(RealT scale)
  {
    for(auto &it : *this)
      it *= scale;
    return *this;
  }

  extern template class PointSet<float, 2>;

}// namespace Ravl2
