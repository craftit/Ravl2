// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Polygon2dIter.hh"
#include "Ravl2/Math.hh"

namespace Ravl2
{

  template<typename RealT>
  void Polygon2dIterC<RealT>::First() {
    m_iel = IELC();
    m_ael = AELC();
    if (m_polygon.size() <= 2) {
      m_valid = false;
      return;
    }
    /* build IEL */
    auto end = m_polygon.end();
    for (auto it = beginCircularFirst(m_polygon); it != end; ++it) {
      m_iel.Add(EdgeC(it.Data(), it.NextCrcData()));
    }

    m_row = m_iel.MinRow() - 1;
    Next();
  }

  template<typename RealT>
  bool Polygon2dIterC<RealT>::Next() 
  {
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

  template<typename RealT>
  Polygon2dIterC<RealT>::EdgeC::EdgeC(const Point<RealT,2> &p1, const Point<RealT,2> &p2) 
  {
    if(p2[0] == p1[0]) { //horizontal line
      m_a = RealT(0.0);  //to avoid dividing by 0
      m_b = RealT(0.0);
    } else {
      m_a = (p2[1]-p1[1])/(p2[0]-p1[0]);
      m_b = p1[1] - m_a * p1[0];
    }
    if (p1[0] < p2[0]) {
      m_minRow = int_ceil(p1[0]);
      m_maxRow = int_ceil(p2[0]);
    } else {
      m_minRow = int_ceil(p2[0]);
      m_maxRow = int_ceil(p1[0]);
    }
  }

  /* add edge at appropriate place in IEL */
  template<typename RealT>
  void Polygon2dIterC<RealT>::IELC::Add (const Polygon2dIterC<RealT>::EdgeC &e)
  {
    m_minRow = std::min(e.MinRow(),m_minRow);
    m_maxRow = std::max(e.MaxRow(),m_maxRow);
    //v is in decreasing order of ymin()
    auto it = this->begin();
    const auto end = this->end();
    for (; it != end && e.MinRow() < it->MinRow(); it++) {
    }
    this->insert(it, e);
  }

  /* next edge with miny() = y from IEL */
  template<typename RealT>
  bool Polygon2dIterC<RealT>::IELC::Next(const int &row, EdgeC &e)
  {
    if (this->empty()) return false;
    if (this->back().MinRow() == row) {
      e = this->back();
      this->pop_back();
      return true;
    }
    return false;
  }

  template<typename RealT>
  void Polygon2dIterC<RealT>::AELC::Add(const Polygon2dIterC<RealT>::EdgeC &e, int row)
  {
    //find right spot to add it at
    RealT x = e.xof(RealT(row)+RealT(0.5));
    auto it = this->begin();
    auto end = this->end();
    for (; it != end; it++) {
      if(it->xof(RealT(row)+RealT(0.5)) >= x)
        break;
    }
    this->insert(it,e);
  }
  
  /* delete edges with max y = y */
  template<typename RealT>
  void Polygon2dIterC<RealT>::AELC::DeleteEdges(const int &row)
  {
    const auto end = this->begin();
    auto it = this->end();
    if(it == end) return;
    --it;
    for (; it != end ;--it) {
      if (it->MaxRow() == row) {
        it = this->erase(it);
      }
    }
    // End is skipped in the loop above, so check it here
    if (it->MaxRow() == row) {
      it = this->erase(it);
    }
  }

  template<typename RealT>
  bool Polygon2dIterC<RealT>::AELC::First(IndexRange<1> &indexRange, const int row)
  {
    m_sortedEdges.clear();
    // use list insertion sort to put in ascending order
    // already pretty much sorted so work backwards for efficiency
    auto it = this->rbegin();
    const auto end = this->rend();
    for (; it != end; it++) {
      RealT edge = it->xof(RealT(row));
      auto rt = m_sortedEdges.begin();
      auto rt_end = m_sortedEdges.end();
      for (; rt != rt_end; rt++) {
        if (edge <= *rt) {
          m_sortedEdges.insert(rt,edge);
          break;
        }
      }
      if (rt == rt_end) // either the list is empty or edge is greater than everything in it
        m_sortedEdges.push_back(edge);
    }
    return Next(indexRange, row);
  }

  template<typename RealT>
  bool Polygon2dIterC<RealT>::AELC::Next(IndexRange<1> &indexRange, [[maybe_unused]] const int row) {
    do {
      if (m_sortedEdges.empty())
        return false;
      int newMin = int_ceil(m_sortedEdges.front());
      m_sortedEdges.pop_front();
      int newMax = int_ceil(m_sortedEdges.front() - RealT(1.0));
      indexRange = IndexRange<1>(newMin, newMax);
      m_sortedEdges.pop_front();
    } while(indexRange.size() <= 0);    // don't include empty sections
    return true;
  }


  template class Polygon2dIterC<float>;
  template class Polygon2dIterC<double>;
}
