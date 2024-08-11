// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////////////

#include "Ravl2/FFT1d.hh"
#include "Ravl2/Slice1d.hh"
#include "Ravl2/Exception.hh"
#include "ccmath/ccmath.h"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  //: Constructor.
  
  FFT1dBodyC::FFT1dBodyC(int nn,bool iinv,bool nZeroPad) 
    : n(0),
      inv(iinv),
      zeroPad(nZeroPad)
  { Init(nn,iinv); }
  
  //: Destructor
  
  FFT1dBodyC::~FFT1dBodyC()  {
  }
  
  //: Create a plan with the given setup.
  
  bool FFT1dBodyC::Init(int nn,bool iinv) {
    // Remeber settings in case we asked...
    inv = iinv;
    pwr2 = IsPow2(nn);
    n = nn;
#if 0
    for(int i = 0;i < 32;i++)
      primeFactors[i] = 0;
#endif
    nf = pfac(n,primeFactors,'o');
    RavlAssertMsg(nf == n,"FFT1dBodyC::Init(), Failed to find prime factors. ");
    return true;
  }
  
  //: Apply transform to array.
  
  Array<std::complex<RealT>,1> FFT1dBodyC::Apply(const Array<std::complex<RealT>,1> &dat)
  {
    ONDEBUG(std::cerr << "FFT1dBodyC::Apply(Array<std::complex<RealT>,1>) n=" << n << " inv=" << inv << " \n");
    Array<std::complex<RealT>,1> ret(n);
    Array<std::complex<RealT>,1> tmpArr(n);
    if (dat.size() < (unsigned) n && zeroPad) {
      ONDEBUG(std::cerr << "Zero padding. \n");
      ret = Array<std::complex<RealT>,1>(n);
      // Copy original data.
      for (BufferAccessIter2C<std::complex<RealT>, std::complex<RealT>> it(dat, tmpArr); it; it++)
        it.data<1>() = it.data<0>();
      // Zero pad it.
      for (BufferAccessIterC<std::complex<RealT>> ita(tmpArr, IndexRange<1>(dat.size(), n - 1)); ita; ita++)
        *ita = 0;
    } else {
      RavlAssert(dat.size() == (unsigned) n);
      tmpArr = dat.Copy();
    }
    Array<ccomplex *,1> ptrArr(n);
    //ptrArr.fill(0);
    //cerr << dat <<  "\n";
    // TODO :- Would it be quicker to copy the array and use fft2 if length is a power of two ?
    if (inv) { // Do inverse.
      for (BufferAccessIter2C<ccomplex *, std::complex<RealT>> it(ptrArr, tmpArr); it; it++)
        it.data<0>() = (ccomplex *) (&it.data<1>());
      fftgc((ccomplex **) ((void *) &(ptrArr[0])), (ccomplex *) ((void *) &(tmpArr[0])), n, primeFactors, 'i');
      for (BufferAccessIter2C<ccomplex *, std::complex<RealT>> itb(ptrArr, ret); itb; itb++)
        itb.data<1>() = *((std::complex<RealT> *) itb.data<0>());
    } else { // Do forward.
      for (BufferAccessIter2C<ccomplex *, std::complex<RealT>> it(ptrArr, ret); it; it++)
        it.data<0>() = (ccomplex *) (&it.data<1>());

      fftgc((ccomplex **) ((void *) &(ptrArr[0])), (ccomplex *) ((void *) &(tmpArr[0])), n, primeFactors, 'd');

      for (BufferAccessIter2C<ccomplex *, std::complex<RealT>> itb(ptrArr, ret); itb; itb++)
        itb.data<1>() = *((std::complex<RealT> *) itb.data<0>());
    }

    //cerr << "result:" << ret << "\n";;
    return ret;
  }
  
  //: Apply transform to array.
  
  Array<std::complex<RealT>,1> FFT1dBodyC::Apply(const Array<RealT,1> &dat) {
    ONDEBUG(std::cerr << "FFT1dBodyC::Apply(Array<RealT,1>) n=" << n << " inv=" << inv << " \n");
    if(dat.size() < (unsigned) n && zeroPad) {
      ONDEBUG(std::cerr << "Zero padding. \n");
      Array<RealT,1> ndat(n);
      // Copy original data.
      for(BufferAccessIter2C<RealT,RealT> it(dat,ndat);it;it++)
        it.data<1>() = it.data<0>();
      // Zero pad it.
      for(BufferAccessIterC<RealT> ita(ndat,IndexRange<1>(dat.size(),n-1));ita;ita++)
        *ita = 0;
      // Then try again.
      return Apply(ndat);
    } else {
      RavlAssert(dat.size() == (unsigned) n);
    }
    Array<std::complex<RealT>,1> ret(n); 
    if(inv)
      fftgr((double *) &(dat[0]),
	    (ccomplex *) ((void *)&(ret[0])),
	    n,
	    primeFactors,
	    'i');
    else
      fftgr((double *) &(dat[0]),
	    (ccomplex *) ((void *)&(ret[0])),
	    n,
	    primeFactors,
	    'd');
    return ret; 
  }

  
}
