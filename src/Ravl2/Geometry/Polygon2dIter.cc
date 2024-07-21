// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Polygon2dIter.hh"


namespace RavlN {
  
  void Polygon2dIterC::First() {
    m_iel = IELC();
    m_ael = AELC();
    if (m_polygon.Size() <= 2) {
      m_valid = false;
      return;
    }
    /* build IEL */
    for (DLIterC<Point2dC> it(m_polygon); it; it++) {
      m_iel.Add(EdgeC(it.Data(), it.NextCrcData()));
    }

    m_row = m_iel.MinRow() - 1;
    Next();
  }
    
  bool Polygon2dIterC::Next() {
    if ((m_valid = m_ael.Next(m_indexRange, m_row)))
      return true;
    for (m_row++; m_row <= m_iel.MaxRow(); m_row++) {
      EdgeC e;
      while (m_iel.Next(m_row, e))
        m_ael.Add(e,m_row);
      m_ael.DeleteEdges(m_row);
      if ((m_valid = m_ael.First(m_indexRange, m_row))) {
        return true;
      }
    }
    return false;
  }

  Polygon2dIterC::EdgeC::EdgeC(const Point2dC &p1, const Point2dC &p2) {
    if(p2.Row() == p1.Row()) { //horizontal line
      m_a = 0.0f;  //to avoid dividing by 0
      m_b = 0.0f;
    } else {
      m_a = (p2.Col()-p1.Col())/(p2.Row()-p1.Row());
      m_b = p1.Col() - m_a * p1.Row();
    }
    if (p1.Row() < p2.Row()) {
      m_minRow = Ceil(p1.Row());
      m_maxRow = Ceil(p2.Row());
    } else {
      m_minRow = Ceil(p2.Row());
      m_maxRow = Ceil(p1.Row());
    }
  }

  /* add edge at appropriate place in IEL */
  void Polygon2dIterC::IELC::Add (const Polygon2dIterC::EdgeC &e) {
    m_minRow = Min(e.MinRow(),m_minRow);
    m_maxRow = Max(e.MaxRow(),m_maxRow);
    //v is in decreasing order of ymin()
    DLIterC<EdgeC> it(*this);
    for (; it && e.MinRow() < it->MinRow(); it++) {
    }
    it.InsertBef(e);
  }

  /* next edge with miny() = y from IEL */
  bool Polygon2dIterC::IELC::Next(const IndexC &row, EdgeC &e) {
    if (Size()==0) return false;
    if (Last().MinRow() == row) {
      e = PopLast();
      return true;
    }
    return false;
  }

  void Polygon2dIterC::AELC::Add(const Polygon2dIterC::EdgeC &e, const IndexC &row) {
    //find right spot to add it at
    RealT x = e.xof(row.V()+0.5);

    DLIterC<EdgeC> it(*this);
    for (; it && it->xof(row.V()+0.5) < x; it++) {
    }
    it.InsertBef(e);
  }
  
  /* delete edges with max y = y */
  void Polygon2dIterC::AELC::DeleteEdges(const IndexC &row) {
    DLIterC<EdgeC> it(*this);
    for (it.Last(); it; it--) {
      if (it->MaxRow() == row)
        it.DelMoveNext();
    }
  }

  bool Polygon2dIterC::AELC::First(IndexRangeC &indexRange, const IndexC &row) {
    m_sortedEdges.Empty();
    // use list insertion sort to put in ascending order
    // already pretty much sorted so work backwards for efficiency
    DLIterC<EdgeC> it(*this);
    for (it.Last(); it; it--) {
      RealT edge = it->xof(row);
      DLIterC<RealT> rt(m_sortedEdges);
      for (; rt; rt++) {
        if (edge <= *rt) {
          rt.InsertBef(edge);
          break;
        }
      }
      if (!rt) // either the list is empty or edge is greater than everything in it
        m_sortedEdges.InsLast(edge);
    }
    return Next(indexRange, row);
  }

  bool Polygon2dIterC::AELC::Next(IndexRangeC &indexRange, const IndexC &row) {
    do {
      if (m_sortedEdges.Size() == 0)
        return false;
      indexRange.Min() = Ceil(m_sortedEdges.PopFirst());
      indexRange.Max() = Ceil(m_sortedEdges.PopFirst() - 1.0);
    } while(indexRange.Size() <= 0);    // don't include empty sections
    return true;
  }

}
