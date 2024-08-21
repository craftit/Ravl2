
#include <cstddef>
#include <array>
#include "Ravl2/Math/PrimeFactors.hh"

namespace Ravl2
{

  /*  pfac.c    CCMATH mathematics library source code.
  *
  *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
  *  This code may be redistributed under the terms of the GNU library
  *  public license (LGPL). ( See the lgpl.license file for details.)
  * ------------------------------------------------------------------------
  */

#define NO_PRIMES 310

  static std::array<size_t, NO_PRIMES> kpf = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,                         /*   0 */
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,                     /*  10 */
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,                /*  20 */
    127, 131, 137, 139, 149, 151, 157, 163, 167, 173,           /*  30 */
    179, 181, 191, 193, 197, 199, 211, 223, 227, 229,           /*  40 */
    233, 239, 241, 251, 257, 263, 269, 271, 277, 281,           /*  50 */
    283, 293, 307, 311, 313, 317, 331, 337, 347, 349,           /*  60 */
    353, 359, 367, 373, 379, 383, 389, 397, 401, 409,           /*  70 */
    419, 421, 431, 433, 439, 443, 449, 457, 461, 463,           /*  80 */
    467, 479, 487, 491, 499, 503, 509, 521, 523, 541,           /*  90 */
    547, 557, 563, 569, 571, 577, 587, 593, 599, 601,           /* 100 */
    607, 613, 617, 619, 631, 641, 643, 647, 653, 659,           /* 110 */
    661, 673, 677, 683, 691, 701, 709, 719, 727, 733,           /* 120 */
    739, 743, 751, 757, 761, 769, 773, 787, 797, 809,           /* 130 */
    811, 821, 823, 827, 829, 839, 853, 857, 859, 863,           /* 140 */
    877, 881, 883, 887, 907, 911, 919, 929, 937, 941,           /* 150 */
    947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013,         /* 160  */
    1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, /* 170  */
    1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, /* 180  */
    1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, /* 190  */
    1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, /* 200  */
    1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373, /* 210  */
    1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, /* 220  */
    1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, /* 230  */
    1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, /* 240  */
    1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, /* 250  */
    1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733, /* 260  */
    1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811, /* 270  */
    1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, /* 280  */
    1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987, /* 290  */
    1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053  /* 300  */
  };

  size_t pfac(size_t n, size_t *kk, int fe)
  {
    size_t dc = 1;
    if(fe == 'e') {
      n -= (n % 2);
      dc = 2;
    }
    for(;; n -= dc) {
      size_t num = n;
      size_t j = 0;
      size_t k = 0;
      while(j < 31) {
        if(num % kpf[k] != 0) {
          if(k == kpf.size())
            break;
          ++k;
        } else {
          kk[++j] = kpf[k];
          num = num / kpf[k];
          if(num == 1) {
            kk[0] = j;
            return n;
          }
        }
      }
    }
    return 0;
  }

  // Convert results to ints for now
  size_t pfac(size_t nn, int *kk, int fe)
  {
    int n = int(nn);
    int num, j, k, dc = 1;
    if(fe == 'e') {
      n -= (n % 2);
      dc = 2;
    }
    for(;; n -= dc) {
      num = n;
      j = k = 0;
      while(j < 31) {
        if(num % int(kpf[size_t(k)]) != 0) {
          if(k == (NO_PRIMES - 1))
            break;
          ++k;
        } else {
          kk[++j] = int(kpf[size_t(k)]);
          num = num / int(kpf[size_t(k)]);
          if(num == 1) {
            kk[0] = j;
            return size_t(n);
          }
        }
      }
    }
    return 0;
  }

}// namespace Ravl2