
#pragma once

#include <cmath>

namespace Ravl2
{
    //! Define the type of real number
    using RealT = float;

    //! Get the sign of number
    //! returns -1,0 or 1
    template <typename T> inline T sign(T val)
    { return T(T(0) < val) - (val < T(0)); }


    //! Returns the value x rounded to the nearest integer.
    template<class RealT, class IntT = int>
    inline IntT int_round(RealT x) { return static_cast<IntT>((x >= 0) ? (x + 0.5) : (x - 0.5)); }

    //! Returns the greatest integral  value  less  than  or equal  to  'x'.
    template<class RealT, class IntT = int>
    inline IntT int_floor(RealT x) {
#if 0
      auto y = static_cast<IntT>(x);
      if (x >= 0) return y;
      return ((static_cast<RealT>(y) != x) ? --y : y);
#else
      return static_cast<IntT>(std::floor(x));
#endif
    }

    template<class RealT, class IntT = int>
    IntT int_ceil(RealT x) {
      return static_cast<IntT>(std::ceil(x));
    }

    //! Returns the square of 'x'.
    template<typename DataT>
    inline DataT sqr(const DataT &x) { return x * x; }

}