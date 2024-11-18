// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2006, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/PolyLine.hh"
#include "Ravl2/Geometry/PolyApprox.hh"

namespace Ravl2
{
  //! Let the compiler know that we will use these classes with the following types
  template class PolyLine<float,2>;
  template class PolyLine<float,3>;

}
