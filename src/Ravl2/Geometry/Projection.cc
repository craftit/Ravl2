// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="17/10/2002"
//! author="Charles Galambos"

#include "Ravl2/Geometry/Projection.hh"

namespace Ravl2
{
  static_assert(PointTransform<Projection<float, 2>>, "Projection<int,2> does not satisfy PointTransform");
  template class Projection<float, 2>;

}// namespace Ravl2
