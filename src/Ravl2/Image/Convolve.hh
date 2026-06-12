//
// Created by charles galambos on 12/06/2026.
//

#pragma once

#include <cassert>
#include <span>
#include <vector>
#include "Ravl2/Array.hh"
#include "Ravl2/Image/ImageExtend.hh"

namespace Ravl2
{

  //! @brief Sampled, normalised 1D Gaussian kernel.
  //! The kernel has size 2*ceil(extent*sigma)+1 and its elements sum to 1.
  //! @param sigma Standard deviation of the Gaussian in pixels, must be > 0.
  //! @param extent Number of standard deviations covered by the kernel radius.
  //! @return The sampled kernel, centre element at index kernel.size()/2.
  template <typename RealT>
  [[nodiscard]] std::vector<RealT> gaussianKernel(RealT sigma, RealT extent = RealT(3))
  {
    assert(sigma > 0);
    const int radius = std::max(1, int(std::ceil(extent * sigma)));
    std::vector<RealT> kernel(size_t(2 * radius + 1));
    RealT sum = 0;
    for(int i = -radius; i <= radius; ++i) {
      const RealT x = RealT(i);
      const RealT value = std::exp(-(x * x) / (2 * sigma * sigma));
      kernel[size_t(i + radius)] = value;
      sum += value;
    }
    for(auto &value : kernel) {
      value /= sum;
    }
    return kernel;
  }

  //! @brief Correlate image rows with a 1D kernel (along the contiguous column dimension).
  //! out[r][c] = sum_i kernel[i] * img[r][c + i - radius], radius = kernel.size()/2.
  //! Note these are correlation semantics: the kernel is not flipped. This makes no
  //! difference for symmetric kernels.
  //! The output range is the input range shrunk by radius in dim 1 only, keeping the
  //! same absolute coordinates (the edgeSobel convention). 'out' is reallocated if its
  //! range does not match.
  template <typename OutT, typename InT, typename KernelT>
  void convolveHorizontal(Array<OutT, 2> &out, const Array<InT, 2> &img, std::span<const KernelT> kernel)
  {
    assert(kernel.size() % 2 == 1);
    const int radius = int(kernel.size()) / 2;
    IndexRange<2> outRange(img.range(0), img.range(1).shrink(radius));
    assert(!outRange.empty());
    if(outRange != out.range())
      out = Array<OutT, 2>(outRange);
    using AccT = decltype(KernelT() * InT());
    for(int r : outRange[0]) {
      const InT *rowPtr = &(img[r][outRange[1].min()]);
      OutT *outPtr = &(out[r][outRange[1].min()]);
      const OutT *outEnd = outPtr + outRange[1].size();
      for(; outPtr != outEnd; ++outPtr, ++rowPtr) {
        AccT sum = 0;
        for(size_t i = 0; i < kernel.size(); ++i) {
          sum += kernel[i] * AccT(rowPtr[int(i) - radius]);
        }
        *outPtr = OutT(sum);
      }
    }
  }

  //! @brief Correlate image columns with a 1D kernel (along the row dimension).
  //! out[r][c] = sum_i kernel[i] * img[r + i - radius][c], radius = kernel.size()/2.
  //! The output range is the input range shrunk by radius in dim 0 only, keeping the
  //! same absolute coordinates. 'out' is reallocated if its range does not match.
  template <typename OutT, typename InT, typename KernelT>
  void convolveVertical(Array<OutT, 2> &out, const Array<InT, 2> &img, std::span<const KernelT> kernel)
  {
    assert(kernel.size() % 2 == 1);
    const int radius = int(kernel.size()) / 2;
    IndexRange<2> outRange(img.range(0).shrink(radius), img.range(1));
    assert(!outRange.empty());
    if(outRange != out.range())
      out = Array<OutT, 2>(outRange);
    using AccT = decltype(KernelT() * InT());
    std::vector<const InT *> rowPtrs(kernel.size());
    for(int r : outRange[0]) {
      OutT *outPtr = &(out[r][outRange[1].min()]);
      const OutT *outEnd = outPtr + outRange[1].size();
      for(size_t i = 0; i < kernel.size(); ++i) {
        rowPtrs[i] = &(img[r + int(i) - radius][outRange[1].min()]);
      }
      for(; outPtr != outEnd; ++outPtr) {
        AccT sum = 0;
        for(size_t i = 0; i < kernel.size(); ++i) {
          sum += kernel[i] * AccT(*(rowPtrs[i]++));
        }
        *outPtr = OutT(sum);
      }
    }
  }

  //! @brief Separable correlation: vertical pass then horizontal pass.
  //! The output range is the input range shrunk by vertKernel.size()/2 in dim 0 and
  //! horzKernel.size()/2 in dim 1, same absolute coordinates.
  //! @param tmp Scratch buffer reused between calls to avoid reallocation.
  template <typename OutT, typename InT, typename KernelT>
  void convolveSeparable(Array<OutT, 2> &out, const Array<InT, 2> &img,
                         std::span<const KernelT> vertKernel, std::span<const KernelT> horzKernel,
                         Array<OutT, 2> &tmp)
  {
    convolveVertical(tmp, img, vertKernel);
    convolveHorizontal(out, tmp, horzKernel);
  }

  //! @brief Range-preserving Gaussian smoothing with caller-supplied workspace.
  //! Mirror-extends 'img' by the kernel radius (extendImageMirror) before convolving,
  //! so out.range() == img.range().
  //! @param extended Scratch for the mirror-extended image, reused between calls.
  //! @param tmp Scratch for the vertical convolution pass, reused between calls.
  template <typename OutT, typename InT, typename RealT>
  void gaussianBlur(Array<OutT, 2> &out, const Array<InT, 2> &img, RealT sigma,
                    Array<InT, 2> &extended, Array<OutT, 2> &tmp, RealT extent = RealT(3))
  {
    const std::vector<RealT> kernel = gaussianKernel(sigma, extent);
    const auto radius = unsigned(kernel.size() / 2);
    extendImageMirror(extended, img, radius);
    convolveSeparable(out, extended, std::span<const RealT>(kernel), std::span<const RealT>(kernel), tmp);
  }

  //! @brief Range-preserving Gaussian smoothing.
  //! Allocates working buffers internally; use the workspace overload in hot loops.
  template <typename OutT, typename InT, typename RealT>
  void gaussianBlur(Array<OutT, 2> &out, const Array<InT, 2> &img, RealT sigma, RealT extent = RealT(3))
  {
    Array<InT, 2> extended;
    Array<OutT, 2> tmp;
    gaussianBlur(out, img, sigma, extended, tmp, extent);
  }

  extern template void convolveHorizontal(Array<float, 2> &, const Array<float, 2> &, std::span<const float>);
  extern template void convolveVertical(Array<float, 2> &, const Array<float, 2> &, std::span<const float>);
  extern template void convolveSeparable(Array<float, 2> &, const Array<float, 2> &, std::span<const float>, std::span<const float>, Array<float, 2> &);
  extern template void gaussianBlur(Array<float, 2> &, const Array<float, 2> &, float, Array<float, 2> &, Array<float, 2> &, float);
  extern template void gaussianBlur(Array<float, 2> &, const Array<float, 2> &, float, float);

}// namespace Ravl2
