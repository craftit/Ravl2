// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include "Ravl2/Index.hh"

namespace Ravl2
{

  //! @brief Iterate a rectangle in a Zig Zag pattern from the top left corner.
  //! Usefull when dealing with forwardDCT coefficients.

  class ZigZagIterC
  {
  public:
    //! Construct an empty iterator.
    ZigZagIterC() = default;

    //! Constructor
    //! Rectangle must be square.
    ZigZagIterC(const IndexRange<2> &nrect);

    //! Access image frame.
    IndexRange<2> &Frame()
    {
      return rect;
    }

    //! Access image frame.
    const IndexRange<2> &Frame() const
    {
      return rect;
    }

    //! Goto first index in sequence.
    bool First();

    //! Test if we're at a valid element.
    bool IsElm() const
    {
      return ok;
    }

    //! Test if we're at a valid element.
    [[nodiscard]] bool valid() const
    {
      return ok;
    }

    //! Get current pixel;
    [[nodiscard]] const Index<2> &Data() const
    {
      return at;
    }

    //! Get current pixel;
    [[nodiscard]] const Index<2> &operator*() const
    {
      return at;
    }

    //! Goto next pixel in scan pattern.
    bool Next();

    //! Goto next pixel in scan pattern.
    bool operator++()
    {
      return Next();
    }

    //! Test if we're at a valid element.
    operator bool() const
    {
      return ok;
    }

  protected:
    Index<2> at;
    IndexRange<2> rect;
    bool ok = false;
  };
}// namespace Ravl2
