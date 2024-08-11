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
#include "Ravl2/Math/FFT1d.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  using ccomplex = std::complex<float>;
  using Cpx = ccomplex;

  void pshuf(Cpx **pa,Cpx **pb,int *kk,int n)
  {
    int *m,i,j,k,jk;
    ccomplex **p,**q;
    std::vector<int> mm(size_t(kk[0]+1));
    for(i=1,mm[0]=1,m=mm.data(); i<=kk[0] ;++i,++m)
      *(m+1)= *m*kk[i];
    for(j=0,p=pb; j<n ;++j){
      jk=j; q=pa;
      for(i=1,m=mm.data(); i<=kk[0] ;++i) {
        k=n/ *++m; q+=(jk/k)* *(m-1); jk%=k;
      }
      *q= *p++;
    }
  }

  void fftgc(Cpx **pc,struct ccomplex *ft,int n,int *kk,int inv)
  {
    Cpx a,b,z,w,*d,*p,**f,**fb;
    double tpi=6.283185307179586,sc,q;
    int *mm,*m,kp,i,j,k,jk,jl,ms,mp;
    std::vector<int> mmVec(size_t(kk[0]+1));
    mm=mmVec.data();
    std::vector<Cpx> dVec(size_t(kk[kk[0]]));
    d= dVec.data();

    for(i=1,*mm=1,m=mm; i<=kk[0] ;++i,++m) *(m+1)= *m*kk[i];
    if(inv=='d'){
      for(j=0,p=ft; j<n ;++j){ jl=j; f=pc;
        for(i=1,m=mm; i<=kk[0] ;++i){
          k=n/ *++m; f+=(jl/k)* *(m-1); jl%=k;}
        *f=p++; } }
    if(inv<='e'){ for(i=0,sc=1./n,p=ft; i<n ;++i){
        p->re*=sc; (p++)->im*=sc;} }
    else{ f=(Cpx **)malloc(n*sizeof(pc[0]));
      for(j=0; j<n ;++j) f[j]=pc[j];
      pshuf(pc,f,kk,n); free(f); }
    for(i=1,m=mm; i<=kk[0] ;++i){
      ms= *m++; mp= *m; kp=kk[i]; q=tpi/mp; if(inv<='e') q= -q;
      a.re=cos(q); a.im=sin(q); b.re=cos(q*=ms); b.im=sin(q);
      for(j=0; j<n ;j+=mp){
        fb=pc+j; z.re=1.; z.im=0.;
        for(jk=0; jk<ms ;++jk,++fb){ w=z;
          for(k=0,p=d; k<kp ;++k,++p){
            f=fb+mp-ms; *p= **f;
            while(f>fb){ f-=ms;
              sc=(*f)->re+p->re*w.re-p->im*w.im;
              p->im=(*f)->im+p->im*w.re+p->re*w.im; p->re=sc; }
            sc=w.re*b.re-w.im*b.im;
            w.im=w.im*b.re+w.re*b.im; w.re=sc;
          }
          for(k=0,f=fb,p=d; k<kp ;++k,++p,f+=ms) **f= *p;
          sc=z.re*a.re-z.im*a.im;
          z.im=z.im*a.re+z.re*a.im; z.re=sc;
        }
      }
    }
  }


  void fftgr(double *x,struct ccomplex *ft,int n,int *kk,int inv)
  {
    ccomplex a,b,z,w,*d,*p,*f,*fb;
    double tpi=6.283185307179586,sc,q,*t;
    int *mm,*m,kp,i,j,k,jk,jl,ms,mp;
    mm=(int *)malloc((kk[0]+1)*sizeof(int));
    d=(struct ccomplex *)malloc(kk[*kk]*sizeof(w));
    for(i=1,*mm=1,m=mm; i<=kk[0] ;++i,++m) *(m+1)= *m*kk[i];
    for(j=0,t=x; j<n ;++j){ jl=j; f=ft;
      for(i=1,m=mm; i<=kk[0] ;++i){
        k=n/ *++m; f+=(jl/k)* *(m-1); jl%=k;}
      f->re= *t++; f->im=0.;
    }
    if(inv=='d'){ for(i=0,sc=1./n,f=ft; i<n ;++i) (f++)->re*=sc;}
    for(i=1,m=mm; i<=kk[0] ;++i){
      ms= *m++; mp= *m; kp=kk[i]; q=tpi/mp; if(inv=='d') q= -q;
      a.re=cos(q); a.im=sin(q); b.re=cos(q*=ms); b.im=sin(q);
      for(j=0; j<n ;j+=mp){
        fb=ft+j; z.re=1.; z.im=0.;
        for(jk=0; jk<ms ;++jk,++fb){ p=d; w=z;
          for(k=0; k<kp ;++k,++p){ f=fb+mp-ms; *p= *f;
            while(f>fb){ f-=ms;
              sc=f->re+p->re*w.re-p->im*w.im;
              p->im=f->im+p->im*w.re+p->re*w.im; p->re=sc; }
            sc=w.re*b.re-w.im*b.im;
            w.im=w.im*b.re+w.re*b.im; w.re=sc;
          }
          for(k=0,f=fb,p=d; k<kp ;++k,f+=ms) *f= *p++;
          sc=z.re*a.re-z.im*a.im;
          z.im=z.im*a.re+z.re*a.im; z.re=sc;
        }
      }
    }
    free(d); free(mm);
  }


  //: Constructor.
  
  template <typename RealT>
  FFT1dBodyC::FFT1dBodyC(int nn,bool iinv,bool nZeroPad)
    : n(0),
      inv(iinv),
      zeroPad(nZeroPad)
  { Init(nn,iinv); }
  

  //: Create a plan with the given setup.
  
  template <typename RealT>
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
  
  template <typename RealT>
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
  
  template <typename RealT>
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
