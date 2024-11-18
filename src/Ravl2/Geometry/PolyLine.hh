// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="16/2/2006"
//! docentry="Ravl.API.Math.Geometry.2D"

#pragma once

#include "Ravl2/Geometry/PointSet.hh"
#include "Ravl2/Geometry/Line2PP.hh"

namespace Ravl2
{

  //! @brief A 2d curve consisting of straight line segments.

  template <typename RealT, unsigned N>
  class PolyLine : public PointSet<RealT, N>
  {
  public:
    //! Empty list of points.
    constexpr PolyLine() = default;

    //! Construct from list of points
    constexpr explicit PolyLine(const std::vector<Point<RealT, N>> &points)
        : PointSet<RealT, N>(points)
    {}

    //! @brief Returns true if the polygon is self intersecting.
    //! If any line segments cross each other then the polygon is self intersecting.
    [[nodiscard]] constexpr bool isSelfIntersecting() const
    {
      if constexpr(N == 2) {
        const auto theEnd = this->end();
        for(auto ft = this->begin();ft != theEnd;++ft) {
          auto next = ft;
          next++;
          if(next == theEnd) break;
          Line2PP<RealT> l1(*ft, *next);
          for(auto it2 = next;it2 != theEnd;++it2) {
            auto next2 = it2;
            next2++;
            if(next2 == theEnd) break;
            Line2PP<RealT> l2(*it2, *next2);
            if(l1.HasInnerIntersection(l2))
              return true;
          }
        }
      }
      return false;
    }

    //! @brief Generate an approximation to the given polyline within the given euclidean distance limit.
    //! This routine generates the approximation by searching for the furthest point from the
    //! line defined by the two end points and if it is further than the distance limit adding it
    //! to the approximation. The procedure is then repeated for each of the segments either side
    //! of the furthest point.
    [[nodiscard]] PolyLine<RealT, N> approx(RealT distLimit) const;

    //! Measure the length of the poly line in euclidean space.
    [[nodiscard]] constexpr RealT length() const
    {
      auto ft = this->begin();
      const auto theEnd = this->end();
      if(ft == theEnd) return 0;
      Point<RealT, N> last = *ft;
      RealT len = 0;
      for(ft++; ft != theEnd; ft++) {
        len += euclidDistance(last, *ft);
        last = *ft;
      }
      return len;
    }
  };
  
  //! Let the compiler know that we will use these classes with the following types
  extern template class PolyLine<float,2>;
  extern template class PolyLine<float,3>;
  
}// namespace Ravl2

#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<Ravl2::PolyLine<float,2>> : fmt::ostream_formatter {
};
template <>
struct fmt::formatter<Ravl2::PolyLine<float,3>> : fmt::ostream_formatter {
};
#endif
