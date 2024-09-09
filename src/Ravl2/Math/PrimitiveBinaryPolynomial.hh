// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! file="Ravl/Math/Sequence/PrimitiveBinaryPolynomial.hh"
//! lib=RavlMath
//! author="Charles Galambos"
//! date="23/03/99"
//! docentry="Ravl.API.Math.Sequences"
//! example=exPrimiteBinaryPolynomial.cc

#pragma once

#include "Ravl2/Types.hh"

namespace Ravl2
{

  //! Evaluate a binary polynomial in a recurrence sequence.
  //! See Numerical Recipies in 'C' pg 312. (this is a full re-implementation) <p>
  //! These are used in generating Sobol sequences.

  template<size_t N>
  constexpr std::array<unsigned, N> primitiveBinaryPolynomial(size_t arg,size_t degree,const std::span<const unsigned> &init)
  {
    std::array<unsigned, N> seq;
    assert(init.size() >= degree);
    std::copy(init.begin(), init.begin() + int(degree), seq.begin());
    for(size_t i = degree; i < N; ++i) {
      auto res = seq[i - degree] ^ (seq[i - degree] << degree);
      auto ta = arg;
      for(auto q = degree - 1; q > 0; q--) {
	if(ta & 1)
	  res ^= seq[size_t(i-q)] << q;
	ta >>= 1;
      }
      seq[i] = res;
    }
    return seq;
  }

  extern template std::array<unsigned, 30> primitiveBinaryPolynomial<30>(size_t arg,size_t degree,const std::span<const unsigned> &init);
}

