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
#include "Ravl2/Geometry/LinePP.hh"

namespace Ravl2
{
  
  //! @brief A 2d curve consisting of straight line segments.
  
  template <typename RealT, unsigned N>
  class PolyLine
    : public PointSet<RealT,N>
  {
  public:
    //! Empty list of points.
    constexpr PolyLine() = default;

    //! Construct from list of points
    constexpr explicit PolyLine(const std::vector<Point<RealT,N>>& points)
      : PointSet<RealT,2>(points)
    {}

    //! @brief Returns true if the polygon is self intersecting.
    //! If any line segments cross each other then the polygon is self intersecting.
    [[nodiscard]] constexpr bool isSelfIntersecting() const {
      auto ft = this->begin();
      const auto theEnd = this->end();
      if(!ft) return false;
      while(true) {
        LinePP<RealT,N> l1(ft.Data(), ft.NextData());
        ft++;
        auto it2 = ft;
        if (it2 == theEnd) break;
        for (; it2; it2++) {
          LinePP<RealT,N> l2(it2.Data(), it2.NextData());
          if (l1.HasInnerIntersection(l2))
            return true;
        }
      }
      return false;
    }

    //! @brief Generate an approximation to the given polyline within the given euclidean distance limit.
    //! This routine generates the approximation by searching for the furthest point from the
    //! line defined by the two end points and if it is further than the distance limit adding it
    //! to the approximation. The procedure is then repeated for each of the segments either side
    //! of the furthest point.
    [[nodiscard]] PolyLine<RealT,N> Approx(RealT distLimit) const;

    //! Measure the length of the poly line in euclidean space.
    [[nodiscard]] constexpr RealT length() const {
      auto ft = this->begin();
      const auto theEnd = this->end();
      if(ft == theEnd) return 0;
      Point<RealT,2> last = *ft;
      RealT len = 0;
      for(ft++;ft != theEnd;ft++) {
        len += euclidDistance(last,*ft);
        last = *ft;
      }
      return len;
    }
  };
}

