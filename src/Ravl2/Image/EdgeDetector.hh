// This file is part of RAVL2, Recognition And Vision Library 2
// Clean-room reimplementation (MIT) of the original RAVL edge-detection /
// edge-linking design by George Matas, Radek Marik and Charles Galambos.

#pragma once

#include <vector>
#include "Ravl2/Array.hh"
#include "Ravl2/Image/Edgel.hh"

namespace Ravl2
{
  //! @brief Parameters for detectEdgeChains().
  struct EdgeDetectorParams
  {
    float minThreshold = 8.0f; //!< hysteresis low: gradient magnitude below this is not an edge
    float maxThreshold = 24.0f;//!< hysteresis high: a chain must contain at least one pixel above this
    bool eightConnect = true;  //!< allow diagonal links between edge pixels
    int minChainLength = 3;    //!< discard chains shorter than this many edgels
  };

  //! @brief Detect edges and link them into ordered, spatially-contiguous chains of edgels.
  //! @details Sobel gradient → non-maximal suppression (octant linear-interpolation thinning) →
  //! hysteresis-seeded contour labelling → chain linking (4-connected first, diagonals only where
  //! the orthogonal neighbours are absent; junctions break chains). Each returned chain is an
  //! ordered run of edge pixels along a single edge — the front end for straight-line fitting.
  //! @param grey Grey image, indexed [row][col].
  //! @param params Detection thresholds + connectivity.
  //! @return Ordered edgel chains (each chain ordered end-to-end along the edge).
  //! @note Thread-safe (reads @p grey, allocates its own working images).
  [[nodiscard]] std::vector<std::vector<Edgel>> detectEdgeChains(const Array<uint8_t, 2> &grey,
                                                                 const EdgeDetectorParams &params = {});

}// namespace Ravl2
