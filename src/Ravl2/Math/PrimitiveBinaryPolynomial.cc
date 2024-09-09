// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! lib=RavlMath
//! file="Ravl/Math/Sequence/PrimitiveBinaryPolynomial.cc"

#include "Ravl2/Math/PrimitiveBinaryPolynomial.hh"

namespace Ravl2
{
  template std::array<unsigned, 30> primitiveBinaryPolynomial<30>(size_t arg,size_t degree,const std::span<const unsigned> &init);
}
