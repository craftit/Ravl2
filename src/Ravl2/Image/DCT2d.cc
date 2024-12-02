// This file is part of RAVL, Recognition And Vision Library
//
// Based on code from ImageLib, original copyright.
//
//    This file forms a part of ImageLib, a C++ class library for image
//    processing.
//
//    Copyright (C) 1998-2003 Brendt Wohlberg  <brendt@dip.ee.uct.ac.za>
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// file-header-ends-here
//! license=own

// Modified by Charles Galambos

#include <numbers>
#include "Ravl2/Image/DCT2d.hh"
#include "Ravl2/Assert.hh"
#include "Ravl2/ArrayIterZip.hh"
#include "Ravl2/Image/ZigZagIter.hh"

// Suppress useless warnings from the original code.

#define PIO2 1.5707966327f

namespace Ravl2
{

  VectorT<float> packZigZag(const Array<float, 2> &img, unsigned n)
  {
    RavlAssert(int(n) <= img.range().area());
    n = std::min(n, unsigned(img.range().elements()));
    VectorT<float> ret(n);
    ZigZagIterC zit(img.range());
    for(auto &it : ret) {
      assert(zit.valid());
      it = img[*zit];
      ++zit;
    }
    return ret;
  }

  void
  unpackZigZag(Array<float, 2> &img, const VectorT<float> &vec)
  {
    RavlAssert(size_t(vec.size()) <= img.range().elements());
    auto it = vec.begin();
    const auto end = vec.end();
    for(ZigZagIterC zit(img.range()); it != end; ++zit, ++it)
      img[*zit] = *it;
  }

  static inline constexpr float alpha(int u, int N)
  {
    if(u == 0)
      return std::sqrt(1 / float(N));
    else if((u > 0) && (u < N))
      return std::sqrt(2 / float(N));
    else
      return 0.0f;
  }

  void
  forwardDCT(Array<float, 2> &dest, const Array<float, 2> &src)
  {
    RavlAssertMsg(src.range().size(0) == src.range().size(1), "forwardDCT(): Images must be square.");

    if(dest.range() != src.range())
      dest = Array<float, 2>(src.range());
    // Transform in x direction
    Array<float, 2> horizontal(src.range());
    IndexRange<1> rowRange = src.range(0);
    IndexRange<1> colRange = src.range(1);
    for(auto it = horizontal.begin(); it.valid(); ++it) {
      Index<2> at = it.index();
      int i = at[0];
      int j = at[1];
      float sum = 0.0;
      for(auto k = rowRange.min(); k <= rowRange.max(); k++)
        sum += src[k][j] * std::cos(float(2 * k + 1) * std::numbers::pi_v<float> * float(i) / (float(2 * src.range().size(1))));
      *it = sum;
    }

    // Transform in y direction
    for(auto it = dest.begin(); it.valid(); it++) {
      Index<2> at = it.index();
      int i = at[0];
      int j = at[1];
      float sum = 0.0;
      for(auto k = colRange.min(); k <= colRange.max(); k++)
        sum += horizontal[i][k] * std::cos(float(2 * k + 1) * std::numbers::pi_v<float> * float(j) / (float(2 * src.range().size(0))));
      *it = alpha(i, src.range().size(1)) * alpha(j, src.range().size(0)) * sum;
    }
  }

  void
  inverseDCT(Array<float, 2> &dest, const Array<float, 2> &src)
  {
    RavlAssertMsg(src.range().size(0) == src.range().size(1), "inverseDCT(): Images must be square.");
    if(dest.range() != src.range())
      dest = Array<float, 2>(src.range());
    IndexRange<1> rowRange = src.range(0);
    IndexRange<1> colRange = src.range(1);
    // Transform in x direction
    Array<float, 2> horizontal(src.range());
    for(auto it = horizontal.begin(); it.valid(); it++) {
      Index<2> at = it.index();
      int i = at[0];
      int j = at[1];
      float sum = 0.0f;
      for(auto k = rowRange.min(); k <= rowRange.max(); k++)
        sum += alpha(k, src.range().size(1)) * src[k][j] * std::cos(float(2 * i + 1) * std::numbers::pi_v<float> * float(k) / (float(2 * src.range().size(1))));
      *it = sum;
    }

    // Transform in y direction
    for(auto it = dest.begin(); it.valid(); it++) {
      Index<2> at = it.index();
      int i = at[0];
      int j = at[1];
      float sum = 0.0f;
      for(auto k = colRange.min(); k <= colRange.max(); k++)
        sum += alpha(k, src.range().size(0)) * horizontal[i][k] * std::cos(float(2 * j + 1) * std::numbers::pi_v<float> * float(k) / (float(2 * src.range().size(0))));
      *it = sum;
    }
  }

  /***************************************************************************

  class ChanDCTC is an encapsulation of sofware written by
  Charilaos A. Christopoulos (Email:chchrist@etro.vub.ac.be) - see
  header file for further details

  ***************************************************************************/

  ChanDCT::ChanDCT(unsigned int size)
  {
    if(size > 0)
      setup(size);
  }

  void ChanDCT::setup(unsigned int size)
  {
    if(size == 0) {
      m = 0;
      N = 0;
      return;
    }
    m = intCeil(std::log(size) / std::log(2.0));
    N = 1 << m;
    cosines.resize(size_t(N));
    makecosinetable();
    scaleDC = 1.0f / RealT(N);
    scaleMix = std::sqrt(2.0f) / RealT(N);
    scaleAC = 2.0f * scaleDC;
  }

  void ChanDCT::dct_in_place(Array<RealT, 2> &dest) const
  {
    int n1, k, j, i, i1, l, n2, rows, cols;//p
    const RealT *p;
    rowsinputmapping(dest);
    for(rows = 0; rows < N; rows++) {
      auto destrow = dest[rows];
      p = cosines.data();
      n2 = N;
      for(k = 1; k < m; k++) {
        n1 = n2;
        n2 = n2 >> 1;
        for(j = 0; j < n2; j++) {
          auto c = *(p++);
          RealT *rowi = &(destrow[j]);
          for(i = j; i < N; i += n1, rowi += n1) {
            RealT &rowl = (rowi)[n2];
            auto xt = *rowi - rowl;
            *rowi += rowl;
            rowl = 2 * xt * c;
          }
        }
      }
      auto c = *(p++);
      for(i = 0; i < N; i += 2) {
        RealT &rowi = destrow[i];
        RealT &rowi1 = (&rowi)[1];
        auto xt = rowi;
        rowi += rowi1;
        rowi1 = (xt - rowi1) * c;
      }
    } /* end of for rows */

    rowsbitreversal(dest);
    rowspostadditions(dest);
    columnsinputmapping(dest);
    for(cols = 0; cols < N; cols++) {
      p = cosines.data();
      n2 = N;
      for(k = 1; k < m; k++) {
        n1 = n2;
        n2 = n2 >> 1;
        for(j = 0; j < n2; j++) {
          auto c = *(p++);
          for(i = j; i < N; i += n1) {
            l = i + n2;
            RealT &coli = dest[i][cols];
            RealT &coll = dest[l][cols];
            auto xt = coli - coll;
            coli += coll;
            coll = 2 * xt * c;
          }
        }
      }
      auto c = *(p++);
      for(i = 0; i < N; i += 2) {
        i1 = i + 1;
        RealT &coli = dest[i][cols];
        RealT &coli1 = dest[i1][cols];
        auto xt = coli;
        coli += coli1;
        coli1 = (xt - coli1) * c;
      }
    } /* end of for cols */
    columnsbitreversal(dest);
    columnspostadditions(dest);

    //////// Scale coefficients

    auto it = dest.begin();
    *it *= scaleDC;
    if(!it.next())
      return;// Must be 1x1
    // Do first row.
    do {
      *it *= scaleMix;
    } while(it.next());

    while(it.valid()) {
      *it *= scaleMix;
      if(!it.next())
        break;
      do {
        *it *= scaleAC;
      } while(it.next());
    }
  }

  void
  ChanDCT::forwardDCT(Array<RealT, 2> &dest, const Array<RealT, 2> &src) const
  {
    RavlAssert(src.range().size(1) == N && src.range().size(0) == N);
    dest = clone(src);
    dct_in_place(dest);
  }

  Array<ChanDCT::RealT, 2> ChanDCT::forwardDCT(const Array<RealT, 2> &im) const
  {
    RavlAssert(im.range().size(1) == N && im.range().size(0) == N);
    Array<RealT, 2> ret = clone(im);
    dct_in_place(ret);
    return ret;
  }

  void ChanDCT::makecosinetable()
  {
    auto n2 = N;
    int p = 0;
    for(int k = 1; k < m; k++) {
      auto n1 = n2;
      n2 = n2 >> 1;
      auto e = std::numbers::pi_v<RealT> / float(n1 << 1);
      for(int j = 0; j < n2; j++) {
        cosines[size_t(p++)] = std::cos(float((j << 2) + 1) * e);
      }
    }
    cosines[size_t(p++)] = std::cos(std::numbers::pi_v<RealT> / 4);
  }

  void ChanDCT::columnspostadditions(Array<RealT, 2> &fi) const
  {
    int step, loops, k, ep, j, i, l, cols;

    for(cols = 0; cols < N; cols++) {
      step = N;
      loops = 1;
      for(k = 1; k < m; k++) {
        step = step >> 1;
        ep = step >> 1;
        loops = loops << 1;
        for(j = 0; j < (step >> 1); j++) {
          l = ep;
          RealT *val = &fi[l][cols];
          *val /= 2;
          //	  fi[l][cols] = fi[l][cols]/2;
          for(i = 1; i < loops; i++) {
            RealT *valn = &fi[l + step][cols];
            *valn -= *val;
            val = valn;
            l = l + step;
          }
          ep += 1;
        }
      }
    }
  }

  void ChanDCT::rowspostadditions(Array<RealT, 2> &fi) const
  {
    int step, loops, k, ep, j, i, l, rows;

    /* Postaditions for the columns */
    for(rows = 0; rows < N; rows++) {
      auto destrow = fi[rows];
      step = N;
      loops = 1;
      for(k = 1; k < m; k++) {
        step = step >> 1;
        ep = step >> 1;
        loops = loops << 1;
        for(j = 0; j < (step >> 1); j++) {
          l = ep;
          RealT *val = &destrow[l];
          *val /= 2;
          for(i = 1; i < loops; i++) {
            RealT *valn = val + step;
            *valn -= *val;
            val = valn;
          }
          ep += 1;
        }
      }
    }
  }

  void ChanDCT::rowsbitreversal(Array<RealT, 2> &fi) const
  {
    int v1, v2, v3, i, j, k, cols;

    /* revesre rows */
    for(cols = 0; cols < N; cols++) {
      auto destrow = fi[cols];
      v1 = (m + 1) / 2;
      v2 = 1 << v1;
      v3 = N - 1 - v2;
      j = 0;
      for(i = 1; i <= v3; i++) {
        k = N >> 1;
        while(k <= j) {
          j = j - k;
          k = k >> 1;
        }
        j += k;
        if(i < j) {
          RealT &fij = destrow[j];
          RealT &fii = destrow[i];
          auto xt = fij;
          fij = fii;
          fii = xt;
        }
      }
    }
  }

  void ChanDCT::columnsbitreversal(Array<RealT, 2> &fi) const
  {
    int v1, v2, v3, i, j, k, rows;
    /* reverse columns */
    for(rows = 0; rows < N; rows++) {
      v1 = (m + 1) / 2;
      v2 = 1 << v1;
      v3 = N - 1 - v2;
      j = 0;
      for(i = 1; i <= v3; i++) {
        k = N >> 1;
        while(k <= j) {
          j = j - k;
          k = k >> 1;
        }
        j += k;
        if(i < j) {
          std::swap(fi[j][rows], fi[i][rows]);
        }
      }
    }
  }

  void ChanDCT::columnsinputmapping(Array<RealT, 2> &fi) const
  {
    int rows, n;
    Array<RealT, 2> s(fi.range());//double s[512][512];
    copy(s, fi);
    for(rows = 0; rows < N; rows++) {
      for(n = 0; n < N / 2; n++) {
        fi[n][rows] = s[2 * n][rows];
        fi[N - n - 1][rows] = s[2 * n + 1][rows];
      }
    }
  }

  void ChanDCT::rowsinputmapping(Array<RealT, 2> &fi) const
  {
    int cols, n;
    Array<RealT, 2> s(fi.range());//double s[512][512];
    copy(s, fi);
    for(cols = 0; cols < N; cols++) {
      auto firow = fi[cols];
      auto srow = s[cols];
      for(n = 0; n < N / 2; n++) {
        firow[n] = srow[2 * n];
        firow[N - n - 1] = srow[2 * n + 1];
      }
    }
  }

  /***************************************************************************

  class VecRadDCT is an encapsulation of sofware written by
  Charilaos A. Christopoulos (Email:chchrist@etro.vub.ac.be) - see
  header file for further details

  ***************************************************************************/

#ifndef __clang__
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

  VecRadDCT::VecRadDCT(unsigned int size, unsigned int pts)
  {
    if(size > 0)
      setup(size, pts);
  }

  void VecRadDCT::setup(unsigned int size, unsigned int pts)
  {
    m = intCeil(std::log(size) / std::log(2));
    N = 1 << m;
    N0 = 1 << intCeil(std::log(pts) / std::log(2));// must be power of 2

    // Allocate ptr array.
    r.resize(size_t(N));

    // Allocate space, and initialise ptr array.
    rpData.resize(N * N);
    auto rp = rpData.data();
    for(int i = 0; i < int(N); i++, rp += N)
      r[size_t(i)] = rp;

    cosine_array.resize(N * size_t(m));
    ct.resize(N);
    ct2d.resize(N * N * size_t(m));

    MASK[0] = 0;
    MASK[1] = ~((-1u) << m);

    lut_of_cos();
    expand1d_lookup_table();
    make2Darray();

    scaleDC = 1.0f / RealT(N);
    scaleMix = std::sqrt(2.0f) / RealT(N);
    scaleAC = 2.0f * scaleDC;
  }

  void VecRadDCT::inPlaceDCT(Array<RealT, 2> &im, bool modifyOutputRect) const
  {
    int q;
    int i, j;
    //RealT sum1,sum2,diff1,diff2;

    firo3(im);

    /* decimation in time DCT */

    /* Perform the first stage of the transform */
    size_t step = 0;

    for(int yi = 0; yi < int(N); yi += 2) {
      auto dest_yi1 = im[yi];
      auto dest_yi2 = im[yi + 1];

      for(int xj = 0; xj < int(N); xj += 2) {
        int xj1 = xj;
        int xj2 = xj1 + 1;

        RealT S0 = dest_yi1[xj1];
        RealT S1 = dest_yi2[xj1];
        RealT S2 = dest_yi1[xj2];
        RealT S3 = dest_yi2[xj2];

        auto sum1 = RealT(S0 + S1);
        auto sum2 = RealT(S2 + S3);
        auto diff1 = RealT(S0 - S1);
        auto diff2 = RealT(S2 - S3);

        dest_yi1[xj1] = sum1 + sum2;
        dest_yi2[xj1] = (diff1 + diff2) * ct2d[step++];
        dest_yi1[xj2] = (sum1 - sum2) * ct2d[step++];
        dest_yi2[xj2] = (diff1 - diff2) * ct2d[step++];
      }
    }

    /* Perform the remaining stages of the transform */
    int bB = 1;
    int mmax = 2;
    while(int(N) > mmax) {
      bB = bB << 2;
      q = N0 * N0 / bB;
      auto istep = 2 * mmax;

      for(int k1 = 0; k1 < mmax; k1++) {
        for(int k2 = 0; k2 < mmax; k2++) {
          for(int yi = k1; yi < int(N); yi += istep) {
            auto dest_yi1 = im[yi];
            auto dest_yi2 = im[yi + mmax];
            for(int xj = k2; xj < int(N); xj += istep) {
              int xj1 = xj;
              int xj2 = xj1 + mmax;

              RealT S0 = dest_yi1[xj1];
              RealT S1 = dest_yi2[xj1];
              RealT S2 = dest_yi1[xj2];
              RealT S3 = dest_yi2[xj2];

              auto sum1 = RealT(S0 + S1);
              auto sum2 = RealT(S2 + S3);
              auto diff1 = RealT(S0 - S1);
              auto diff2 = RealT(S2 - S3);

              if(q <= 1) {
                dest_yi1[xj1] = sum1 + sum2;
                step += 3;
              } else {// if q > 1
                dest_yi1[xj1] = sum1 + sum2;
                dest_yi2[xj1] = (diff1 + diff2) * ct2d[step++];
                dest_yi1[xj2] = (sum1 - sum2) * ct2d[step++];
                dest_yi2[xj2] = (diff1 - diff2) * ct2d[step++];
              }
            }
          }
        }
      }
      mmax = istep;
    }

    post_adds(im);
    //Scale coefficients
    im[0][0] *= scaleDC;
    auto destzero = im[0];
    for(i = 1; i < N0; i++) {
      im[i][0] *= scaleMix;
      destzero[i] *= scaleMix;
    }
    for(i = 1; i < N0; i++) {
      auto desti = im[i];
      for(j = 1; j < N0; j++)
        desti[j] *= scaleAC;
    }

    if(modifyOutputRect)
      im = Array<RealT, 2>(im, IndexRange<2>({{0, N0 - 1}, {0, N0 - 1}}));
  }

  void
  VecRadDCT::forwardDCT(Array<RealT, 2> &dest, const Array<RealT, 2> &src) const
  {
    RavlAssert(src.range().size(1) == int(N) && src.range().size(0) == int(N));
    dest = clone(src);
    inPlaceDCT(dest);
  }

  Array<VecRadDCT::RealT, 2> VecRadDCT::forwardDCT(const Array<RealT, 2> &im) const
  {
    RavlAssert(im.range().size(1) == int(N) && im.range().size(0) == int(N));
    Array<RealT, 2> ret = clone(im);
    inPlaceDCT(ret);
    return ret;
  }

  void VecRadDCT::lut_of_cos()
  {
    std::vector<unsigned int> et(static_cast<unsigned long>(N));
    size_t p = 0;
    int mm1 = m - 1;
    unsigned e = 1;

    for(int k = 0; k < m; k++) {
      int len = 1;
      auto inc = unsigned(N);
      size_t i = 0;
      et[i] = e;
      i++;
      ct[p] = RealT(2.0f * std::cos(PIO2 * RealT(e) / RealT(N)));
      p++;
      for(int t = 0; t < mm1; t++) {
        for(int l = 0; l < len; l++) {
          et[i] = et[size_t(l)] + inc;
          ct[p] = RealT(2.0f * std::cos(RealT(et[i]) * PIO2 / RealT(N)));
          i++;
          p++;
        }
        len = len << 1;
        inc = inc >> 1;
      }
      e = e << 1;
      mm1 = mm1 - 1;
    }
  }

  void VecRadDCT::expand1d_lookup_table()
  {
    size_t ncb = 0;

    auto Bs = N;
    size_t bB = 1;
    size_t bls = 1;
    int p = 0;
    size_t step = 0;

    for(int k = 0; k < m; k++) {
      Bs = Bs >> 1;
      auto q = N / bB;
      auto xr = N % bB;
      auto ble = step;
      bls = bls << 1;

      if(q == 1) {
        ncb = xr;
      }
      if(q < 1) {
        ncb = 0;
      }
      if(q > 1) {
        ncb = bB;
      }

      for(size_t j = 0; j < Bs; j++) {
        auto l = ble;
        auto c = ct[size_t(p)];
        p++;
        for(size_t i = 0; i < ncb; i++) {
          cosine_array[l + step] = 1.0f;
          cosine_array[step + l + bB] = RealT(c);
          l++;
        }

        ble += bls;
      }
      bB = bB << 1;
      step += N / 2;
    }
  }

  void VecRadDCT::make2Darray()
  {
    size_t ND1 = 0;
    size_t MD1 = 0;
    size_t cos_step = 0;
    size_t step = 0;

    for(size_t yi = 0; yi < N; yi += 2) {
      size_t yi1 = yi + MD1;
      size_t yi2 = yi1 + 1;
      for(size_t xj = 0; xj < N; xj += 2) {
        size_t xj1 = xj + ND1;
        size_t xj2 = xj1 + 1;
        ct2d[step++] = cosine_array[yi2] * cosine_array[xj1];
        ct2d[step++] = cosine_array[yi1] * cosine_array[xj2];
        ct2d[step++] = cosine_array[yi2] * cosine_array[xj2];
      }
    }
    /* Find cosines for the remaining stages of the transform */
    size_t mmax = 2;
    while(N > mmax) {
      cos_step += size_t(N);
      auto istep = 2 * mmax;

      for(size_t k1 = 0; k1 < mmax; k1++) {
        for(size_t k2 = 0; k2 < mmax; k2++) {
          for(size_t yi = k1; yi < N; yi += istep) {
            size_t yi1 = yi + MD1;
            size_t yi2 = yi1 + mmax;
            for(size_t xj = k2; xj < N; xj += istep) {
              size_t xj1 = xj + ND1;
              size_t xj2 = xj1 + mmax;

              ct2d[step++] = cosine_array[cos_step + yi2] * cosine_array[cos_step + xj1];
              ct2d[step++] = cosine_array[cos_step + yi1] * cosine_array[cos_step + xj2];
              ct2d[step++] = cosine_array[cos_step + yi2] * cosine_array[cos_step + xj2];

            } /* x*/
          } /*y */
        } /* k2 */
      } /* k1 */
      mmax = istep;
    } /* while */
  }

  void VecRadDCT::firo3(Array<RealT, 2> &fi) const
  {
    int eo, group, nog, p, q, F;

    bitreversalrows();
    for(size_t rows = 0; rows < N; rows++) {
      auto M = m;
      eo = M % 2;
      M = m >> 1;
      group = nog = 1 << M;
      if(eo == 1) M++;

      /*..................... M=even/odd ..........................*/

      //cerr << "VecRadDCTC::firo3 Loop1 \n";
      for(int i = 0; i < (nog - 1); i++) {
        F = 0;
        q = i << M;
        p = q >> 1;
        for(int j = 1; j < group; j++) {
          F = 1 - F;
          q++;
          auto a = (((r[size_t(p)][rows]) << 1) ^ (MASK[F])); /* CC*/
          std::swap(fi[int(a)][int(rows)], fi[q][int(rows)]);
          p += F;
        }
        group--;
      }
      //cerr << "VecRadDCTC::firo3 Loop2 \n";

      if(eo != 0) {
        /*....................... M=odd ..........................*/

        group = nog;
        //cerr << "VecRadDCTC::firo3 Loop2 \n";

        for(int i = 1; i < nog; i++) {
          F = 0;
          q = i << M;
          p = q >> 1;
          p--;
          q--;
          for(int j = 1; j < group; j++) {
            q--;
            auto a = ((r[size_t(p)][rows] << 1) ^ MASK[F]); /* CC*/
            auto b = q;                                     /*CC*/
            std::swap(fi[int(a)][int(rows)], fi[b][int(rows)]);
            F = 1 - F;
            p -= F;
          }
          group--;
        }
      } /* end of 'if' statement */

    } /* end for rows */

    bitreversalcolumns();
    //cerr << "VecRadDCTC::firo3 Loop3 \n";

    /* Input reordering for the columns */
    for(int cols = 0; cols < int(N); cols++) {
      auto ficol = fi[cols];
      const unsigned *rcol = r[size_t(cols)];
      auto M = m;
      eo = M % 2;
      M = m >> 1;
      group = nog = 1 << M;
      if(eo == 1) M++;

      /*..................... M=even/odd ..........................*/

      for(int i = 0; i < (nog - 1); i++) {
        F = 0;
        q = i << M;
        p = q >> 1;
        for(int j = 1; j < group; j++) {
          F = 1 - F;
          q++;
          auto a = ((rcol[p] << 1) ^ MASK[F]); /* CC*/
          auto b = q;                          /*CC*/
          std::swap(ficol[int(a)], ficol[b]);
          p += F;
        }
        group--;
      }

      if(eo != 0) {
        /*....................... M=odd ..........................*/
        group = nog;
        //cerr << "VecRadDCTC::firo3 Loop4 \n";

        for(int i = 1; i < nog; i++) {
          F = 0;
          q = i << M;
          p = q >> 1;
          p--;
          q--;
          for(int j = 1; j < group; j++) {
            q--;
            auto a = ((rcol[p] << 1) ^ MASK[F]); /* CC*/
            auto b = q;                          /*CC*/
            std::swap(ficol[int(a)], ficol[b]);
            F = 1 - F;
            p -= F;
          }
          group--;
        }
      } /* end of 'if' statement */
    } /* end for rows */
  }

  void VecRadDCT::bitreversalrows() const
  {
    for(size_t rows = 0; rows < N; rows++) {
      size_t l = 1;
      r[0][rows] = 0;
      for(size_t i = 1; i < size_t(m); i++) {
        for(size_t j = 0; j < l; j++) {
          unsigned &val = r[j][rows];
          val <<= 1;
          r[j + l][rows] = val + 1;
        }
        l <<= 1;
      }
    } /* end for rows */
  }

  void VecRadDCT::bitreversalcolumns() const
  {
    for(size_t cols = 0; cols < N; cols++) {
      size_t l = 1;
      unsigned *rc = r[cols];
      rc[0] = 0;
      for(size_t i = 1; i < size_t(m); i++) {
        for(size_t j = 0; j < l; j++) {
          unsigned *val = &(rc[j]);
          (*val) <<= 1;
          val[l] = (*val) + 1;
        }
        l <<= 1;
      }
    } /* end for cols */
  }

  void VecRadDCT::post_adds(Array<RealT, 2> &fi) const
  {
    /* Do divisions by 2 */
    {
      auto firow = fi[0];
      for(int j = 1; j < int(N); j++)
        firow[j] *= 0.5f;
    }
    for(int i = 1; i < int(N); i++) {
      auto firow = fi[i];
      firow[0] *= 0.5f;
      for(int j = 1; j < int(N); j++)
        firow[j] *= 0.25f;
    }

    /* Postadditions for the rows */
    for(int cols = 0; cols < int(N); cols++) {
      int step = int(N);
      int loops = 1;
      for(int k = 1; k < m; k++) {
        step = step >> 1;
        int ep = step >> 1;
        loops = loops << 1;
        for(int j = 0; j < (step >> 1); j++) {
          int l = ep;
          for(int i = 1; i < loops; i++) {
            int z = l + step;
            fi[z][cols] -= fi[l][cols];
            l = z;
          }
          ep += 1;
        }
      }
    }

    /* Postaditions for the columns */
    for(int rows = 0; rows < int(N); rows++) {
      auto firow = fi[rows];
      int step = int(N);
      int loops = 1;
      for(int k = 1; k < m; k++) {
        step = step >> 1;
        int ep = step >> 1;
        loops = loops << 1;
        for(int j = 0; j < (step >> 1); j++) {
          int l = ep;
          for(int i = 1; i < loops; i++) {
            int z = l + step;
            firow[z] -= firow[l];
            l = z;
          }
          ep += 1;
        }
      }
    }
  }
}// namespace Ravl2
