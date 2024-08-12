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

  using ccomplex = std::complex<float>;
  using Cpx = ccomplex;

  void pshuf(Cpx **pa, Cpx **pb, int *kk, int n)
  {
    int *m, i, j, k, jk;
    ccomplex **p, **q;
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

  void fftgc(Cpx **pc, ccomplex *ft, int n, int *kk, int inv)
  {
    using RealT = float;
    Cpx a, b, z, w, *d, *p, **f, **fb;
    const auto tpi = RealT(6.283185307179586);
    RealT q;
    int *mm, *m, kp, i, j, k, jk, jl;
    std::vector<int> mmVec(size_t(kk[0] + 1));
    mm = mmVec.data();
    std::vector<Cpx> dVec((size_t(kk[kk[0]])));
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
      std::vector<Cpx *> fVec((size_t(n)));
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
      a = std::complex<RealT>(std::cos(q),std::sin(q));
      q *= RealT(ms);
      b = std::complex<RealT>(std::cos(q),std::sin(q));
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

  void fftgr(float *x, ccomplex *ft, int n, int *kk, int inv)
  {
    using RealT = float;
    ccomplex a, b, z, w, *d, *p, *f, *fb;
    const auto tpi = RealT(6.283185307179586);
    RealT sc, q, *t;
    int *mm, *m, kp, i, j, k, jk, jl, ms, mp;
    std::vector<int> mmVec(size_t(kk[0] + 1));
    mm = mmVec.data();
    std::vector<Cpx> dVec((size_t(kk[kk[0]])));
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
      a = std::complex<RealT>(std::cos(q),std::sin(q));
      q *= RealT(ms);
      b = std::complex<RealT>(std::cos(q),std::sin(q));
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
                                       f->imag() + p->imag() * w.real() + p->real() * w.imag()
                                       );
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
  template<typename RealT>
  void computeFFT(std::span<std::complex<RealT> > result,std::span<const RealT> data)
  {
    (void) result;
        (void) data;
  }

  //! @brief Compute inverse Fast Fourier Transform of a real signal.
  //! @param result The output of the FFT.
  //! @param data The input signal.
  template<typename RealT>
  void computeInverseFFT(std::span<std::complex<RealT> > result,std::span<const RealT> data);

  //! @brief Compute the Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT.
  //! @param data The input signal.
  template<typename RealT>
  void computeFFT(std::span<std::complex<RealT> > result,std::span<const std::complex<RealT>> data);

  //! @brief Compute the inverse Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT.
  //! @param data The input signal.
  template<typename RealT>
  void computeInverseFFT(std::span<std::complex<RealT> > result,std::span<const std::complex<RealT>> data);


#if 0
  //: Constructor.

  //! @brief Fast Fourier Transform (FFT) for 1D signals.
  //! Uses the CCMath implementation

  template <typename RealT>
  class FFT1dBodyC
  {
  public:
    FFT1dBodyC(int n,bool iinv,bool zeroPad = false);
    //: Constructor.

    ~FFT1dBodyC();
    //: Destructor

    bool Init(int n,bool iinv);
    //: Create a plan with the given setup.

    Array<std::complex<RealT>,1> Apply(const Array<std::complex<RealT>,1> &dat);
    //: Apply transform to array.
    // Note, only the first 'n' byte of dat are processed.
    // if the array is shorter than the given length, an
    // exception 'ErrorOutOfRangeC' will be thrown.

    Array<std::complex<RealT>,1> Apply(const Array<RealT,1> &dat);
    //: Apply transform to real array
    // Note, only the first 'n' byte of dat are processed.
    // if the array is shorter than the given length, an
    // exception 'ErrorOutOfRangeC' will be thrown.

    int N() const
    { return n; }
    //: The size of the transform.

    bool IsZeroPad() const
    { return zeroPad; }
    //: Test if we're doing zero padding.

  protected:
    int n;  // Size of the transform.
    bool inv; // Is the transform backward ??
    bool pwr2; // Is length a power of two ?
    bool zeroPad; // Zero pad input to 'n' bytes ?
    int primeFactors[32];
    int nf; // Number of factors. Sufficient for all 32-bit lengths.
  };


  template <typename RealT>
  FFT1dBodyC<RealT>::FFT1dBodyC(int nn, bool iinv, bool nZeroPad)
      : n(0),
        inv(iinv),
        zeroPad(nZeroPad)
  {
    Init(nn, iinv);
  }

  //: Create a plan with the given setup.

  template <typename RealT>
  bool FFT1dBodyC<RealT>::Init(int nn, bool iinv)
  {
    // Remember settings in case we asked...
    inv = iinv;
    pwr2 = isPow2(nn);
    n = nn;
    nf = pfac(n, primeFactors, 'o');
    RavlAssertMsg(nf == n, "FFT1dBodyC::Init(), Failed to find prime factors. ");
    return true;
  }

  //: Apply transform to array.

  template <typename RealT>
  Array<std::complex<RealT>, 1> FFT1dBodyC<RealT>::Apply(const Array<std::complex<RealT>, 1> &dat)
  {
    ONDEBUG(std::cerr << "FFT1dBodyC::Apply(Array<std::complex<RealT>,1>) n=" << n << " inv=" << inv << " \n");
    Array<std::complex<RealT>, 1> ret(n);
    Array<std::complex<RealT>, 1> tmpArr(n);
    if(dat.size() < unsigned(n) && zeroPad) {
      ONDEBUG(std::cerr << "Zero padding. \n");
      ret = Array<std::complex<RealT>, 1>(n);
      // Copy original data.
      for(BufferAccessIter2C<std::complex<RealT>, std::complex<RealT>> it(dat, tmpArr); it; it++)
        it.template data<1>() = it.template data<0>();
      // Zero pad it.
      for(BufferAccessIterC<std::complex<RealT>> ita(tmpArr, IndexRange<1>(dat.size(), n - 1)); ita; ita++)
        *ita = 0;
    } else {
      RavlAssert(dat.size() == (unsigned)n);
      tmpArr = dat.Copy();
    }
    Array<ccomplex *, 1> ptrArr(n);
    //ptrArr.fill(0);
    //cerr << dat <<  "\n";
    // TODO :- Would it be quicker to copy the array and use fft2 if length is a power of two ?
    if(inv) {// Do inverse.
      for(BufferAccessIter2C<ccomplex *, std::complex<RealT>> it(ptrArr, tmpArr); it; it++)
        it.data<0>() = (ccomplex *)(&it.data<1>());
      fftgc((ccomplex **)((void *)&(ptrArr[0])), (ccomplex *)((void *)&(tmpArr[0])), n, primeFactors, 'i');
      for(BufferAccessIter2C<ccomplex *, std::complex<RealT>> itb(ptrArr, ret); itb; itb++)
        itb.data<1>() = *((std::complex<RealT> *)itb.data<0>());
    } else {// Do forward.
      for(BufferAccessIter2C<ccomplex *, std::complex<RealT>> it(ptrArr, ret); it; it++)
        it.data<0>() = (ccomplex *)(&it.data<1>());

      fftgc((ccomplex **)((void *)&(ptrArr[0])), (ccomplex *)((void *)&(tmpArr[0])), n, primeFactors, 'd');

      for(BufferAccessIter2C<ccomplex *, std::complex<RealT>> itb(ptrArr, ret); itb; itb++)
        itb.data<1>() = *((std::complex<RealT> *)itb.data<0>());
    }

    //cerr << "result:" << ret << "\n";;
    return ret;
  }

  //: Apply transform to array.

  template <typename RealT>
  Array<std::complex<RealT>, 1> FFT1dBodyC::Apply(const Array<RealT, 1> &dat)
  {
    ONDEBUG(std::cerr << "FFT1dBodyC::Apply(Array<RealT,1>) n=" << n << " inv=" << inv << " \n");
    if(dat.size() < (unsigned)n && zeroPad) {
      ONDEBUG(std::cerr << "Zero padding. \n");
      Array<RealT, 1> ndat(n);
      // Copy original data.
      for(BufferAccessIter2C<RealT, RealT> it(dat, ndat); it; it++)
        it.data<1>() = it.data<0>();
      // Zero pad it.
      for(BufferAccessIterC<RealT> ita(ndat, IndexRange<1>(dat.size(), n - 1)); ita; ita++)
        *ita = 0;
      // Then try again.
      return Apply(ndat);
    } else {
      RavlAssert(dat.size() == (unsigned)n);
    }
    Array<std::complex<RealT>, 1> ret(n);
    if(inv)
      fftgr((double *)&(dat[0]),
            (ccomplex *)((void *)&(ret[0])),
            n,
            primeFactors,
            'i');
    else
      fftgr((double *)&(dat[0]),
            (ccomplex *)((void *)&(ret[0])),
            n,
            primeFactors,
            'd');
    return ret;
  }

#endif
}// namespace Ravl2
