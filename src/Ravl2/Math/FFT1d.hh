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
#include <span>
#include "Ravl2/Array1.hh"

namespace Ravl2 {
  

  //! @brief Compute the Fast Fourier Transform of a real signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template<typename RealT>
  void computeFFT(std::span<std::complex<RealT> > result,std::span<const RealT> data);

  //! @brief Compute inverse Fast Fourier Transform of a real signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template<typename RealT>
  void computeInverseFFT(std::span<std::complex<RealT> > result,std::span<const RealT> data);

  //! @brief Compute the Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template<typename RealT>
  void computeFFT(std::span<std::complex<RealT> > result,std::span<const std::complex<RealT>> data);

  //! @brief Compute the inverse Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template<typename RealT>
  void computeInverseFFT(std::span<std::complex<RealT> > result,std::span<const std::complex<RealT>> data);

  //! Declare there are flloat implementations of the above functions.
  extern template void computeFFT(std::span<std::complex<float> > result,std::span<const float> data);
  extern template void computeInverseFFT(std::span<std::complex<float> > result,std::span<const float> data);
  extern template void computeFFT(std::span<std::complex<float> > result,std::span<const std::complex<float>> data);
  extern template void computeInverseFFT(std::span<std::complex<float> > result,std::span<const std::complex<float>> data);


}

#endif
