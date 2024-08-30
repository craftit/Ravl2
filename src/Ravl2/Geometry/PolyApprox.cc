// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2006, OmniPerception Ltd
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Polygon.hh"
#include "Ravl2/Geometry/LineABC2d.hh"

namespace Ravl2
{

  template <class RealT>
  static std::vector<Point<RealT, 2>> SortSegment(auto &p1, auto &p2, RealT maxDist)
  {
    std::vector<Point<RealT, 2>> ret;
    auto at = p1;
    LineABC2dC<RealT> line(*p1, *p2);
    decltype(p1) fp;
    RealT largestDist = maxDist;
    for(; at != p2; at.NextCrc()) {
      RealT d = line.Distance(*at);
      if(d > largestDist) {
        fp = at;
        largestDist = d;
      }
    }
    // No mid point, return empty list.
    if(!fp) return ret;

    // Do two sub sections either side.
    std::vector<Point<RealT, 2>> tmp = SortSegment(p1, fp, maxDist);
    ret.MoveLast(tmp);
    ret.push_back(*fp);// Insert mid point.
    tmp = SortSegment(fp, p2, maxDist);
    ret.MoveLast(tmp);
    return ret;
  }

  //: Generate an approximation to the given polyline within the given distance limit.

  //  template <class RealT>
  //  PolyLine2dC PolyLine2dC::Approx(RealT distLimit) const {
  //    DLIterC<Point<RealT,2>> first = *this;
  //    if(!first) return PolyLine2dC();
  //    DLIterC<Point<RealT,2>> last = *this; last.Last();
  //    PolyLine2dC ret = SortSegment(first,last,distLimit);
  //    ret.InsFirst(*first);
  //    ret.push_back(*last);
  //    return ret;
  //  }

  template <class RealT>
  Polygon2dC<RealT> Polygon2dC<RealT>::Approx(RealT maxDist) const
  {
    Polygon2dC<RealT> ret;
    auto it = this->begin();
    if(!it) return ret;

    // Find furthest point from start.

    RealT maxPointDist = 0;
    auto maxAt = it;
    Point<RealT, 2> at = *it;
    for(; it; it++) {
      RealT x = at.SqrEuclidDistance(*it);
      if(x <= maxPointDist)
        continue;
      maxPointDist = x;
      maxAt = it;
    }

    // Find furthest point from other furthest point.
    it.First();
    maxPointDist = 0;
    auto max2At = it;
    at = *maxAt;
    for(; it; it++) {
      RealT x = at.SqrEuclidDistance(*it);
      if(x <= maxPointDist)
        continue;
      maxPointDist = x;
      max2At = it;
    }

    // Put in first point.
    ret.push_back(*max2At);

    // Sort out first segment
    std::vector<Point<RealT, 2>> tmp = SortSegment(max2At, maxAt, maxDist);
    ret.MoveLast(tmp);

    // Insert mid point.
    ret.push_back(*maxAt);

    // And the last.
    tmp = SortSegment(maxAt, max2At, maxDist);
    ret.MoveLast(tmp);

    return ret;
  }

}// namespace Ravl2
