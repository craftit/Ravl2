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
#include <vector>

namespace Ravl2
{

  //! @brief Compute the Fast Fourier Transform of a real signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template <typename RealT>
  void fastFourierTransform(std::span<std::complex<RealT>> result, std::span<const RealT> data);

  //! @brief Compute inverse Fast Fourier Transform of a real signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template <typename RealT>
  void inverseFastFourierTransform(std::span<std::complex<RealT>> result, std::span<const RealT> data);

  //! @brief Compute the Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template <typename RealT>
  void fastFourierTransform(std::span<std::complex<RealT>> result, std::span<const std::complex<RealT>> data);

  //! @brief Compute the inverse Fast Fourier Transform of a complex signal.
  //! @param result The output of the FFT, must be the same size as the input signal.
  //! @param data The input signal.
  template <typename RealT>
  void inverseFastFourierTransform(std::span<std::complex<RealT>> result, std::span<const std::complex<RealT>> data);

  //! @brief Compute the Fast Fourier Transform of a real signal.
  //! @param data The input signal.  maybe a std::vector, std::array, or similar.
  //! @return The FFT of 'data'
  template <typename RealT, typename ContainerT>
  [[nodiscard]] std::vector<std::complex<RealT>> fastFourierTransform(const ContainerT &data)
  {
    std::vector<std::complex<RealT>> result(data.size());
    using std::span;
    fastFourierTransform(span(result), span(data));
    return result;
  }

  //! @brief Compute inverse Fast Fourier Transform of a real signal.
  //! @param data The input signal. maybe a std::vector, std::array, or similar.
  //! @return The inverse FFT of 'data'
  template <typename RealT, typename ContainerT>
  [[nodiscard]] std::vector<std::complex<RealT>> inverseFastFourierTransform(const ContainerT &data)
  {
    std::vector<std::complex<RealT>> result(data.size());
    using std::span;
    inverseFastFourierTransform(span(result), span(data));
    return result;
  }

  //! Declare there are float implementations of the above functions.
  extern template void fastFourierTransform(std::span<std::complex<float>> result, std::span<const float> data);
  extern template void inverseFastFourierTransform(std::span<std::complex<float>> result, std::span<const float> data);
  extern template void fastFourierTransform(std::span<std::complex<float>> result, std::span<const std::complex<float>> data);
  extern template void inverseFastFourierTransform(std::span<std::complex<float>> result, std::span<const std::complex<float>> data);

}// namespace Ravl2

#endif
