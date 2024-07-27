// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.04.1994"

#include "Ravl2/Image/Segmentation/Boundary.hh"
#include "Ravl2/Assert.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //! Creates a boundary around the rectangle.
  BoundaryC toBoundary(IndexRange<2> rect, BoundaryTypeT type)
  {
    auto origin = rect.min();
    auto endP = rect.max();
    BVertexC   oVertex(origin);      // to help to GNU C++ 2.6.0
    CrackCodeC cr(CrackCodeT::CR_RIGHT);
    CrackC      edge(oVertex, cr);
    std::vector<CrackC> edges;

    for(int cUp=origin[1]; cUp <= endP[1]; cUp++) {
      edges.push_back(edge);
      edge.Right();
    }
    edge.TurnClock();
    for(int rRight=origin[0]; rRight <= endP[0]; rRight++) {
      edges.push_back(edge);
      edge.Down();
    }
    edge.TurnClock();
    for(int cDown=endP[1]; cDown >= origin[1]; cDown--) {
      edges.push_back(edge);
      edge.Left();
    }
    edge.TurnClock();
    for(int rLeft=endP[0]; rLeft >= origin[0]; rLeft--) {
      edges.push_back(edge);
      edge.Up();
    }
    return BoundaryC(std::move(edges), type == BoundaryTypeT::OUTSIDE);
  }

  int BoundaryC::area() const
  {
    int   col  = 0; // relative column of the boundary pixel
    int area = 0; // region area

    for(auto edge : mEdges ) {
      switch (edge.crackCode())  {
	case CrackCodeT::CR_DOWN  : area -= col;  break;
	case CrackCodeT::CR_RIGHT : col++;        break;
	case CrackCodeT::CR_UP    : area += col;  break;
	case CrackCodeT::CR_LEFT  : col--;        break;
	case CrackCodeT::CR_NODIR :
	default       :
	  RavlAssertMsg(0,"BoundaryC::area(), Illegal direction code. ");
	  break;
      };
      //ONDEBUG(std::cerr << "BoundaryC::area() Area=" << area << " col=" << col << "\n");
    }
    if(orientation)
      return -area;
    return area;
  }

  //! Reverse the order of the edges.
  [[nodiscard]] BoundaryC BoundaryC::reverse() const
  {
    std::vector<CrackC> newEdges;
    newEdges.reserve(mEdges.size());
    auto end = mEdges.rend();
    for(auto it = mEdges.rbegin(); it != end; ++it) {
      newEdges.push_back(it->reversed());
    }
    return BoundaryC(std::move(newEdges), !orientation);
  }

  BoundaryC &BoundaryC::BReverse()
  {
    *this = reverse();
    return *this;
  }

  IndexRange<2> BoundaryC::BoundingBox() const
  {
    IndexRange<2> bb;
    if (empty())
      return bb;
    bb = IndexRange<2>::mostEmpty();
    if(!orientation) {
      ONDEBUG(std::cerr << "BoundaryC::BoundingBox(), Object is on left. \n");
      for(auto edge : mEdges)
      {
	Index<2> vx = edge.LPixel();
	bb.involve(vx);
      }
    } else {
      ONDEBUG(std::cerr << "BoundaryC::BoundingBox(), Object is on right. \n");
      for(auto edge : mEdges)
      {
	Index<2> vx = edge.RPixel();
	bb.involve(vx);
      }
    }
    return bb;
  }


#if 0
  //: Create a boundary from the edges between 'inLabel' pixels an other values

  template<typename PixelT>
  BoundaryC::BoundaryC(const Array<PixelT,2> &emask,PixelT inLabel)
    : orientation(false)
  {
    if(emask.range().Rows() < 3 || emask.range().Cols() < 3) {
      std::cerr << "RegionMaskBodyC::Boundary(), Mask too small to compute boundary. \n";
      return;
    }
    for(Array2dSqr3IterC<PixelT> it(emask);it;it++) {
      if(it.DataMM() != inLabel)
	continue;
      if(it.DataMR() != inLabel)
	InsLast(CrackC(it.Index(),CrackCodeT::CR_UP));
      if(it.DataML() != inLabel)
	InsLast(CrackC(it.Index(),CrackCodeT::CR_DOWN));
      if(it.DataTM() != inLabel)
	InsLast(CrackC(it.Index(),CrackCodeT::CR_LEFT));
      if(it.DataBM() != inLabel)
	InsLast(CrackC(it.Index(),CrackCodeT::CR_RIGHT));
    }
  }

  std::vector<BoundaryC> BoundaryC::Order(const CrackC & firstEdge, bool orient) {
    std::vector<BoundaryC> bnds;

    RCHashC<BVertexC, std::array<BVertexC,2> > hashtable = CreateHashtable();

    std::vector<BVertexC> endpoints = FindEndpoints(hashtable);
    if (endpoints.empty()){
      BoundaryC bnd = OrderContinuous(hashtable, firstEdge, orient);
      bnds.push_back(bnd);
    }
    else {
      DLIterC<BVertexC> ep_it(endpoints);
      for (ep_it.First(); ep_it.valid(); ep_it.next()){
	BoundaryC bnd = OrderContinuous(hashtable, CrackC(ep_it.Data(), CrackCodeC(CrackCodeT::CR_NODIR)) , orient);
	DLIterC<BVertexC> ep2_it(endpoints);
	for (ep2_it.First(); ep2_it.valid(); ep2_it.next()){
	  if (ep2_it.Data()==bnd.Last().End()) ep2_it.Del();
	}
	bnds.push_back(bnd);
      }
    }

    return bnds;
  }


  //: Generate a set of ordered boundries.

  std::vector<BoundaryC> BoundaryC::OrderEdges() const {
    ONDEBUG(std::cerr << "std::vector<BoundaryC> BoundaryC::OrderEdges() const \n");
    std::vector<BoundaryC> ret;

    HashC<CrackC,CrackC> edges;
    HashC<BVertexC,std::vector<CrackC> > leavers;

    // Make table of all possible paths.

    for(DLIterC<CrackC> it(*this);it;it++) {
      ONDEBUG(std::cerr << "Begin=" << it->Begin() << "\n");
      leavers[it->Begin()].push_back(*it);
    }

    ONDEBUG(std::cerr << "leavers.size()=" << leavers.size() << ". \n");

    // Make table of prefered paths.
    CrackC invalid(BVertexC(0,0),CrackCodeT::CR_NODIR);
    for(DLIterC<CrackC> it(*this);it;it++) {
      ONDEBUG(std::cerr << "End=" << it->End() << "\n");
      std::vector<CrackC> &lst = leavers[it->End()];
      Uint size = lst.size();
      switch(size) {
      case 0: // Nothing leaving...
	edges[*it] = invalid;
	break;
      case 1:
	edges[*it] = lst.First();
	break;
      case 2:
	{
	// Need to choose the edge to follow
	  RelativeCrackCodeT rc1 = it->Relative(lst.First());
	  RelativeCrackCodeT rc2 = it->Relative(lst.Last());
	  if(rc1 > rc2)
	    edges[*it] = lst.First();
	  else
	    edges[*it] = lst.Last();
	} break;
      default:
	RavlAssertMsg(0,"BoundaryC::OrderEdges(), Unexpected edge topology. ");
	break;
      }
    }
    leavers.Empty(); // Done with these.

    // Seperate boundries or boundry segements.
    ONDEBUG(std::cerr << "edges.size()=" << edges.size() << ". \n");

    CrackC at,nxt;
    HashC<CrackC,BoundaryC> startMap;
    while(!edges.empty()) {
      HashIterC<CrackC,CrackC> it(edges); // Use iterator to pick an edge.
      BoundaryC bnds;
      at=it.Key();
      CrackC first = at;
      for(;;) {
	if(!edges.Lookup(at,nxt))
	  break;
	bnds.push_back(at);
	edges.Del(at);
	at = nxt;
      }
      if(at == first) { // If its a loop we're done.
	ONDEBUG(std::cerr << "Found closed boundary. \n");
	ret.push_back(bnds);
      } else {
	ONDEBUG(std::cerr << "Found open boundary. \n");
	// Tie boundry segements together.
	// 'at' is the last edge from the segment.
	// 'first' is the first edge from the segment.
	BoundaryC nbnds;
	if(startMap.Lookup(at,nbnds)) {
	  ONDEBUG(std::cerr << "Joinging boundary. \n");
	  //nbnds.DelFirst();
	  bnds.MoveLast(nbnds);
	  startMap.Del(at);
	  first = bnds.First();
	}
	startMap[first] = bnds;
      }
    }

    // Clean up any remaining boundry segments.
    ONDEBUG(std::cerr << "StartMap.size()=" << startMap.size() << "\n");
    for(HashIterC<CrackC,BoundaryC> smit(startMap);smit;smit++)
      ret.push_back(smit.Data());
    return ret;
  }

  RCHashC<BVertexC, std::array<BVertexC,2> > BoundaryC::CreateHashtable() const {
    RCHashC<BVertexC, std::array<BVertexC,2> > hashtable;
    for(DLIterC<CrackC> edge(*this);edge;edge++) {
      BVertexC bvertex1(edge->Begin());
      BVertexC bvertex2(edge->End());
      BVertexC invalid_vertex(-1, -1);

      if (!hashtable.IsElm(bvertex1)){
	hashtable.Insert(bvertex1, std::array<BVertexC,2>(bvertex2, invalid_vertex));
      }
      else {
	BVertexC neighbouring_vertex = hashtable[bvertex1].A();
	hashtable.Insert(bvertex1, std::array<BVertexC,2>(neighbouring_vertex, bvertex2));
      }

      if (!hashtable.IsElm(bvertex2)){
	hashtable.Insert(bvertex2, std::array<BVertexC,2>(bvertex1, invalid_vertex));
      }
      else {
	BVertexC neighbouring_vertex = hashtable[bvertex2].A();
	hashtable.Insert(bvertex2, std::array<BVertexC,2>(neighbouring_vertex, bvertex1));
      }
    }

    return hashtable;
  }

  BoundaryC BoundaryC::OrderContinuous(const RCHashC<BVertexC,std::array<BVertexC,2> > & hashtable,
				       const CrackC & firstEdge,
				       bool orient
				       ) const
  {
    BoundaryC bnd(orient);
    BVertexC present_vertex = firstEdge.Begin();
    BVertexC next_vertex(-1, -1);
    BVertexC previous_vertex(-1, -1);
    BVertexC invalid_vertex(-1, -1);

    BVertexC neighbour1 = hashtable[present_vertex].A();
    BVertexC neighbour2 = hashtable[present_vertex].B();
    if (firstEdge.End()==neighbour1) next_vertex = neighbour1;
    else if (firstEdge.End()==neighbour2) next_vertex = neighbour2;
    else if (neighbour1==invalid_vertex) next_vertex = neighbour2;
    else if (neighbour2==invalid_vertex) next_vertex = neighbour1;
    bnd.push_back(CrackC(present_vertex, next_vertex));

    for(;;){
      present_vertex = bnd.Last().End();
      previous_vertex = bnd.Last().Begin();
      neighbour1 = hashtable[present_vertex].A();
      neighbour2 = hashtable[present_vertex].B();

      if (previous_vertex == neighbour1)
	next_vertex = neighbour2;
      else next_vertex = neighbour1;

      if (next_vertex!=invalid_vertex)
	bnd.push_back(CrackC(present_vertex, next_vertex));

      if (next_vertex==bnd.First().Begin() || next_vertex==invalid_vertex) break;
      // boundary has been traced
    }

    return bnd;
  }

  std::vector<BVertexC> BoundaryC::FindEndpoints(const RCHashC<BVertexC, std::array<BVertexC,2> > & hashtable) const {
    BVertexC invalid_vertex(-1, -1);
    HashIterC<BVertexC, std::array<BVertexC,2> > hash_iter(hashtable);
    std::vector<BVertexC> endpoints;
    for(hash_iter.First(); hash_iter.valid(); hash_iter.next()){
      BVertexC neighbour1 = hash_iter.Data().A();
      BVertexC neighbour2 = hash_iter.Data().B();
      if (neighbour1==invalid_vertex || neighbour2==invalid_vertex)
	endpoints.push_back(hash_iter.Key());
    }
    return endpoints;
  }

  //: Convert a boundry to a polygon.
  // Note straigh edges are compressed into a single segment.

  Polygon2dC BoundaryC::Polygon2d(bool bHalfPixelOffset) const {
    Polygon2dC polygon;
    DLIterC<CrackC> et(*this);
    if(!et) return polygon;
    polygon.push_back(Point<RealT,2>(*et));
    CrackCodeT lastCode = et->Code();
    if (bHalfPixelOffset) {
      Point<RealT,2> halfPixelOffset(-0.5,-0.5);
      for (et++; et; et++) {
        if (et->Code() == lastCode)
          continue;
        lastCode = et->Code();
        polygon.push_back(Point<RealT,2>(*et) + halfPixelOffset);
      }
    }
    else {
      for (et++; et; et++) {
        if (et->Code() == lastCode)
          continue;
        lastCode = et->Code();
        polygon.push_back(Point<RealT,2>(*et));
      }
    }
    return polygon;
  }

#endif

  BoundaryC Line2Boundary(const BVertexC & startVertex, const BVertexC & endVertex)
  {
    std::vector<CrackC> boundary;
    BVertexC  vertex(startVertex);
    using RealT = float;
    auto     startRow = RealT(startVertex[0]);
    auto     startCol = RealT(startVertex[1]);
    RealT     k = 0;
    RealT     kk = 0;
    if (endVertex[0] == startVertex[0])
      k = 0;
    else if (endVertex[1] == startVertex[1])
      kk = 0;
    else if (std::abs(endVertex[0] - startVertex[0]) < std::abs(endVertex[1] - startVertex[1]))
      k = (RealT(endVertex[0] - startVertex[0])) / RealT(endVertex[1] - startVertex[1]);
    else
      kk = (RealT(endVertex[1] - startVertex[1])) / RealT(endVertex[0] - startVertex[0]);

    if (startVertex[1] < endVertex[1]) {  // 1 or 2 or 7 or 8 octant
      if (startVertex[0] > endVertex[0]) {  // 1 or 2 octant
	if ( -(endVertex[0]-startVertex[0]) < (endVertex[1]-startVertex[1]) ) {
	  // 1. octant
	  //        cout << "1. octant: " << k << '\n';
	  while (vertex[1] < endVertex[1]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_RIGHT));
	    vertex = right(vertex);
	    if ( std::abs(startRow + k * (RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	      vertex = up(vertex);
	    }
	  }
	} else { // 2. octant
	  //        cout << "2. octant: " << kk << '\n';
	  while (vertex[0] > endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	    vertex = up(vertex);
	    if ( std::abs(startCol + kk *(RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5) ) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_RIGHT));
	      vertex = right(vertex);
	    }
	  }
	}
      } else { // 7 or 8 octant
	if ( (endVertex[0]-startVertex[0]) < (endVertex[1]-startVertex[1]) ) {
	  // 8. octant
	  //        cout << "8. octant: " << k << '\n';
	  while (vertex[1] < endVertex[1]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_RIGHT));
	    vertex = right(vertex);
	    if (std::abs(startRow + k *(RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	      vertex = down(vertex);
	    }
	  }
	} else { // 7. octant
	  //        cout << "7. octant: " << kk << '\n';
	  while (vertex[0] < endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	    vertex = down(vertex);
	    if ( std::abs(startCol + kk *(RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5)) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_RIGHT));
	      vertex = right(vertex);
	    }
	  }
	}
      }
    } else { // 3 or 4 or 5 or 6 octant
      if (startVertex[0] > endVertex[0]) { // 3 or 4 octant
	if ( -(endVertex[0]-startVertex[0])  < -(endVertex[1]-startVertex[1])) {
	  // 4. octant
	  //        cout << "4. octant: " << k << '\n';
	  while (vertex[1] > endVertex[1]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_LEFT));
	    vertex = left(vertex);
	    if (std::abs(startRow + k *(RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	      vertex = up(vertex);
	    }
	  }
	} else { // 3. octant
	  //        cout << "3. octant: " << kk << '\n';
	  while (vertex[0] > endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	    vertex = up(vertex);
	    if ( std::abs(startCol + kk *(RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5)) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_LEFT));
	      vertex = left(vertex);
	    }
	  }
	}
      } else { // 5 or 6 octant
	if (  (endVertex[0]-startVertex[0]) < -(endVertex[1]-startVertex[1]) ) {
	  // 5. octant
	  //        cout << "5. octant: " << k << '\n';
	  while (vertex[1] > endVertex[1]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_LEFT));
	    vertex = left(vertex);
	    if ( std::abs(startRow + k *(RealT(vertex[1]) - startCol) - RealT(vertex[0])) > RealT(0.5)) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	      vertex = down(vertex);
	    }
	  }
	} else { // 6. octant
	  //        cout << "6. octant: " << kk << '\n';
	  while (vertex[0] < endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	    vertex = down(vertex);
	    if (std::abs(startCol + kk *(RealT(vertex[0]) - startRow) - RealT(vertex[1])) > RealT(0.5)) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_LEFT));
	      vertex = left(vertex);
	    }
	  }
	}
      }
    }
    //  cout << "Line2Boundary - size:" << boundary.size() << '\n';
    return BoundaryC(std::move(boundary));
  }

}



