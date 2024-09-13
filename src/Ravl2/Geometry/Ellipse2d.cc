// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2004, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Ellipse2d.hh"
//#include "Ravl2/Conic2d.hh"
//#include "Ravl2/SVD.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
  template class Ellipse2dC<float>;

}// namespace Ravl2
