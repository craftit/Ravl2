// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.04.1994"


#include <algorithm>
#include <spdlog/spdlog.h>
#include "Ravl2/Image/Segmentation/Boundary.hh"
#include "Ravl2/Assert.hh"
#include "Ravl2/Geometry/Polygon2d.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //! Creates a boundary around the rectangle.
  Boundary toBoundary(IndexRange<2> rect, BoundaryOrientationT type)
  {
    auto origin = rect.min();
    auto endP = rect.max();
    BoundaryVertex oVertex(origin);// to help to GNU C++ 2.6.0
    CrackCode cr(CrackCodeT::CR_DOWN);
    CrackC edge(oVertex, cr);
    std::vector<CrackC> edges;

    for(int i = origin[0]; i <= endP[0]; i++) {
      edges.push_back(edge);
      edge.moveDown();
    }
    edge.turnCClock();
    for(int i = origin[1]; i <= endP[1]; i++) {
      edges.push_back(edge);
      edge.moveRight();
    }
    edge.turnCClock();
    for(int i = endP[1]; i >= origin[1]; i--) {
      edges.push_back(edge);
      edge.moveUp();
    }
    edge.turnCClock();
    for(int i = endP[0]; i >= origin[0]; i--) {
      edges.push_back(edge);
      edge.moveLeft();
    }
    return Boundary(std::move(edges), type);
  }

  int Boundary::area() const
  {
    int area = 0;// region area
    for(auto edge : mEdges) {
      // 5 or 2
      switch(edge.crackCode()) {
        case CrackCodeT::CR_DOWN: area -= edge.at()[1]; break;
        case CrackCodeT::CR_UP: area += edge.at()[1]; break;
        case CrackCodeT::CR_RIGHT:
        case CrackCodeT::CR_LEFT: break;
        case CrackCodeT::CR_NODIR: break;
      }
    }
    if(orientation == BoundaryOrientationT::INSIDE_RIGHT)
      return -area;
    return area;
  }

  //! reverse the order of the edges.
  [[nodiscard]] Boundary Boundary::reverse() const
  {
    std::vector<CrackC> newEdges;
    newEdges.reserve(mEdges.size());
    auto end = mEdges.rend();
    for(auto it = mEdges.rbegin(); it != end; ++it) {
      newEdges.push_back(it->reversed());
    }
    return Boundary(std::move(newEdges), Ravl2::reverse(orientation));
  }

  Boundary &Boundary::BReverse()
  {
    *this = reverse();
    return *this;
  }

  IndexRange<2> Boundary::boundingBox() const
  {
    IndexRange<2> bb;
    if(empty())
      return bb;
    bb = IndexRange<2>::mostEmpty();
    if(orientation == BoundaryOrientationT::INSIDE_LEFT) {
      ONDEBUG(SPDLOG_INFO("Boundary::boundingBox(), Object is on left. "));
      for(auto edge : mEdges) {
        Index<2> vx = edge.leftPixel();
        bb.involve(vx);
      }
    } else {
      ONDEBUG(SPDLOG_INFO("Boundary::boundingBox(), Object is on right. "));
      for(auto edge : mEdges) {
        Index<2> vx = edge.rightPixel();
        bb.involve(vx);
      }
    }
    return bb;
  }
  

  std::unordered_map<BoundaryVertex, std::array<BoundaryVertex, 2>>  Boundary::CreateHashtable() const
  {
    std::unordered_map<BoundaryVertex, std::array<BoundaryVertex, 2>> hashtable;
    hashtable.reserve(mEdges.size());
    for(const auto &edge : mEdges)
    {
      BoundaryVertex const bvertex1(edge.vertexBegin());
      BoundaryVertex const bvertex2(edge.vertexEnd());
      BoundaryVertex const invalid_vertex(-1, -1);

      auto at = hashtable.find(bvertex1);
      if(at == hashtable.end()) {
        hashtable.emplace(bvertex1, std::array<BoundaryVertex, 2> {bvertex2, invalid_vertex});
      } else {
        hashtable.emplace(bvertex1, std::array<BoundaryVertex,2>{at->second[0], bvertex2});
      }

      at = hashtable.find(bvertex2);
      if(at == hashtable.end()) {
        hashtable.emplace(bvertex2, std::array<BoundaryVertex, 2> {bvertex1, invalid_vertex});
      } else {
        hashtable.emplace(bvertex2, std::array<BoundaryVertex,2>{at->second[0], bvertex1});
      }
    }

    return hashtable;
  }

  std::vector<BoundaryVertex> Boundary::findEndpoints(const std::unordered_map<BoundaryVertex, std::array<BoundaryVertex,2> > & hashtable)
  {
    BoundaryVertex const invalid_vertex(-1, -1);
    std::vector<BoundaryVertex> endpoints;
    for(auto it : hashtable) {
      BoundaryVertex const neighbour1 = it.second[0];
      BoundaryVertex const neighbour2 = it.second[1];
      if (neighbour1==invalid_vertex || neighbour2==invalid_vertex)
        endpoints.push_back(it.first);
    }
    return endpoints;
  }

   
  Boundary Boundary::OrderContinuous(const std::unordered_map<BoundaryVertex,std::array<BoundaryVertex,2> > & hashtable,
                                     const CrackC & firstEdge
  )
  {
    std::vector<CrackC> bnd;
    BoundaryVertex present_vertex = firstEdge.vertexBegin();
    BoundaryVertex next_vertex(-1, -1);
    BoundaryVertex previous_vertex(-1, -1);
    BoundaryVertex const invalid_vertex(-1, -1);

    auto it = hashtable.find(present_vertex);
    if(it == hashtable.end()) {
      SPDLOG_ERROR("Boundary::OrderContinuous(), No entry in hashtable for vertex. ");
      RavlAssertMsg(0, "Boundary::OrderContinuous(), No entry in hashtable for vertex. ");
      return {};
    }
    BoundaryVertex neighbour1 = it->second[0];
    BoundaryVertex neighbour2 = it->second[1];

    if (firstEdge.vertexEnd()==neighbour1) next_vertex = neighbour1;
    else if (firstEdge.vertexEnd()==neighbour2) next_vertex = neighbour2;
    else if (neighbour1==invalid_vertex) next_vertex = neighbour2;
    else if (neighbour2==invalid_vertex) next_vertex = neighbour1;

    bnd.emplace_back(present_vertex, next_vertex);

    for(;;){
      present_vertex = bnd.back().vertexEnd();
      previous_vertex = bnd.back().vertexBegin();
      it = hashtable.find(present_vertex);
      if(it == hashtable.end()) {
        SPDLOG_ERROR("Boundary::OrderContinuous(), No entry in hashtable for vertex. ");
        RavlAssertMsg(0, "Boundary::OrderContinuous(), No entry in hashtable for vertex. ");
        return {};
      }
      neighbour1 = it->second[0];
      neighbour2 = it->second[1];

      if (previous_vertex == neighbour1) {
        next_vertex = neighbour2;
      } else {
        next_vertex = neighbour1;
      }

      if (next_vertex!=invalid_vertex) {
        bnd.emplace_back(present_vertex, next_vertex);
      }

      if (next_vertex==bnd.front().vertexBegin() || next_vertex==invalid_vertex) {
        break;
      }
      // boundary has been traced
    }

    return  Boundary(std::move(bnd));
  }


  //: Generate a set of ordered boundaries.

  std::vector<Boundary> Boundary::orderEdges() const
  {
    ONDEBUG(SPDLOG_INFO("std::vector<Boundary> Boundary::OrderEdges() const"));
    std::vector<Boundary> ret;

    std::unordered_map<BoundaryVertex,std::vector<CrackC> > leavers;
    leavers.reserve(mEdges.size());
    // Make table of all possible paths.
    for(auto it : mEdges) {
      ONDEBUG(SPDLOG_INFO("Begin={} End={}", it.vertexBegin(), it.vertexEnd()));
      leavers[it.vertexBegin()].push_back(it);
    }

    ONDEBUG(SPDLOG_INFO("leavers.size()={}", leavers.size()));

    // Make table of preferred paths.
    CrackC invalid(BoundaryVertex(0,0),CrackCodeT::CR_NODIR);

    std::unordered_map<CrackC,CrackC> edges;
    edges.reserve(mEdges.size() + (mEdges.size()>>2));
    for(auto it: mEdges) {
      std::vector<CrackC> &lst = leavers[it.vertexEnd()];
      size_t lstSize = lst.size();
      ONDEBUG(SPDLOG_INFO("End={} Size:{}", it.vertexEnd(),lstSize));
      switch(lstSize) {
        case 0: // Nothing leaving...
          edges[it] = invalid;
          break;
        case 1:
          edges[it] = lst.front();
          break;
        case 2:
        {
          // Need to choose the edge to follow
          RelativeCrackCodeT rc1 = it.code().relative(lst.front().code());
          RelativeCrackCodeT rc2 = it.code().relative(lst.back().code());
          if(rc1 > rc2) {
            edges[it] = lst.front();
          } else {
            edges[it] = lst.back();
          }
        } break;
        default:
          RavlAssertMsg(0,"Boundary::orderEdges(), Unexpected edge topology. ");
          break;
      }
    }
    leavers.clear(); // Done with these.

    // Separate boundaries or boundary segments.
    ONDEBUG(SPDLOG_INFO("edges.size()={}", edges.size()));

    std::unordered_map<CrackC,Boundary> startMap;
    startMap.reserve(edges.size());
    while(!edges.empty()) {
      auto it = edges.begin(); // Use iterator to pick an edge.
      std::vector<CrackC> bnds;
      CrackC at = it->first;
      CrackC first = at;
      for(;;) {
        auto atIsAt = edges.find(at);
        if(atIsAt == edges.end()) {
          break;
        }
        bnds.push_back(at);
        at = atIsAt->second;
        edges.erase(atIsAt);
      }
      if(at == first) { // If its a loop we're done.
        ONDEBUG(SPDLOG_INFO("Found closed boundary with {} edges ", bnds.size()));
        ret.push_back(Boundary(std::move(bnds)));
      } else {
        ONDEBUG(SPDLOG_INFO("Found open boundary. "));
        // Tie boundary segments together.
        // 'at' is the last edge from the segment.
        // 'first' is the first edge from the segment.
        auto atIsAt = startMap.find(at);
        if(atIsAt != startMap.end()) {
          ONDEBUG(SPDLOG_INFO("Joining boundary. "));
          //nbnds.DelFirst();
          const auto &edgeList = atIsAt->second.edges();
          bnds.insert(bnds.end(),edgeList.begin(),edgeList.end());
          first = bnds.front();
          startMap.erase(atIsAt);
        }
        startMap[first] = Boundary(std::move(bnds));
      }
    }

    // Clean up any remaining boundary segments.
    ONDEBUG(SPDLOG_INFO("StartMap.size()={}", startMap.size()));
    for(auto &smit : startMap)
      ret.push_back(smit.second);
    return ret;
  }

  [[maybe_unused]] Boundary line2Boundary(const BoundaryVertex &startVertex, const BoundaryVertex &endVertex)
  {
    std::vector<CrackC> boundary;
    BoundaryVertex vertex(startVertex);
    using RealT = float;
    auto startRow = RealT(startVertex[0]);
    auto startCol = RealT(startVertex[1]);
    RealT k = 0;
    RealT kk = 0;
    if(endVertex[0] == startVertex[0])
      k = 0;
    else if(endVertex[1] == startVertex[1])
      kk = 0;
    else if(std::abs(endVertex[0] - startVertex[0]) < std::abs(endVertex[1] - startVertex[1]))
      k = (RealT(endVertex[0] - startVertex[0])) / RealT(endVertex[1] - startVertex[1]);
    else
      kk = (RealT(endVertex[1] - startVertex[1])) / RealT(endVertex[0] - startVertex[0]);

    if(startVertex[1] < endVertex[1]) {  // 1 or 2 or 7 or 8 octant
      if(startVertex[0] > endVertex[0]) {// 1 or 2 octant
        if(-(endVertex[0] - startVertex[0]) < (endVertex[1] - startVertex[1])) {
          // 1. octant
          //        cout << "1. octant: " << k << '\n';
          while(vertex[1] < endVertex[1]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_RIGHT));
            vertex = right(vertex);
            if(std::abs(startRow + k * (RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_UP));
              vertex = up(vertex);
            }
          }
        } else {// 2. octant
          //        cout << "2. octant: " << kk << '\n';
          while(vertex[0] > endVertex[0]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_UP));
            vertex = up(vertex);
            if(std::abs(startCol + kk * (RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_RIGHT));
              vertex = right(vertex);
            }
          }
        }
      } else {// 7 or 8 octant
        if((endVertex[0] - startVertex[0]) < (endVertex[1] - startVertex[1])) {
          // 8. octant
          //        cout << "8. octant: " << k << '\n';
          while(vertex[1] < endVertex[1]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_RIGHT));
            vertex = right(vertex);
            if(std::abs(startRow + k * (RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_DOWN));
              vertex = down(vertex);
            }
          }
        } else {// 7. octant
          //        cout << "7. octant: " << kk << '\n';
          while(vertex[0] < endVertex[0]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_DOWN));
            vertex = down(vertex);
            if(std::abs(startCol + kk * (RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_RIGHT));
              vertex = right(vertex);
            }
          }
        }
      }
    } else {                             // 3 or 4 or 5 or 6 octant
      if(startVertex[0] > endVertex[0]) {// 3 or 4 octant
        if(-(endVertex[0] - startVertex[0]) < -(endVertex[1] - startVertex[1])) {
          // 4. octant
          //        cout << "4. octant: " << k << '\n';
          while(vertex[1] > endVertex[1]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_LEFT));
            vertex = left(vertex);
            if(std::abs(startRow + k * (RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_UP));
              vertex = up(vertex);
            }
          }
        } else {// 3. octant
          //        cout << "3. octant: " << kk << '\n';
          while(vertex[0] > endVertex[0]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_UP));
            vertex = up(vertex);
            if(std::abs(startCol + kk * (RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_LEFT));
              vertex = left(vertex);
            }
          }
        }
      } else {// 5 or 6 octant
        if((endVertex[0] - startVertex[0]) < -(endVertex[1] - startVertex[1])) {
          // 5. octant
          //        cout << "5. octant: " << k << '\n';
          while(vertex[1] > endVertex[1]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_LEFT));
            vertex = left(vertex);
            if(std::abs(startRow + k * (RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_DOWN));
              vertex = down(vertex);
            }
          }
        } else {// 6. octant
          //        cout << "6. octant: " << kk << '\n';
          while(vertex[0] < endVertex[0]) {
            boundary.push_back(CrackC(vertex, CrackCodeT::CR_DOWN));
            vertex = down(vertex);
            if(std::abs(startCol + kk * (RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5)) {
              boundary.push_back(CrackC(vertex, CrackCodeT::CR_LEFT));
              vertex = left(vertex);
            }
          }
        }
      }
    }
    //  cout << "Line2Boundary - size:" << boundary.size() << '\n';
    return Boundary(std::move(boundary));
  }

  //! Write out the boundary to a stream.
  std::ostream &operator<<(std::ostream &os, const Boundary &bnd)
  {
    os << "Boundary: ";
    for(auto edge : bnd.edges()) {
      os << edge << ",";
    }
    return os;
  }

}// namespace Ravl2
