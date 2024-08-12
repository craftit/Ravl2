// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//
/*  fftgc.c    CCMATH mathematics library source code.
*
*  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
*  This code may be redistributed under the terms of the GNU library
*  public license (LGPL). ( See the lgpl.license file for details.)
* ------------------------------------------------------------------------
*/

#include <complex>
#include <numbers>
#include "Ravl2/Assert.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Math/FFT1d.hh"
#include "Ravl2/Math/PrimeFactors.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
  template <typename RealT>
  void fft2(std::complex<RealT> *ft, int m, int inv)
  {
    int i, j, k, mm, mp;
    const RealT tpi = std::numbers::pi_v<RealT> * 2; // 2 * pi
    std::complex<RealT> *p, *q;
    int n = 1 << m;
    std::complex<RealT> *pf = ft + n - 1;
    for(j = 0, p = ft; p < pf; ++p) {
      q = ft + j;
      if(p < q) {
        RealT t = p->real();
        p->real(q->real());
        q->real(t);

        t = p->imag();
        p->imag(q->imag());
        q->imag(t);
      }
      for(mm = n / 2; mm <= j; mm /= 2)
        j -= mm;
      j += mm;
    }
    if(inv == 'd') {
      RealT s = 1. / n;
      for(p = ft; p <= pf;) {
        (*p) *= s;
        p++;
      }
    }
    for(i = mp = 1; i <= m; ++i) {
      mm = mp;
      mp *= 2;
      RealT ang = tpi / mp;
      if(inv == 'd')
        ang = -ang;
      auto w = std::complex<RealT>(std::cos(ang),std::sin(ang));
      for(j = 0; j < n; j += mp) {
        p = ft + j;
        auto u = std::complex<RealT>(1, 0);
        for(k = 0; k < mm; ++k, ++p) {
          q = p + mm;
          std::complex<RealT> t1 = std::complex<RealT>(
            q->real() * u.real() - q->imag() * u.imag(),
            q->imag() * u.real() + q->real() * u.imag()
          );
          *q = p - t1;
          *p += t1;
          u = std::complex<RealT>(u.real() * w.real() - u.imag() * w.imag(),
                                  u.imag() * w.real() + u.real() * w.imag());
        }
      }
    }
  }


  template <typename RealT>
  void pshuf(std::complex<RealT> **pa, std::complex<RealT> **pb, int *kk, int n)
  {
    int *m, i, j, k, jk;
    std::complex<RealT> **p, **q;
    std::vector<int> mm(size_t(kk[0] + 1));
    for(i = 1, mm[0] = 1, m = mm.data(); i <= kk[0]; ++i, ++m)
      *(m + 1) = *m * kk[i];
    for(j = 0, p = pb; j < n; ++j) {
      jk = j;
      q = pa;
      for(i = 1, m = mm.data(); i <= kk[0]; ++i) {
        k = n / *++m;
        q += (jk / k) * *(m - 1);
        jk %= k;
      }
      *q = *p++;
    }
  }

  template <typename RealT>
  void fftgc(std::complex<RealT> **pc, std::complex<RealT> *ft, int n, int *kk, int inv)
  {
    std::complex<RealT> a, b, z, w, *d, *p, **f, **fb;
    const auto tpi = std::numbers::pi_v<RealT> * 2;
    RealT q;
    int *mm, *m, kp, i, j, k, jk, jl;
    std::vector<int> mmVec(size_t(kk[0] + 1));
    mm = mmVec.data();
    std::vector<std::complex<RealT>> dVec((size_t(kk[kk[0]])));
    d = dVec.data();

    for(i = 1, *mm = 1, m = mm; i <= kk[0]; ++i, ++m)
      *(m + 1) = *m * kk[i];

    if(inv == 'd') {
      for(j = 0, p = ft; j < n; ++j) {
        jl = j;
        f = pc;
        for(i = 1, m = mm; i <= kk[0]; ++i) {
          k = n / *(++m);
          f += (jl / k) * *(m - 1);
          jl %= k;
        }
        *f = p++;
      }
    }
    if(inv <= 'e') {
      RealT sc;
      for(i = 0, sc = RealT(1) / RealT(n), p = ft; i < n; ++i) {
        (*p) *= sc;
        p++;
      }
    } else {
      std::vector<std::complex<RealT> *> fVec((size_t(n)));
      f = fVec.data();
      for(j = 0; j < n; ++j)
        f[j] = pc[j];
      pshuf(pc, f, kk, n);
    }
    for(i = 1, m = mm; i <= kk[0]; ++i) {
      int ms = *m++;
      int mp = *m;
      kp = kk[i];
      q = tpi / RealT(mp);
      if(inv <= 'e')
        q = -q;
      a = std::complex<RealT>(std::cos(q), std::sin(q));
      q *= RealT(ms);
      b = std::complex<RealT>(std::cos(q), std::sin(q));
      for(j = 0; j < n; j += mp) {
        fb = pc + j;
        z = std::complex<RealT>(1., 0.);
        for(jk = 0; jk < ms; ++jk, ++fb) {
          w = z;
          for(k = 0, p = d; k < kp; ++k, ++p) {
            f = fb + mp - ms;
            *p = **f;
            while(f > fb) {
              f -= ms;
              *p = std::complex<RealT>((*f)->real() + p->real() * w.real() - p->imag() * w.imag(),
                                       (*f)->imag() + p->imag() * w.real() + p->real() * w.imag());
            }
            w = std::complex<RealT>(w.real() * b.real() - w.imag() * b.imag(),
                                    w.imag() * b.real() + w.real() * b.imag());
          }
          for(k = 0, f = fb, p = d; k < kp; ++k, ++p, f += ms)
            **f = *p;
          z = std::complex<RealT>(z.real() * a.real() - z.imag() * a.imag(),
                                  z.imag() * a.real() + z.real() * a.imag());
        }
      }
    }
  }

  template <typename RealT>
  void fftgr(float *x, std::complex<RealT> *ft, int n, int *kk, int inv)
  {
    std::complex<RealT> a, b, z, w, *d, *p, *f, *fb;
    const auto tpi = RealT(6.283185307179586);
    RealT sc, q, *t;
    int *mm, *m, kp, i, j, k, jk, jl, ms, mp;
    std::vector<int> mmVec(size_t(kk[0] + 1));
    mm = mmVec.data();
    std::vector<std::complex<RealT>> dVec((size_t(kk[kk[0]])));
    d = dVec.data();
    for(i = 1, *mm = 1, m = mm; i <= kk[0]; ++i, ++m)
      *(m + 1) = *m * kk[i];
    for(j = 0, t = x; j < n; ++j) {
      jl = j;
      f = ft;
      for(i = 1, m = mm; i <= kk[0]; ++i) {
        k = n / *++m;
        f += (jl / k) * *(m - 1);
        jl %= k;
      }
      *f = std::complex<RealT>(*t++, 0);
    }
    if(inv == 'd') {
      for(i = 0, sc = RealT(1) / RealT(n), f = ft; i < n; ++i) {
        f->real(f->real() * sc);
        f++;
      }
    }
    for(i = 1, m = mm; i <= kk[0]; ++i) {
      ms = *m++;
      mp = *m;
      kp = kk[i];
      q = tpi / RealT(mp);
      if(inv == 'd')
        q = -q;
      a = std::complex<RealT>(std::cos(q), std::sin(q));
      q *= RealT(ms);
      b = std::complex<RealT>(std::cos(q), std::sin(q));
      for(j = 0; j < n; j += mp) {
        fb = ft + j;
        z = std::complex<RealT>(1., 0.);
        for(jk = 0; jk < ms; ++jk, ++fb) {
          p = d;
          w = z;
          for(k = 0; k < kp; ++k, ++p) {
            f = fb + mp - ms;
            *p = *f;
            while(f > fb) {
              f -= ms;
              *p = std::complex<RealT>(f->real() + p->real() * w.real() - p->imag() * w.imag(),
                                       f->imag() + p->imag() * w.real() + p->real() * w.imag());
            }
            w = std::complex<RealT>(w.real() * b.real() - w.imag() * b.imag(),
                                    w.imag() * b.real() + w.real() * b.imag());
          }
          for(k = 0, f = fb, p = d; k < kp; ++k, f += ms)
            *f = *p++;
          z = std::complex<RealT>(z.real() * a.real() - z.imag() * a.imag(),
                                  z.imag() * a.real() + z.real() * a.imag());
        }
      }
    }
  }

  //! @brief Compute the Fast Fourier Transform of a real signal.
  //! @param result The output of the FFT.
  //! @param data The input signal.
  template <typename RealT>
  void computeFFT(std::span<std::complex<RealT>> result, std::span<const RealT> data)
  {
    if(result.size() != data.size()) {
      throw std::runtime_error("computeFFT(), result and data must be the same size.");
    }
    int n = data.size();
    std::array<int, 32> primeFactors {};
    int nf = pfac(n, primeFactors.data(), 'o');
    if(nf == n) {
      SPDLOG_WARN("FFT1dBodyC::Init(), Failed to find prime factors. ");
      throw std::runtime_error("FFT1dBodyC::Init(), Failed to find prime factors. ");
    }
    fftgr(data.data(),
          result.data(),
          n,
          primeFactors,
          'd');
  }

  //! @brief Compute inverse Fast Fourier Transform of a real signal.
  //! @param result The output of the FFT.
  //! @param data The input signal.
  template <typename RealT>
  void computeInverseFFT(std::span<std::complex<RealT>> result, std::span<const RealT> data)
  {
    if(result.size() != data.size()) {
      throw std::runtime_error("computeFFT(), result and data must be the same size.");
    }
    int n = data.size();
    std::array<int, 32> primeFactors {};
    int nf = pfac(n, primeFactors.data(), 'o');
    if(nf == n) {
      SPDLOG_WARN("FFT1dBodyC::Init(), Failed to find prime factors. ");
      throw std::runtime_error("FFT1dBodyC::Init(), Failed to find prime factors. ");
    }
    fftgr(data.data(),
          result.data(),
          n,
          primeFactors,
          'i');
  }

  //! @brief Compute the Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT.
  //! @param data The input signal.
  template <typename RealT>
  void computeFFT(std::span<std::complex<RealT>> result, std::span<const std::complex<RealT>> data)
  {
    if(result.size() != data.size()) {
      throw std::runtime_error("computeFFT(), result and data must be the same size.");
    }
    int n = data.size();
    std::array<int, 32> primeFactors {};
    int nf = pfac(n, primeFactors.data(), 'o');
    if(nf == n) {
      SPDLOG_WARN("FFT1dBodyC::Init(), Failed to find prime factors. ");
      throw std::runtime_error("FFT1dBodyC::Init(), Failed to find prime factors. ");
    }

    std::vector<std::complex<RealT>> tmpArr(n);
    std::vector<std::complex<RealT> *> ptrArr;
    ptrArr.reserve(size_t(n));
    for(auto &resPtr : tmpArr)
      ptrArr.push_back(&resPtr);

    fftgc(ptrArr.data(), tmpArr.data(), n, primeFactors.data(), 'd');
    for(size_t i = 0; i < result.size(); i++)
      result[i] = *ptrArr[i];
  }

  //! @brief Compute the inverse Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT.
  //! @param data The input signal.
  template <typename RealT>
  void computeInverseFFT(std::span<std::complex<RealT>> result, std::span<const std::complex<RealT>> data)
  {
    if(result.size() != data.size()) {
      throw std::runtime_error("computeFFT(), result and data must be the same size.");
    }
    int n = data.size();
    std::array<int, 32> primeFactors {};
    int nf = pfac(n, primeFactors.data(), 'o');
    if(nf == n) {
      SPDLOG_WARN("FFT1dBodyC::Init(), Failed to find prime factors. ");
      throw std::runtime_error("FFT1dBodyC::Init(), Failed to find prime factors. ");
    }

    std::vector<std::complex<RealT>> tmpArr(n);
    std::vector<std::complex<RealT> *> ptrArr;
    ptrArr.reserve(size_t(n));
    for(auto &resPtr : tmpArr)
      ptrArr.push_back(&resPtr);

    fftgc(ptrArr.data(), tmpArr.data(), n, primeFactors, 'i');
    for(size_t i = 0; i < result.size(); i++)
      result[i] = *ptrArr[i];
  }

}// namespace Ravl2
