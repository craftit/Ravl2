// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Robert Crida"
//! date="08/02/1999"

#pragma once

#include <list>
#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Polygon2d.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{
  //! Iterate across the interior of a polygon on an index grid
  //! This performs a scanline conversion of the polygon

  template<class RealT>
  class Polygon2dIterC {
    class EdgeC {
    public:
      EdgeC() = default;
      EdgeC(const Point<RealT,2> &p1, const Point<RealT,2> &p2);
      RealT xof(RealT row) const { return m_a * row + m_b; }
      [[nodiscard]] int MinRow() const { return m_minRow; }
      [[nodiscard]] int MaxRow() const { return m_maxRow; }
    private:
      int m_minRow = 0,m_maxRow =0; //end point with smallest y coord
      RealT m_a = 0,m_b = 0; //line is x = a*y + b;
    };

    class IELC: public std::list<EdgeC> {
    public:
      IELC() :m_minRow(std::numeric_limits<int>::max()), m_maxRow(std::numeric_limits<int>::min()) {}
      void Add(const EdgeC &e);
      bool Next(const int &row, EdgeC &e);
      [[nodiscard]] int MinRow() { return m_minRow; }
      [[nodiscard]] int MaxRow() { return m_maxRow; }
    private:
      int m_minRow = 0;
      int m_maxRow = 0;
    };

    class AELC: public std::list<EdgeC> {
    public:
      void Add(const EdgeC &e, int row);
      void DeleteEdges(const int &row);
      bool First(IndexRange<1> &indexRange, int row);
      bool Next(IndexRange<1> &indexRange, int row);
    private:
      std::list<RealT> m_sortedEdges;
    };
  public:
    //! Default constructor does not create a valid iterator!
    inline Polygon2dIterC() :m_valid(false) {}

    //! Construct from a polygon
    [[maybe_unused]] inline explicit Polygon2dIterC(const Polygon2dC<RealT> &polygon)
      : m_polygon(polygon)
    { First(); }

    //! Goto first scan line in the polygon.
    void First();

    //! At valid position ?
    //! \return true if we're at a valid scanline.
    [[nodiscard]] inline bool IsElm() const { return m_valid; }

    //! At a valid position ?
    //! \return true if we're at a valid scanline.
    operator bool() const
    { return IsElm(); }

    //! Get the current row for the scanline
    [[nodiscard]] inline const int &Row() const { return m_row; }

    //! Get range.
    //! Largest error from radius should be less than 0.5
    [[nodiscard]] inline const IndexRange<1> &RowIndexRange() const { return m_indexRange; }

    //! Goto next scan line.
    //! Returns true if we're now at a valid scanline.
    bool Next();

    //! Goto next point.
    // Returns true if we're now at a valid scanline.
    bool operator++(int)
    { return Next(); }

    //! Goto next point.
    // Returns true if we're now at a valid scanline.
    bool operator++()
    { return Next(); }
  private:
    Polygon2dC<RealT> m_polygon;
    int m_row = 0;
    IndexRange<1> m_indexRange;
    bool m_valid = false;
    IELC m_iel;
    AELC m_ael;
  };

}  

