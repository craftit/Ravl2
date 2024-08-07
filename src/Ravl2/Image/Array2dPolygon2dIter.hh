// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Robert Crida"
//! date="08/02/1999"
#pragma once

#include "Ravl2/Geometry/Polygon2dIter.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{
  //: Iterate across the interior of a polygon on an index grid within an array

  template <class DataT>
  class Array2dPolygon2dIterC
  {
  public:
    Array2dPolygon2dIterC(const Array<DataT, 2> &array, const Polygon2dC<float> &polygon);
    //: Constructor.

    void First();
    //: Goto point of the iterator.

    bool IsElm() const
    {
      return m_arrayIter.valid();
    }
    //: At valid position ?

    operator bool() const
    {
      return IsElm();
    }
    //: At a valid position ?

    DataT &Data()
    {
      return m_arrayIter.Data();
    }
    //: Access data.

    const DataT &Data() const
    {
      return m_arrayIter.Data();
    }
    //: Constant access to data.

    DataT &operator*()
    {
      return Data();
    }
    //: Access data.

    const DataT &operator*() const
    {
      return Data();
    }
    //: Constant access to data.

    DataT *operator->()
    {
      return &Data();
    }
    //: Access member function of data..

    const DataT *operator->() const
    {
      return &Data();
    }
    //: Constant access to member function of data..

    Index<2> Index()
    {
      return Index<2>(m_polygonIter[0], m_arrayIter.Index());
    }
    //: Access to the current index in the array

    bool Next();
    //: Goto next scan line.
    // Returns true if we're now at a valid scanline.

    bool operator++(int)
    {
      return Next();
    }
    //: Goto next point.
    // Returns true if we're now at a valid scanline.
  private:
    Array<DataT, 2> m_array;
    Polygon2dIterC m_polygonIter;
    ArrayIter<DataT, 1> m_arrayIter;
    IndexRange<1> m_rowRange;
    IndexRange<1> m_colRange;
  };

  template <class DataT>
  Array2dPolygon2dIterC<DataT>::Array2dPolygon2dIterC(const Array<DataT, 2> &array, const Polygon2dC &polygon)
      : m_array(array),
        m_rowRange(array.range(0)),
        m_colRange(array.range(1))
  {
    m_polygonIter = Polygon2dIterC(polygon);
    First();
  }

  template <class DataT>
  void Array2dPolygon2dIterC<DataT>::First()
  {
    m_polygonIter.first();
    Next();
  }

  template <class DataT>
  bool Array2dPolygon2dIterC<DataT>::Next()
  {
    if(m_arrayIter.valid()) {
      m_arrayIter.next();
      if(m_arrayIter.valid())
        return true;
    }
    for(; m_polygonIter; m_polygonIter++) {
      int row = m_polygonIter[0];
      IndexRange<1> indexRange = m_polygonIter.IndexRange();
      if(m_rowRange.contains(row) && m_colRange.overlaps(indexRange)) {
        m_arrayIter = Array1dIterC<DataT>(m_array.SliceRow(row), m_colRange.clip(indexRange));
        m_polygonIter++;
        return true;
      }
    }
    m_arrayIter = Array1dIterC<DataT>();
    return false;
  }

}// namespace Ravl2
