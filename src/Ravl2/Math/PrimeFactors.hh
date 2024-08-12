
// Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
// This code may be redistributed under the terms of the GNU library
// public license (LGPL). ( See the lgpl.license file for details.)
// Code from the CCMATH library.
//  pfac.c    CCMATH mathematics library source code.

#pragma once

namespace Ravl2
{

  //! @brief Factor an integer into its prime factors.
  //
  //     int pfac(int n,int *kk,int fe)
  //        n = input integer
  //        kk = pointer to array containing factors of n, with
  //               kk[0] = number of factors and the output returned
  //                       n' = kk[1]*kk[2]* -- *kk[kk[0]]
  //               (The dimension of kk should be 32.)
  //        fe = control flag, with:
  //               fe = 'e' -> even integer output required
  //               fe = 'o' -> odd integers allowed
  //       return value: n' = integer factored (n' <= n)
  //
  //
  //          All prime factors < 101 are considered, with n' decremented
  //          if a factorization fails. The dimension 32 for the factor
  //          array kk is sufficient to hold all factors of 32-bit integers.

  size_t pfac(size_t n,size_t *kk,int fe);

  size_t pfac(size_t n, int *kk, int fe);

}