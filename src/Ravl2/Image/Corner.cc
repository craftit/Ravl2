// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Image/Corner.hh"

namespace Ravl2
{
  
  std::ostream &operator<<(std::ostream &out,const CornerC &corn) {
    out << corn.Location() << ' ' << corn.Gradient() << ' ' << corn.Level();
    return out;
  }

//  std::istream &operator>>(std::istream &in,CornerC &corn) {
//    in >> corn.Location() >> corn.Gradient() >> corn.Level();
//    return in;
//  }

}
