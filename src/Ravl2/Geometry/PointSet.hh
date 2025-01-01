// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="James Smith"
//! date="26/9/2002"

#pragma once

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
    using iterator = typename PointArrayT::iterator;
    using const_iterator = typename PointArrayT::const_iterator;
    using value_type = typename PointArrayT::value_type;

    //! Empty list of points.
    constexpr PointSet() = default;

    //! Construct from list of points
    constexpr explicit PointSet(const std::vector<Point<RealT, N>> &points)
        : std::vector<Point<RealT, N>>(points)
    {}

    //! Returns the centroid (mean) of the points
    [[nodiscard]] constexpr Point<RealT, N> pointCentroid() const;

    //! Calculates the barycentric coordinate of point
    [[nodiscard]] constexpr std::vector<RealT> BarycentricCoordinate(const Point<RealT, N> &point) const;

    //! Compute the bounding rectangle for the point set.
    [[nodiscard]] constexpr Range<RealT, N> boundingRectangle() const;

    //! translate point set by adding a vector.
    constexpr const PointSet &operator+=(const Vector<RealT, N> &offset);

    //! translate point set by subtracting a vector.
    constexpr const PointSet &operator-=(const Vector<RealT, N> &offset);

    //! scale the point set by multiplying the points by 'scale'.
    constexpr const PointSet &operator*=(RealT scale);
    
    //! scale the point set by multiplying the points by 'scale'.
    constexpr PointSet operator*(RealT scale) const {
      PointSet<RealT, N> ret;
      ret.reserve(this->size());
      for(auto it : *this)
        ret.push_back(it * scale);
      return ret;
    }
    
    
    //! Transform the points in place
    template <PointTransform transform>
    constexpr const PointSet<RealT, N> &operator*=(const transform &trans)
    {
      for(auto &it : *this)
        it = trans(it);
      return *this;
    }

    //! The assumes the points form a closed boundary.
    //! That is, they are in order and the last point is connected to the first.
    [[nodiscard]] RealT perimeter() const
    {
      RealT perimeter = 0;
      if(!this->empty()) {
        auto pLast = this->back();
        for(auto ptr : *this) {
          perimeter += euclidDistance(pLast, ptr);
          pLast = ptr;
        }
      }
      return perimeter;
    }

  protected:
    static constexpr RealT pCot(const Point<RealT, 2> &oPointA, const Point<RealT, 2> &oPointB, const Point<RealT, 2> &oPointC)
    {
      Point<RealT, 2> oBA = (oPointA - oPointB);
      Point<RealT, 2> oBC = (oPointC - oPointB);
      auto cr = std::abs(cross(oBC, oBA));
      if(cr < RealT(1e-6))
        cr = RealT(1e-6);
      return oBC.dot(oBA) / cr;
    }
  };

  template <typename RealT, unsigned N>
  constexpr Point<RealT, N> PointSet<RealT, N>::pointCentroid() const
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
  constexpr std::vector<RealT> PointSet<RealT, N>::BarycentricCoordinate(const Point<RealT, N> &point) const
  {
    if constexpr(N != 2) {
      throw std::runtime_error("BarycentricCoordinate only implemented for 2D points");
    } else {
      // Create return array
      std::vector<RealT> oWeights(this->size());
      if(this->size() == 0)
	return oWeights;
      // Keep track of total
      RealT fTotalWeight = 0;
      // For each polygon vertex
      auto res = oWeights.begin();
      const auto end = this->end();
      Point<RealT, N> previous = *(this->end() - 1);
      auto next = this->begin() + 1;
      for(const auto &it : (*this)) {
	RealT sqDist = sumOfSqr(point - it);
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
  }

  //! Compute the bounding rectangle for the point set.

  template <typename RealT, unsigned N>
  constexpr Range<RealT, N> PointSet<RealT, N>::boundingRectangle() const
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

  //! Translate point set by vector.

  template <typename RealT, unsigned N>
  constexpr const PointSet<RealT, N> &PointSet<RealT, N>::operator+=(const Vector<RealT, N> &offset)
  {
    for(auto &it : *this)
      it += offset;
    return *this;
  }

  //! Translate point set by subracting a vector.

  template <typename RealT, unsigned N>
  constexpr const PointSet<RealT, N> &PointSet<RealT, N>::operator-=(const Vector<RealT, N> &offset)
  {
    for(auto &it : *this)
      it -= offset;
    return *this;
  }

  //! Scale the point set by multiplying the points by 'scale'.

  template <typename RealT, unsigned N>
  constexpr const PointSet<RealT, N> &PointSet<RealT, N>::operator*=(RealT scale)
  {
    for(auto &it : *this)
      it *= scale;
    return *this;
  }

  template <typename RealT, unsigned N>
  std::ostream &operator<<(std::ostream &s, const PointSet<RealT, N> &dat)
  {
    s << dat.size() << std::endl;
    for(auto it : dat) {
      s << ' ' << Eigen::WithFormat(it, defaultEigenFormat()) << std::endl;
    }
    return s;
  }

  extern template class PointSet<float, 2>;
  extern template class PointSet<float, 3>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<Ravl2::PointSet<float, 2>> : fmt::ostream_formatter {
};
template <>
struct fmt::formatter<Ravl2::PointSet<double, 2>> : fmt::ostream_formatter {
};
template <>
struct fmt::formatter<Ravl2::PointSet<float, 3>> : fmt::ostream_formatter {
};
template <>
struct fmt::formatter<Ravl2::PointSet<double, 3>> : fmt::ostream_formatter {
};
#endif
