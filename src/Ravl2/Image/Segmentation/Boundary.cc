// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.04.1994"

#include "Ravl2/Image/Segmentation/Boundary.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  template<typename PixelT>
  BoundaryC::BoundaryC(const Array<PixelT, 2> &emask, PixelT inLabel)
    : orientation(false)
  {
    if(emask.Frame().Rows() < 3 || emask.Frame().Cols() < 3) {
      std::cerr << "RegionMaskBodyC::Boundary(), Mask too small to compute boundary. \n";
      return;
    }
    mEdges.reserve((emask.range(0).size() + emask.range(1).size()) * 2);
    for(Array2dSqr2IterC<PixelT> it(emask); it; it++) {
      if(it.DataBR() == inLabel) {
	if(it.DataTR() != inLabel)
	  mEdges.emplace_back(it.Index(), CrackCodeT::CR_LEFT);
	if(it.DataBL() != inLabel)
	  mEdges.emplace_back(it.Index(), CrackCodeT::CR_DOWN);
      } else {
	if(it.DataTR() == inLabel)
	  mEdges.emplace_back(it.Index() + Index<2>({-1, 0}), CrackCodeT::CR_RIGHT);
	if(it.DataBL() == inLabel)
	  mEdges.emplace_back(it.Index() + Index<2>({0, -1}), CrackCodeT::CR_UP);
      }
    }
  }

#if 0
  //: Create a boundary from the edges between 'inLabel' pixels an other values

  template<typename PixelT>
  BoundaryC::BoundaryC(const Array<PixelT,2> &emask,PixelT inLabel)
    : orientation(false)
  {
    if(emask.Frame().Rows() < 3 || emask.Frame().Cols() < 3) {
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


  BoundaryC::BoundaryC(const IndexRange<2> & rect,bool asHole)
    : orientation(!asHole)
  {
    Index<2>   origin(rect.Origin());
    Index<2>   endP(rect.End());
    BVertexC   oVertex(origin);      // to help to GNU C++ 2.6.0
    CrackCodeC cr(CrackCodeT::CR_RIGHT);
    CrackC      edge(oVertex, cr);

    for(int cUp=origin.Col(); cUp <= endP.Col(); cUp++) {
      InsLast(edge);
      edge.Step(NEIGH_RIGHT);
    }
    edge.TurnClock();
    for(int rRight=origin.Row(); rRight <= endP.Row(); rRight++) {
      InsLast(edge);
      edge.Step(NEIGH_DOWN);
    }
    edge.TurnClock();
    for(int cDown=endP.Col(); cDown >= origin.Col(); cDown--) {
      InsLast(edge);
      edge.Step(NEIGH_LEFT);
    }
    edge.TurnClock();
    for(int rLeft=endP.Row(); rLeft >= origin.Row(); rLeft--) {
      InsLast(edge);
      edge.Step(NEIGH_UP);
    }
  }

  int BoundaryC::Area() const {
    int   col  = 0; // relative column of the boundary pixel
    int area = 0; // region area

    for(auto edge : *this ) {
      switch (edge.Data().Code())  {
      case CrackCodeT::CR_DOWN  : area -= col;  break;
      case CrackCodeT::CR_RIGHT : col++;        break;
      case CrackCodeT::CR_UP    : area += col;  break;
      case CrackCodeT::CR_LEFT  : col--;        break;
      case CrackCodeT::CR_NODIR :
      default       :
        RavlAssertMsg(0,"BoundaryC::Area(), Illegal direction code. ");
        break;
      };
      //ONDEBUG(cerr << "BoundaryC::Area() Area=" << area << " col=" << col << "\n");
    }
    if(orientation)
      return -area;
    return area;
  }

  IndexRange<2> BoundaryC::BoundingBox() const {
    if (IsEmpty())
      return IndexRange<2>();
    BVertexC firstV(First());
    int minR = firstV.Row();
    int minC = firstV.Col();
    int maxR = minR;
    int maxC = minC;
    // False=Right.
    // True=Left.
    if(!orientation) {
      ONDEBUG(cerr << "BoundaryC::BoundingBox(), Object is on left. \n");
      for(DLIterC<CrackC> edge(*this);edge;edge++) {
	Index<2> vx = edge->LPixel();
	if (minR > vx.Row()) minR = vx.Row();
	if (maxR < vx.Row()) maxR = vx.Row();
	if (minC > vx.Col()) minC = vx.Col();
	if (maxC < vx.Col()) maxC = vx.Col();
      }
    } else {
      ONDEBUG(cerr << "BoundaryC::BoundingBox(), Object is on right. \n");
      for(DLIterC<CrackC> edge(*this);edge;edge++) {
	Index<2> vx = edge->RPixel();
	if (minR > vx.Row()) minR = vx.Row();
	if (maxR < vx.Row()) maxR = vx.Row();
	if (minC > vx.Col()) minC = vx.Col();
	if (maxC < vx.Col()) maxC = vx.Col();
      }
    }
    return IndexRange<2>(Index<2>(minR,minC), Index<2>(maxR,maxC));
  }



  DListC<BoundaryC> BoundaryC::Order(const CrackC & firstEdge, bool orient) {
    DListC<BoundaryC> bnds;

    RCHashC<BVertexC, std::array<BVertexC,2> > hashtable = CreateHashtable();

    DListC<BVertexC> endpoints = FindEndpoints(hashtable);
    if (endpoints.IsEmpty()){
      BoundaryC bnd = OrderContinuous(hashtable, firstEdge, orient);
      bnds.InsLast(bnd);
    }
    else {
      DLIterC<BVertexC> ep_it(endpoints);
      for (ep_it.First(); ep_it.IsElm(); ep_it.Next()){
	BoundaryC bnd = OrderContinuous(hashtable, CrackC(ep_it.Data(), CrackCodeC(CrackCodeT::CR_NODIR)) , orient);
	DLIterC<BVertexC> ep2_it(endpoints);
	for (ep2_it.First(); ep2_it.IsElm(); ep2_it.Next()){
	  if (ep2_it.Data()==bnd.Last().End()) ep2_it.Del();
	}
	bnds.InsLast(bnd);
      }
    }

    return bnds;
  }


  //: Generate a set of ordered boundries.

  DListC<BoundaryC> BoundaryC::OrderEdges() const {
    ONDEBUG(cerr << "DListC<BoundaryC> BoundaryC::OrderEdges() const \n");
    DListC<BoundaryC> ret;

    HashC<CrackC,CrackC> edges;
    HashC<BVertexC,DListC<CrackC> > leavers;

    // Make table of all possible paths.

    for(DLIterC<CrackC> it(*this);it;it++) {
      ONDEBUG(cerr << "Begin=" << it->Begin() << "\n");
      leavers[it->Begin()].InsLast(*it);
    }

    ONDEBUG(cerr << "leavers.Size()=" << leavers.Size() << ". \n");

    // Make table of prefered paths.
    CrackC invalid(BVertexC(0,0),CrackCodeT::CR_NODIR);
    for(DLIterC<CrackC> it(*this);it;it++) {
      ONDEBUG(cerr << "End=" << it->End() << "\n");
      DListC<CrackC> &lst = leavers[it->End()];
      Uint size = lst.Size();
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
    ONDEBUG(cerr << "edges.Size()=" << edges.Size() << ". \n");

    CrackC at,nxt;
    HashC<CrackC,BoundaryC> startMap;
    while(!edges.IsEmpty()) {
      HashIterC<CrackC,CrackC> it(edges); // Use iterator to pick an edge.
      BoundaryC bnds;
      at=it.Key();
      CrackC first = at;
      for(;;) {
	if(!edges.Lookup(at,nxt))
	  break;
	bnds.InsLast(at);
	edges.Del(at);
	at = nxt;
      }
      if(at == first) { // If its a loop we're done.
	ONDEBUG(cerr << "Found closed boundary. \n");
	ret.InsLast(bnds);
      } else {
	ONDEBUG(cerr << "Found open boundary. \n");
	// Tie boundry segements together.
	// 'at' is the last edge from the segment.
	// 'first' is the first edge from the segment.
	BoundaryC nbnds;
	if(startMap.Lookup(at,nbnds)) {
	  ONDEBUG(cerr << "Joinging boundary. \n");
	  //nbnds.DelFirst();
	  bnds.MoveLast(nbnds);
	  startMap.Del(at);
	  first = bnds.First();
	}
	startMap[first] = bnds;
      }
    }

    // Clean up any remaining boundry segments.
    ONDEBUG(cerr << "StartMap.Size()=" << startMap.Size() << "\n");
    for(HashIterC<CrackC,BoundaryC> smit(startMap);smit;smit++)
      ret.InsLast(smit.Data());
    return ret;
  }


  BoundaryC &BoundaryC::BReverse()
  {
    //  cout << "BoundaryC::BReverse()\n";

    Reverse();
    for(DLIterC<CrackC> it(*this);it;it++)
      it.Data().Reverse();
    orientation = !orientation;
    return *this;
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
    bnd.InsLast(CrackC(present_vertex, next_vertex));

    for(;;){
      present_vertex = bnd.Last().End();
      previous_vertex = bnd.Last().Begin();
      neighbour1 = hashtable[present_vertex].A();
      neighbour2 = hashtable[present_vertex].B();

      if (previous_vertex == neighbour1)
	next_vertex = neighbour2;
      else next_vertex = neighbour1;

      if (next_vertex!=invalid_vertex)
	bnd.InsLast(CrackC(present_vertex, next_vertex));

      if (next_vertex==bnd.First().Begin() || next_vertex==invalid_vertex) break;
      // boundary has been traced
    }

    return bnd;
  }

  DListC<BVertexC> BoundaryC::FindEndpoints(const RCHashC<BVertexC, std::array<BVertexC,2> > & hashtable) const {
    BVertexC invalid_vertex(-1, -1);
    HashIterC<BVertexC, std::array<BVertexC,2> > hash_iter(hashtable);
    DListC<BVertexC> endpoints;
    for(hash_iter.First(); hash_iter.IsElm(); hash_iter.Next()){
      BVertexC neighbour1 = hash_iter.Data().A();
      BVertexC neighbour2 = hash_iter.Data().B();
      if (neighbour1==invalid_vertex || neighbour2==invalid_vertex)
	endpoints.InsLast(hash_iter.Key());
    }
    return endpoints;
  }

  //: Convert a boundry to a polygon.
  // Note straigh edges are compressed into a single segment.

  Polygon2dC BoundaryC::Polygon2d(bool bHalfPixelOffset) const {
    Polygon2dC polygon;
    DLIterC<CrackC> et(*this);
    if(!et) return polygon;
    polygon.InsLast(Point2dC(*et));
    CrackCodeT lastCode = et->Code();
    if (bHalfPixelOffset) {
      Point2dC halfPixelOffset(-0.5,-0.5);
      for (et++; et; et++) {
        if (et->Code() == lastCode)
          continue;
        lastCode = et->Code();
        polygon.InsLast(Point2dC(*et) + halfPixelOffset);
      }
    }
    else {
      for (et++; et; et++) {
        if (et->Code() == lastCode)
          continue;
        lastCode = et->Code();
        polygon.InsLast(Point2dC(*et));
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
    RealT     startRow = startVertex[0];
    RealT     startCol = startVertex[1];
    RealT     k = 0;
    RealT     kk = 0;
    if (endVertex[0] == startVertex[0])
      k = 0;
    else if (endVertex[1] == startVertex[1])
      kk = 0;
    else if (std::abs(endVertex[0] - startVertex[0]) < std::abs(endVertex[1] - startVertex[1]))
      k = ((RealT)(endVertex[0] - startVertex[0])) / (endVertex[1] - startVertex[1]);
    else
      kk = ((RealT)(endVertex[1] - startVertex[1])) / (endVertex[0] - startVertex[0]);

    if (startVertex[1] < endVertex[1]) {  // 1 or 2 or 7 or 8 octant
      if (startVertex[0] > endVertex[0]) {  // 1 or 2 octant
	if ( -(endVertex[0]-startVertex[0]) < (endVertex[1]-startVertex[1]) ) {
	  // 1. octant
	  //        cout << "1. octant: " << k << '\n';
	  while (vertex[1] < endVertex[1]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_RIGHT));
	    vertex = right(vertex);
	    if ( std::abs(startRow + k *(vertex[1] - startCol) - vertex[0]) > 0.5) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	      vertex = up(vertex);
	    }
	  }
	} else { // 2. octant
	  //        cout << "2. octant: " << kk << '\n';
	  while (vertex[0] > endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	    vertex = up(vertex);
	    if ( std::abs(startCol + kk *(vertex[0] - startRow) - vertex[1]) > 0.5 ) {
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
	    if (std::abs(startRow + k *(vertex[1] - startCol) - vertex[0]) > 0.5) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	      vertex = down(vertex);
	    }
	  }
	} else { // 7. octant
	  //        cout << "7. octant: " << kk << '\n';
	  while (vertex[0] < endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	    vertex = down(vertex);
	    if ( std::abs(startCol + kk *(vertex[0] - startRow) - vertex[1]) > 0.5) {
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
	    if (std::abs(startRow + k *(vertex[1] - startCol) - vertex[0]) > 0.5) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	      vertex = up(vertex);
	    }
	  }
	} else { // 3. octant
	  //        cout << "3. octant: " << kk << '\n';
	  while (vertex[0] > endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_UP));
	    vertex = up(vertex);
	    if ( std::abs(startCol + kk *(vertex[0] - startRow) - vertex[1]) > 0.5) {
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
	    if ( std::abs(startRow + k *(vertex[1] - startCol) - vertex[0]) > 0.5) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	      vertex = down(vertex);
	    }
	  }
	} else { // 6. octant
	  //        cout << "6. octant: " << kk << '\n';
	  while (vertex[0] < endVertex[0]) {
	    boundary.push_back(CrackC(vertex,CrackCodeT::CR_DOWN));
	    vertex = down(vertex);
	    if (std::abs(startCol + kk *(vertex[0] - startRow) - vertex[1]) > 0.5) {
	      boundary.push_back(CrackC(vertex,CrackCodeT::CR_LEFT));
	      vertex = left(vertex);
	    }
	  }
	}
      }
    }
    //  cout << "Line2Boundary - size:" << boundary.Size() << '\n';
    return BoundaryC(std::move(boundary));
  }

}



