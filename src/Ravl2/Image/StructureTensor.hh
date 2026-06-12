//
// Created by charles galambos on 12/06/2026.
//

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Image/Convolve.hh"

namespace Ravl2
{

  //! Scratch buffers for structureTensor(), reused between calls to avoid reallocation.
  template <typename RealT>
  struct StructureTensorWorkspace {
    Array<RealT, 2> smoothed;
    Array<RealT, 2> gradR;
    Array<RealT, 2> gradC;
    Array<RealT, 2> extended;
    Array<RealT, 2> tmp;
  };

  //! @brief Second-moment matrix (structure tensor) field of an image.
  //! The image is smoothed at the differentiation scale sigmaD, gradients are taken
  //! with central differences and scale-normalised by sigmaD, and the gradient outer
  //! products are smoothed at the integration scale sigmaI:
  //!   outRR = g(sigmaI) * (sigmaD * Ir)^2
  //!   outRC = g(sigmaI) * (sigmaD^2 * Ir * Ic)
  //!   outCC = g(sigmaI) * (sigmaD * Ic)^2
  //! All three outputs share the range img.range().shrink(1) (central differences cost
  //! one pixel; the Gaussian smoothing passes are range-preserving), with the same
  //! absolute coordinates as the input.
  //! @param ws Scratch buffers reused between calls.
  template <typename RealT>
  void structureTensor(Array<RealT, 2> &outRR, Array<RealT, 2> &outRC, Array<RealT, 2> &outCC,
                       const Array<RealT, 2> &img, RealT sigmaD, RealT sigmaI,
                       StructureTensorWorkspace<RealT> &ws)
  {
    gaussianBlur(ws.smoothed, img, sigmaD, ws.extended, ws.tmp);
    const IndexRange<2> gradRange = img.range().shrink(1);
    if(ws.gradR.range() != gradRange)
      ws.gradR = Array<RealT, 2>(gradRange);
    if(ws.gradC.range() != gradRange)
      ws.gradC = Array<RealT, 2>(gradRange);
    const RealT half = RealT(0.5) * sigmaD;// Central difference + scale normalisation.
    for(int r : gradRange[0]) {
      for(int c : gradRange[1]) {
        ws.gradR[r][c] = (ws.smoothed[r + 1][c] - ws.smoothed[r - 1][c]) * half;
        ws.gradC[r][c] = (ws.smoothed[r][c + 1] - ws.smoothed[r][c - 1]) * half;
      }
    }
    // Reuse the gradient buffers as product sources one at a time so the
    // integration blur can run in place of a fresh allocation per output.
    if(outRR.range() != gradRange)
      outRR = Array<RealT, 2>(gradRange);
    if(outRC.range() != gradRange)
      outRC = Array<RealT, 2>(gradRange);
    if(outCC.range() != gradRange)
      outCC = Array<RealT, 2>(gradRange);
    for(int r : gradRange[0]) {
      for(int c : gradRange[1]) {
        const RealT gr = ws.gradR[r][c];
        const RealT gc = ws.gradC[r][c];
        outRR[r][c] = gr * gr;
        outRC[r][c] = gr * gc;
        outCC[r][c] = gc * gc;
      }
    }
    gaussianBlur(outRR, outRR, sigmaI, ws.extended, ws.tmp);
    gaussianBlur(outRC, outRC, sigmaI, ws.extended, ws.tmp);
    gaussianBlur(outCC, outCC, sigmaI, ws.extended, ws.tmp);
  }

  //! @brief Convenience overload allocating its own workspace.
  template <typename RealT>
  void structureTensor(Array<RealT, 2> &outRR, Array<RealT, 2> &outRC, Array<RealT, 2> &outCC,
                       const Array<RealT, 2> &img, RealT sigmaD, RealT sigmaI)
  {
    StructureTensorWorkspace<RealT> ws;
    structureTensor(outRR, outRC, outCC, img, sigmaD, sigmaI, ws);
  }

  //! @brief Harris corner response from a precomputed structure tensor field.
  //! out = det(M) - k * trace(M)^2 over the common range of the three planes.
  template <typename RealT>
  void harrisResponse(Array<RealT, 2> &out,
                      const Array<RealT, 2> &rr, const Array<RealT, 2> &rc, const Array<RealT, 2> &cc,
                      RealT k)
  {
    assert(rr.range() == rc.range() && rr.range() == cc.range());
    if(out.range() != rr.range())
      out = Array<RealT, 2>(rr.range());
    for(int r : out.range(0)) {
      for(int c : out.range(1)) {
        const RealT det = rr[r][c] * cc[r][c] - rc[r][c] * rc[r][c];
        const RealT trace = rr[r][c] + cc[r][c];
        out[r][c] = det - k * trace * trace;
      }
    }
  }

  extern template struct StructureTensorWorkspace<float>;
  extern template void structureTensor(Array<float, 2> &, Array<float, 2> &, Array<float, 2> &,
                                       const Array<float, 2> &, float, float, StructureTensorWorkspace<float> &);
  extern template void harrisResponse(Array<float, 2> &, const Array<float, 2> &, const Array<float, 2> &,
                                      const Array<float, 2> &, float);

}// namespace Ravl2
