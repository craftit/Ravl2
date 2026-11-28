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
#include "Ravl2/Math/FastFourierTransformMath.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //! Declare there are float implementations of the above functions.
  template void fastFourierTransform(std::span<std::complex<float>> result, std::span<const float> data);
  template void inverseFastFourierTransform(std::span<std::complex<float>> result, std::span<const float> data);
  template void fastFourierTransform(std::span<std::complex<float>> result, std::span<const std::complex<float>> data);
  template void inverseFastFourierTransform(std::span<std::complex<float>> result, std::span<const std::complex<float>> data);

  //! And double (the declarations allowed it, but there was no definition to link to).
  template void fastFourierTransform(std::span<std::complex<double>> result, std::span<const double> data);
  template void inverseFastFourierTransform(std::span<std::complex<double>> result, std::span<const double> data);
  template void fastFourierTransform(std::span<std::complex<double>> result, std::span<const std::complex<double>> data);
  template void inverseFastFourierTransform(std::span<std::complex<double>> result, std::span<const std::complex<double>> data);

}// namespace Ravl2
