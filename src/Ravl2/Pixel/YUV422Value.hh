// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

  //: YUV422 Pixel base class.
  // Pixels even image columns contain U and odd columns V

  template <class CompT>
  class YUV422ValueC : public Vector<CompT, 2>
  {
  public:
    YUV422ValueC()
    {}
    //: Default constructor.
    // Contents of pixels are left undefined.

    YUV422ValueC(const CompT &uv, const CompT &y)
    {
      this->data[0] = uv;
      this->data[1] = y;
    }
    //: Construct from components.

    inline const CompT &UV() const
    {
      return (*this)[0];
    }
    //: Access u or v component.

    inline CompT &UV()
    {
      return (*this)[0];
    }
    //: Access u or v component.

    inline const CompT &Y() const
    {
      return (*this)[1];
    }
    //: Access Y component.

    inline CompT &Y()
    {
      return (*this)[1];
    }
    //: Access Y component.
  };

  template <class CompT>
  inline std::istream &operator>>(std::istream &strm, YUV422ValueC<CompT> &val)
  {
    return strm >> ((Vector<CompT, 2> &)(val));
  }
  //: Stream input.

  template <class CompT>
  inline std::ostream &operator<<(std::ostream &strm, const YUV422ValueC<CompT> &val)
  {
    return strm << ((const Vector<CompT, 2> &)(val));
  }
  //: Stream output.

}// namespace Ravl2
