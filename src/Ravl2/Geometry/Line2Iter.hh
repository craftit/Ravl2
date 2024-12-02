// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_LINE2DITER_HEADER
#define RAVL_LINE2DITER_HEADER 1
////////////////////////////////////////////////////////////////////
//! author="Charles Galambos"

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Line2PP.hh"

namespace Ravl2
{

  //! @brief Iterate through integer grid points along a 2d line.
  //! Uses a version of the midpoint algorithm to iterate all
  //! 8-connected grid points on a line between two positions.

  class Line2IterC
  {
  public:
    //: Construct from two points.
    Line2IterC(const Index<2> &start, const Index<2> &end);

    //! Construct from a line.
    template <class RealT>
    explicit Line2IterC(const Line2PP<RealT> &line)
        : Line2IterC(toIndex<2>(line.P1()), toIndex<2>(line.P2()))
    {}

    //! Start line again.
    void First(const Index<2> &start, const Index<2> &end);

    //! Still in line.
    [[nodiscard]] bool IsElm() const
    {
      return isElm;
    }

    //! At a pixel in line ?
    operator bool() const
    {
      return isElm;
    }

    //! At a pixel in line ?
    [[nodiscard]] bool valid() const
    {
      return isElm;
    }

    //! Goto next point on line.
    bool Next();

    //! Goto next point on line.
    void operator++(int)
    {
      Next();
    }

    //! Goto next point on line.
    void operator++()
    {
      Next();
    }

    //! Access position.
    Index<2> Data() const
    {
      return Index<2>(x, y);
    }

    //! Access position.
    Index<2> operator*() const
    {
      return Index<2>(x, y);
    }

  protected:
    int dx, dy;
    int d;
    int incrE, incrNE;
    int x, y;
    int xe, ye;
    int xc, yc;
    bool isElm;
  };
}// namespace Ravl2

#endif
