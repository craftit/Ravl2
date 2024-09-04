
#pragma once

#include <cstdint>
#include <cmath>
#include <numbers>

namespace Ravl2
{

  //! Get the sign of number
  //! returns -1,0 or 1
  template <typename T>
  inline constexpr T sign(T val)
  {
    return T(T(0) < val) - (val < T(0));
  }

  //! Returns the value x rounded to the nearest integer.
  template <typename RealT, typename IntT = int>
   requires std::is_floating_point<RealT>::value
  inline constexpr IntT intRound(RealT x)
  {
    return IntT(std::lrint(x));
    //return static_cast<IntT>((RealT(x) >= 0) ? (x + RealT(0.5)) : (x - RealT(0.5)));
  }

  //! Returns the greatest integral  value  less  than  or equal  to  'x'.
  template <typename RealT, typename IntT = int>
    requires std::is_floating_point<RealT>::value
  inline constexpr IntT intFloor(RealT x)
  {
    return static_cast<IntT>(std::floor(x));
  }

  //! Returns the smallest integral  value  greater  than  or equal  to  'x'.
  template <typename RealT, typename IntT = int>
  constexpr IntT intCeil(RealT x)
  {
    return static_cast<IntT>(std::ceil(x));
  }

  //! @brief Is integer power of 2 ?
  //! @return true if 'i' is a power of 2.

  template <typename IntT>
    requires std::is_integral<IntT>::value
  [[nodiscard]] inline constexpr bool isPow2(IntT i)
  {
    IntT j = 1;
    while(j < i)
      j *= 2;
    return i == j;
  }

  //! @brief Is x near zero ?
  //! By default this will check if the x is within the machine epsilon.
  //! The smallest number that can be added to 1.0 to get a result different from 1.0.
  //! @param x value to check
  //! @param tol tolerance, default is machine epsilon.
  //! @return true if 'x' is near zero.
  template <class RealT>
  inline constexpr bool isNearZero(RealT x, RealT tol = std::numeric_limits<RealT>::epsilon())
  {
    return std::abs(x) < tol;
  }

  //! @brief Returns the square of 'x'.
  template <typename DataT>
  inline constexpr DataT sqr(const DataT &x)
  {
    return x * x;
  }

  //! @brief Convert degrees to radians
  template <typename RealT>
  inline RealT deg2rad(RealT x)
  {
    return x * static_cast<RealT>(std::numbers::pi / 180.0);
  }

  //! @brief Convert radians to degrees
  template <typename RealT>
  inline RealT rad2deg(RealT x)
  {
    return x * static_cast<RealT>(180.0 / std::numbers::pi);
  }

}// namespace Ravl2