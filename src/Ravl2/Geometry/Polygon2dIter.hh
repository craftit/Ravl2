// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Robert Crida"
//! date="08/02/1999"

#pragma once

#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Polygon2d.hh"

namespace Ravl2
{
  //! Iterate across the interior of a polygon on an index grid
  // NB. This performs a scanline conversion of the polygon

  template<class RealT>
  class Polygon2dIterC {
    class EdgeC {
    public:
      EdgeC() {}
      EdgeC(const Point<RealT,2> &p1, const Point<RealT,2> &p2);
      RealT xof(RealT row) const { return m_a * row + m_b; }
      int MinRow() const { return m_minRow; }
      int MaxRow() const { return m_maxRow; }
    private:
      int m_minRow,m_maxRow; //end point with smallest y coord
      RealT m_a,m_b; //line is x = a*y + b;
    };

    class IELC: public std::vector<EdgeC> {
    public:
      IELC() :m_minRow(std::numeric_limits<int>::max()), m_maxRow(std::numeric_limits<int>::min()) {}
      void Add(const EdgeC &e);
      bool Next(const int &row, EdgeC &e);
      int MinRow() { return m_minRow; }
      int MaxRow() { return m_maxRow; }
    private:
      int m_minRow;
      int m_maxRow;
    };

    class AELC: public std::vector<EdgeC> {
    public:
      void Add(const EdgeC &e, const int &row);
      void DeleteEdges(const int &row);
      bool First(IndexRange<1> &indexRange, const int &row);
      bool Next(IndexRange<1> &indexRange, const int &row);
    private:
      std::vector<RealT> m_sortedEdges;
    };
  public:
    inline Polygon2dIterC() :m_valid(false) {}
    //: Default constructor does not create a valid iterator!

    inline Polygon2dIterC(const Polygon2dC<RealT> &polygon)
      : m_polygon(polygon)
    { First(); }
    //: Constructor.
    
    void First();
    //: Goto first scan line in the polygon.
    
    inline bool IsElm() const { return m_valid; }
    //: At valid position ?

    operator bool() const
    { return IsElm(); }
    //: At a valid position ?

    inline const int &Row() const { return m_row; }
    //: Get the current row for the scanline
    
    inline const IndexRange<1> &IndexRange() const { return m_indexRange; }
    //: Get point.
    // Largest error from radius should be less than 0.5
    
    bool Next();
    //: Goto next scan line.
    // Returns true if we're now at a valid scanline.
    
    bool operator++(int)
    { return Next(); }
    //: Goto next point.
    // Returns true if we're now at a valid scanline.
  private:
    Polygon2dC<RealT> m_polygon;
    int m_row;
    IndexRange<1> m_indexRange;
    bool m_valid;
    IELC m_iel;
    AELC m_ael;

  };

}  

