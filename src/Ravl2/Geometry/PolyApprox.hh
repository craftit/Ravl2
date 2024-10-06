//
// Created by charles galambos on 11/08/2024.
//

#pragma once

#include <vector>
#include "Ravl2/Geometry/Polygon.hh"
#include "Ravl2/Geometry/PolyLine.hh"
#include "Ravl2/Geometry/Line2ABC.hh"
#include "Ravl2/Geometry/Line3PV.hh"
#include "Ravl2/LoopIter.hh"

namespace Ravl2
{
  namespace detail
  {
    template <class RealT,size_t N>
    void approxSegment(std::vector<Point<RealT,N> > &ret, auto p1, auto p2, RealT maxDist)
    {
      auto at = p1;
      auto fp = p1;
      auto line = toLine<RealT>(*p1, *p2);
      ++at;
      if(at == p2) {
        return;
      }
      RealT largestDist = maxDist;
      for(;at != p2;++at) {
        RealT d = line.distance(*at);
        if(d > largestDist) {
          fp = at;
          largestDist = d;
        }
      }
      if(fp == p1) {
        return; // No point on the line is further than maxDist.
      }
      
      // Do two subsections either side.
      approxSegment(ret, p1, fp, maxDist);
      ret.push_back(*fp);
      approxSegment(ret, fp, p2, maxDist);
    }
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
  Polygon<RealT> Polygon<RealT>::approx(RealT maxDist) const
  {
    Polygon<RealT> ret;
    ret.reserve(this->size());
    auto it = this->begin();
    // Deal with special cases.
    switch(this->size()) {
      case 0:
        return ret;
      case 1:
        ret.push_back(*it);
        return ret;
      case 2:
        ret.push_back(*it);
        ++it;
        ret.push_back(*it);
        return ret;
    }
    const auto end = this->end();
    
    // Find the furthest point from start.
    
    RealT maxPointDist = -1;
    auto maxAt = it;
    Point<RealT, 2> at = *it;
    for(; it != end; ++it) {
      RealT x = euclidDistance(at,*it);
      if(x <= maxPointDist)
        continue;
      maxPointDist = x;
      maxAt = it;
    }
    
    // Find the furthest point from other furthest point.
    it = this->begin();
    maxPointDist = -1;
    auto max2At = it;
    at = *maxAt;
    for(; it != end; it++) {
      RealT x = euclidDistance(at,*it);
      if(x <= maxPointDist)
        continue;
      maxPointDist = x;
      max2At = it;
    }
    
    if(max2At < maxAt) {
      // Swap the two points so that maxAt is before max2At.
      std::swap(maxAt,max2At);
    }
    
    ret.push_back(*maxAt);
    detail::approxSegment(ret, maxAt,max2At , maxDist);
    ret.push_back(*max2At);
    detail::approxSegment(ret, loopIter(*this,max2At), loopIter(*this,maxAt), maxDist);
    return ret;
  }

  template <typename RealT, unsigned N>
  PolyLine<RealT, N> PolyLine<RealT,N>::approx(RealT distLimit) const
  {
    PolyLine<RealT, N> ret;
    ret.reserve(this->size());
    auto it = this->begin();
    // Deal with special cases.
    switch(this->size()) {
      case 0: break;
      case 1:
        ret.push_back(*it);
        break;
      case 2:
        ret.push_back(*it);
        ++it;
        ret.push_back(*it);
        break;
      default: {
        ret.push_back(*it);
        detail::approxSegment(ret, it, this->end()-1, distLimit);
        ret.push_back(this->back());
      } break;
    }
    
    return ret;
  }
  
  
}