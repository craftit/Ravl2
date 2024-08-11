// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_FFT1D_HEADER
#define RAVL_FFT1D_HEADER 1
////////////////////////////////////////////////////////////////////
//! example=exFFT1d.cc
//! author="Charles Galambos"
//! docentry="Ravl.API.Math.Signals.1D"

#include <complex>
#include "Ravl2/Array1.hh"

namespace Ravl2 {
  
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
  

}

#endif
