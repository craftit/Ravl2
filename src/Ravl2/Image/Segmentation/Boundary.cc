// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26.04.1994"
//! rcsid="$Id$"
//! lib=RavlMath
//! file="Ravl/Math/Geometry/Euclidean/Boundary/Boundary.cc"

#include "Ravl2/Image/Segmentation/Boundary.hh"
//#include "Ravl/Hash.hh"
//#include "Ravl/RCHash.hh"
//#include "Ravl/HashIter.hh"
//#include "Ravl/Pair.hh"
//#include "Ravl/Polygon2d.hh"
//#include "Ravl/Array2dSqr3Iter.hh"
//#include "Ravl/Array2dSqr2Iter.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //: Create a boundary from the edges between 'inLabel' pixels an other values
  
  BoundaryC::BoundaryC(const Array2dC<int> &emask,int inLabel)
   : orientation(false)
  {
    if(emask.Frame().Rows() < 3 || emask.Frame().Cols() < 3) {
      cerr << "RegionMaskBodyC::Boundary(), Mask too small to compute boundary. \n";
      return;
    }
    for(Array2dSqr2IterC<int> it(emask);it;it++) {
      if(it.DataBR() == inLabel) {
        if(it.DataTR() != inLabel) 
          InsLast(CrackC(it.Index(),CR_LEFT));
        if(it.DataBL() != inLabel) 
          InsLast(CrackC(it.Index(),CR_DOWN));
      } else {
        if(it.DataTR() == inLabel) 
          InsLast(CrackC(it.Index()+Index<2>(-1,0),CR_RIGHT));
        if(it.DataBL() == inLabel) 
          InsLast(CrackC(it.Index()+Index<2>(0,-1),CR_UP));
      }
    }
  }

  //: Create a boundary from the edges between 'inLabel' pixels an other values
  
  BoundaryC::BoundaryC(const Array2dC<Uint> &emask,int inLabel)
   : orientation(false)
  {
    if(emask.Frame().Rows() < 3 || emask.Frame().Cols() < 3) {
      cerr << "RegionMaskBodyC::Boundary(), Mask too small to compute boundary. \n";
      return;
    }
    for(Array2dSqr3IterC<Uint> it(emask);it;it++) {
      if(it.DataMM() != (Uint)inLabel)
	continue;
      if(it.DataMR() != (Uint)inLabel)
	InsLast(CrackC(it.Index(),CR_UP));
      if(it.DataML() != (Uint)inLabel)
	InsLast(CrackC(it.Index(),CR_DOWN));
      if(it.DataTM() != (Uint)inLabel)
	InsLast(CrackC(it.Index(),CR_LEFT));
      if(it.DataBM() != (Uint)inLabel)
	InsLast(CrackC(it.Index(),CR_RIGHT));
    }
  }


  BoundaryC::BoundaryC(const IndexRange<2> & rect,bool asHole)
    : orientation(!asHole)
  {
    Index<2>   origin(rect.Origin());
    Index<2>   endP(rect.End());
    BVertexC   oVertex(origin);      // to help to GNU C++ 2.6.0
    CrackCodeC cr(CR_RIGHT);
    CrackC      edge(oVertex, cr);
    
    for(IndexC cUp=origin.Col(); cUp <= endP.Col(); cUp++) {
      InsLast(edge);
      edge.Step(NEIGH_RIGHT);
    }
    edge.TurnClock();
    for(IndexC rRight=origin.Row(); rRight <= endP.Row(); rRight++) {
      InsLast(edge);
      edge.Step(NEIGH_DOWN);
    }
    edge.TurnClock();
    for(IndexC cDown=endP.Col(); cDown >= origin.Col(); cDown--) {
      InsLast(edge);
      edge.Step(NEIGH_LEFT);
    }
    edge.TurnClock();
    for(IndexC rLeft=endP.Row(); rLeft >= origin.Row(); rLeft--) {
      InsLast(edge);
      edge.Step(NEIGH_UP);
    }
  }
  
  int BoundaryC::Area() const {
    int   col  = 0; // relative column of the boundary pixel 
    int area = 0; // region area 
    
    for(auto edge : *this ) {
      switch (edge.Data().Code())  {
      case CR_DOWN  : area -= col;  break;
      case CR_RIGHT : col++;        break;
      case CR_UP    : area += col;  break;
      case CR_LEFT  : col--;        break;
      case CR_NODIR :
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
    IndexC minR = firstV.Row();
    IndexC minC = firstV.Col();
    IndexC maxR = minR;
    IndexC maxC = minC;
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

    RCHashC<BVertexC, PairC<BVertexC> > hashtable = CreateHashtable();

    DListC<BVertexC> endpoints = FindEndpoints(hashtable);
    if (endpoints.IsEmpty()){
      BoundaryC bnd = OrderContinuous(hashtable, firstEdge, orient);
      bnds.InsLast(bnd);
    }
    else {
      DLIterC<BVertexC> ep_it(endpoints);
      for (ep_it.First(); ep_it.IsElm(); ep_it.Next()){
	BoundaryC bnd = OrderContinuous(hashtable, CrackC(ep_it.Data(), CrackCodeC(CR_NODIR)) , orient);
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
    CrackC invalid(BVertexC(0,0),CR_NODIR);    
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
  
  
  BoundaryC &BoundaryC::BReverse() {
    //  cout << "BoundaryC::BReverse()\n";
    Reverse();
    for(DLIterC<CrackC> it(*this);it;it++)
      it.Data().Reverse();
    orientation = !orientation;
    return *this;
  }
  
  RCHashC<BVertexC, PairC<BVertexC> > BoundaryC::CreateHashtable() const {
    RCHashC<BVertexC, PairC<BVertexC> > hashtable;
    for(DLIterC<CrackC> edge(*this);edge;edge++) {
      BVertexC bvertex1(edge->Begin());
      BVertexC bvertex2(edge->End());
      BVertexC invalid_vertex(-1, -1);

      if (!hashtable.IsElm(bvertex1)){
	hashtable.Insert(bvertex1, PairC<BVertexC>(bvertex2, invalid_vertex));
      }
      else {
	BVertexC neighbouring_vertex = hashtable[bvertex1].A();
	hashtable.Insert(bvertex1, PairC<BVertexC>(neighbouring_vertex, bvertex2));
      }

      if (!hashtable.IsElm(bvertex2)){
	hashtable.Insert(bvertex2, PairC<BVertexC>(bvertex1, invalid_vertex));
      }
      else {
	BVertexC neighbouring_vertex = hashtable[bvertex2].A();
	hashtable.Insert(bvertex2, PairC<BVertexC>(neighbouring_vertex, bvertex1));
      }
    }

    return hashtable;
  }

  BoundaryC BoundaryC::OrderContinuous(const RCHashC<BVertexC,PairC<BVertexC> > & hashtable, 
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
  
  DListC<BVertexC> BoundaryC::FindEndpoints(const RCHashC<BVertexC, PairC<BVertexC> > & hashtable) const {
    BVertexC invalid_vertex(-1, -1);
    HashIterC<BVertexC, PairC<BVertexC> > hash_iter(hashtable);
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
  
  
  BoundaryC Line2Boundary(const BVertexC & startVertex, const BVertexC & endVertex)
  {
    BoundaryC boundary;
    BVertexC  vertex(startVertex);
    RealT     startRow = startVertex.Row();
    RealT     startCol = startVertex.Col();
    RealT     k = 0;
    RealT     kk = 0;
    if (endVertex.Row() == startVertex.Row())
      k = 0;
    else if (endVertex.Col() == startVertex.Col()) 
      kk = 0;
    else if (Abs(endVertex.Row() - startVertex.Row()) < Abs(endVertex.Col() - startVertex.Col()))
      k = ((RealT)(endVertex.Row() - startVertex.Row())) / (endVertex.Col() - startVertex.Col());
    else
      kk = ((RealT)(endVertex.Col() - startVertex.Col())) / (endVertex.Row() - startVertex.Row());
    
    if (startVertex.Col() < endVertex.Col()) {  // 1 or 2 or 7 or 8 octant
      if (startVertex.Row() > endVertex.Row()) {  // 1 or 2 octant
	if ( -(endVertex.Row()-startVertex.Row()) < (endVertex.Col()-startVertex.Col()) ) { 
	  // 1. octant
	  //        cout << "1. octant: " << k << '\n';
	  while (vertex.Col() < endVertex.Col()) {
	    boundary.InsLast(CrackC(vertex,CR_RIGHT));
	    vertex.Step(NEIGH_RIGHT);
	    if ( Abs(startRow + k *(vertex.Col() - startCol) - vertex.Row()) > 0.5) {
	      boundary.InsLast(CrackC(vertex,CR_UP));
	      vertex.Step(NEIGH_UP);
	    }
	  }
	} else { // 2. octant
	  //        cout << "2. octant: " << kk << '\n';
	  while (vertex.Row() > endVertex.Row()) {
	    boundary.InsLast(CrackC(vertex,CR_UP));
	    vertex.Step(NEIGH_UP);
	    if ( Abs(startCol + kk *(vertex.Row() - startRow) - vertex.Col()) > 0.5 ) {
	      boundary.InsLast(CrackC(vertex,CR_RIGHT));
	      vertex.Step(NEIGH_RIGHT);
	    }
	  }
	}
      } else { // 7 or 8 octant
	if ( (endVertex.Row()-startVertex.Row()) < (endVertex.Col()-startVertex.Col()) ) { 
	  // 8. octant
	  //        cout << "8. octant: " << k << '\n';
	  while (vertex.Col() < endVertex.Col()) {
	    boundary.InsLast(CrackC(vertex,CR_RIGHT));
	    vertex.Step(NEIGH_RIGHT);
	    if (Abs(startRow + k *(vertex.Col() - startCol) - vertex.Row()) > 0.5) {
	      boundary.InsLast(CrackC(vertex,CR_DOWN));
	      vertex.Step(NEIGH_DOWN);
	    }
	  }
	} else { // 7. octant
	  //        cout << "7. octant: " << kk << '\n';
	  while (vertex.Row() < endVertex.Row()) {
	    boundary.InsLast(CrackC(vertex,CR_DOWN));
	    vertex.Step(NEIGH_DOWN);
	    if ( Abs(startCol + kk *(vertex.Row() - startRow) - vertex.Col()) > 0.5) {
	      boundary.InsLast(CrackC(vertex,CR_RIGHT));
	      vertex.Step(NEIGH_RIGHT);
	    }
	  }
	}
      }
    } else { // 3 or 4 or 5 or 6 octant
      if (startVertex.Row() > endVertex.Row()) { // 3 or 4 octant
	if ( -(endVertex.Row()-startVertex.Row())  < -(endVertex.Col()-startVertex.Col())) { 
	  // 4. octant
	  //        cout << "4. octant: " << k << '\n';
	  while (vertex.Col() > endVertex.Col()) {
	    boundary.InsLast(CrackC(vertex,CR_LEFT));
	    vertex.Step(NEIGH_LEFT);
	    if (Abs(startRow + k *(vertex.Col() - startCol) - vertex.Row()) > 0.5) {
	      boundary.InsLast(CrackC(vertex,CR_UP));
	      vertex.Step(NEIGH_UP);
	    }
	  }
	} else { // 3. octant
	  //        cout << "3. octant: " << kk << '\n';
	  while (vertex.Row() > endVertex.Row()) {
	    boundary.InsLast(CrackC(vertex,CR_UP));
	    vertex.Step(NEIGH_UP);
	    if ( Abs(startCol + kk *(vertex.Row() - startRow) - vertex.Col()) > 0.5) {
	      boundary.InsLast(CrackC(vertex,CR_LEFT));
	      vertex.Step(NEIGH_LEFT);
	    }
	  }
	}
      } else { // 5 or 6 octant
	if (  (endVertex.Row()-startVertex.Row()) < -(endVertex.Col()-startVertex.Col()) ) { 
	  // 5. octant
	  //        cout << "5. octant: " << k << '\n';
	  while (vertex.Col() > endVertex.Col()) {
	    boundary.InsLast(CrackC(vertex,CR_LEFT));
	    vertex.Step(NEIGH_LEFT);
	    if ( Abs(startRow + k *(vertex.Col() - startCol) - vertex.Row()) > 0.5) {
	      boundary.InsLast(CrackC(vertex,CR_DOWN));
	      vertex.Step(NEIGH_DOWN);
	    }
	  }
	} else { // 6. octant
	  //        cout << "6. octant: " << kk << '\n';
	  while (vertex.Row() < endVertex.Row()) {
	    boundary.InsLast(CrackC(vertex,CR_DOWN));
	    vertex.Step(NEIGH_DOWN);
	    if (Abs(startCol + kk *(vertex.Row() - startRow) - vertex.Col()) > 0.5) {
	      boundary.InsLast(CrackC(vertex,CR_LEFT));
	      vertex.Step(NEIGH_LEFT);
	    }
	  }
	}
      }
    }
    //  cout << "Line2Boundary - size:" << boundary.Size() << '\n';
    return boundary;
  }


}



