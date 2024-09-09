// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! lib=RavlMath
//! author="Charles Galambos"
//! docentry="Ravl.API.Math.Sequences"
//! date="15/03/1999"
#pragma once

#include <vector>
#include "Ravl2/Types.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Sentinel.hh"
#include "Ravl2/Math/PrimitiveBinaryPolynomial.hh"

namespace Ravl2
{
  //! Generate a Sobol sequence.
  //! Re-implementation from numerical recipies 2nd edition. pg 312

  template<typename RealT, size_t N, size_t bits = (sizeof(RealT) * 8 - 2)>
   requires std::is_floating_point<RealT>::value && (bits > 0) && (N <= 6)
  class SobolSequence
  {
  public:
    //! Generate a sequence with n dimensions.
    constexpr inline SobolSequence() { init(); }

    //! Goto first point in sequence.
    constexpr bool reset()
    {
      work = xt::zeros<unsigned>({N});
      done = false;
      s = 0;
      next();
      return true;
    }

    //! Are the numbers left in the sequence ?
    [[nodiscard]] constexpr bool valid() const { return !done; }

    //! Goto next point in sequence.
    //! Return: false if out of numbers.
    constexpr bool next()
    {
      if(done)
	return false;
      size_t b = 0;
      size_t k = s++;
      // Find the least unset bit from sequence position.
      for(; b <= bits; b++) {
	if(!(k & 1))
	  break;
	k >>= 1;
      }
      //cerr << "b:" << b << " " << s << "\n";
      if(b == bits) {
	done = true;
	return false;
      }
      for(size_t i = 0; i < N; i++) {
	work[i] ^= vx(b,i);
	result[i] = RealT(work[i]) * frac;
      }
      return true;
    }

    //: Goto next value in the sequence.
    constexpr void operator++() { next(); }

    //: Get point. each element will have a value between
    // 0 and one.
    [[nodiscard]] constexpr const auto &data() const { return result; }

    //! Access point.
    [[nodiscard]] constexpr const auto &operator*() const { return data(); }

  private:

    static constexpr unsigned initseq[6][4] =
      {{1, 3, 5, 15},
       {1, 1, 7, 11},
       {1, 3, 7, 5},
       {1, 3, 3, 15},
       {1, 1, 3, 13},
       {1, 1, 5, 9}};

    static constexpr size_t initdeg[6] = {1, 2, 3, 3, 4, 4};
    static constexpr size_t initpoly[6] = {0, 1, 1, 2, 1, 4};

    constexpr void init()
    {
      for(size_t k = 0; k < N; k++) { ;
	auto seq =
	  primitiveBinaryPolynomial<bits>(initpoly[k], initdeg[k], std::span<const unsigned>(initseq[k], initdeg[k]));

	// Work out direction numbers.
	int shift = bits - 1;
	for(size_t i = 0; i < bits; i++) {
	  vx(i, k) = seq[i] << (shift--);
	}
      }
      next();
    }

    size_t s = 0;        // Position in sequence.
    bool done = false;
    const RealT frac = 1.0 / (1 << bits);
    Vector<unsigned, N> work = xt::zeros<unsigned>({N});
    Point<RealT, N> result;
    Matrix<unsigned, bits, N> vx;
  };

  extern template class SobolSequence<float, 1>;
  extern template class SobolSequence<float, 2>;
  extern template class SobolSequence<float, 3>;
}
  

