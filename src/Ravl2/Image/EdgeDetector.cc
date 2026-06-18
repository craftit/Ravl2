// This file is part of RAVL2, Recognition And Vision Library 2
// Clean-room reimplementation (MIT) of the original RAVL edge-detection /
// edge-linking design by George Matas, Radek Marik and Charles Galambos.

#include "Ravl2/Image/EdgeDetector.hh"

#include <cmath>
#include <cstdint>
#include <array>
#include <deque>
#include <utility>

namespace Ravl2
{
  namespace
  {
    // Edge-pixel state (low 2 bits of the working byte image). The upper 6 bits hold, for a
    // 2-neighbour chain pixel, the forward (bits 2-4) and backward (bits 5-7) link directions.
    constexpr uint8_t kProc = 0;    // not an edge / already consumed
    constexpr uint8_t kUnproc = 1;  // candidate edge, not yet reached by a contour
    constexpr uint8_t kJunct = 2;   // junction or end point (chain terminator)
    constexpr uint8_t kInString = 3;// interior of a chain (has stored link directions)

    // 8-neighbour directions, ordered so reverse(d) == (d+4)&7.
    //  0:down 1:down-right 2:right 3:up-right 4:up 5:up-left 6:left 7:down-left
    constexpr std::array<int, 8> kOffRow = {+1, +1, 0, -1, -1, -1, 0, +1};
    constexpr std::array<int, 8> kOffCol = {0, +1, +1, +1, 0, -1, -1, -1};

    inline int offRow(int dir) { return kOffRow[static_cast<std::size_t>(dir)]; }
    inline int offCol(int dir) { return kOffCol[static_cast<std::size_t>(dir)]; }
  }// namespace

  std::vector<std::vector<Edgel>> detectEdgeChains(const Array<uint8_t, 2> &grey, const EdgeDetectorParams &params)
  {
    std::vector<std::vector<Edgel>> chains;
    const IndexRange<2> gradRange = grey.range().shrink(1);
    if(gradRange.empty()) { return chains; }

    // --- 1. Sobel gradient (dRow, dCol) + magnitude. ---
    Array<float, 2> dRow(gradRange), dCol(gradRange), mag(gradRange);
    for(int r : gradRange[0]) {
      for(int c : gradRange[1]) {
        const float gr = float(grey[r + 1][c - 1] + 2 * grey[r + 1][c] + grey[r + 1][c + 1]
                                - grey[r - 1][c - 1] - 2 * grey[r - 1][c] - grey[r - 1][c + 1]);
        const float gc = float(grey[r - 1][c + 1] + 2 * grey[r][c + 1] + grey[r + 1][c + 1]
                                - grey[r - 1][c - 1] - 2 * grey[r][c - 1] - grey[r + 1][c - 1]);
        dRow[r][c] = gr;
        dCol[r][c] = gc;
        mag[r][c] = std::hypot(gr, gc);
      }
    }

    // --- 2. Non-maximal suppression (octant linear-interpolation thinning). ---
    const IndexRange<2> inner = gradRange.shrink(1);
    Array<float, 2> nms(gradRange);
    for(int r : gradRange[0]) { for(int c : gradRange[1]) { nms[r][c] = 0.0f; } }
    for(int r : inner[0]) {
      for(int c : inner[1]) {
        float dr = dRow[r][c], dc = dCol[r][c];
        const float cen = mag[r][c];
        if(dr == 0.0f && dc == 0.0f) { continue; }
        if(dc < 0) { dc = -dc; dr = -dr; }// fold into octants 1-4 (dc >= 0)
        const float TL = mag[r - 1][c - 1], TM = mag[r - 1][c], TR = mag[r - 1][c + 1];
        const float ML = mag[r][c - 1], MR = mag[r][c + 1];
        const float BL = mag[r + 1][c - 1], BM = mag[r + 1][c], BR = mag[r + 1][c + 1];
        bool isMax = false;
        if(dr >= 0) {
          if(dr > dc) {// octant 1
            isMax = (BR - BM) * dc <= (cen - BM) * dr && (TL - TM) * dc < (cen - TM) * dr;
          } else {// octant 2
            isMax = (BR - MR) * dr < (cen - MR) * dc && (TL - ML) * dr <= (cen - ML) * dc;
          }
        } else {
          if(-dr < dc) {// octant 3
            isMax = (ML - BL) * dr <= (cen - ML) * dc && (MR - TR) * dr < (cen - MR) * dc;
          } else {// octant 4
            isMax = (BM - BL) * dc >= (cen - BM) * dr && (TM - TR) * dc > (cen - TM) * dr;
          }
        }
        if(isMax) { nms[r][c] = cen; }
      }
    }

    // --- 3. State image: candidates above the low threshold; border stays kProc. ---
    Array<uint8_t, 2> state(gradRange);
    for(int r : gradRange[0]) { for(int c : gradRange[1]) { state[r][c] = kProc; } }
    for(int r : inner[0]) {
      for(int c : inner[1]) { state[r][c] = (nms[r][c] > params.minThreshold) ? kUnproc : kProc; }
    }

    auto isEdge = [&](int r, int c) { return (state[r][c] & 3) != kProc; };

    // --- 4. Hysteresis: from each strong (> high) candidate, flood the connected candidate set,
    // marking chain interiors (exactly 2 edge neighbours, with their two link directions) and
    // junctions/ends (otherwise). 4-connected first; a diagonal only counts if neither orthogonal
    // neighbour it sits between is present (avoids staircase double-counting). ---
    auto labelContour = [&](int sr, int sc) {
      std::vector<std::pair<int, int>> stack;
      stack.emplace_back(sr, sc);
      state[sr][sc] = uint8_t((state[sr][sc] & ~3) | kInString);
      while(!stack.empty()) {
        const auto [r, c] = stack.back();
        stack.pop_back();
        std::array<int, 8> dirs{};
        std::size_t n = 0;
        const bool down = isEdge(r + 1, c), right = isEdge(r, c + 1), up = isEdge(r - 1, c), left = isEdge(r, c - 1);
        if(down) { dirs[n++] = 0; }
        if(right) { dirs[n++] = 2; }
        if(up) { dirs[n++] = 4; }
        if(left) { dirs[n++] = 6; }
        if(params.eightConnect) {
          if(!(down || right) && isEdge(r + 1, c + 1)) { dirs[n++] = 1; }
          if(!(right || up) && isEdge(r - 1, c + 1)) { dirs[n++] = 3; }
          if(!(up || left) && isEdge(r - 1, c - 1)) { dirs[n++] = 5; }
          if(!(left || down) && isEdge(r + 1, c - 1)) { dirs[n++] = 7; }
        }
        for(std::size_t k = 0; k < n; k++) {
          const int nr = r + offRow(dirs[k]), nc = c + offCol(dirs[k]);
          if((state[nr][nc] & 3) == kUnproc) {
            stack.emplace_back(nr, nc);
            state[nr][nc] = uint8_t((state[nr][nc] & ~3) | kInString);
          }
        }
        uint8_t &val = state[r][c];
        if(n != 2) {
          val = uint8_t((val & ~3) | kJunct);
        } else {// store the two link directions (state bits preserved)
          val = uint8_t(val | (uint8_t(dirs[0]) << 2) | (uint8_t(dirs[1]) << 5));
        }
      }
    };
    for(int r : inner[0]) {
      for(int c : inner[1]) {
        if(nms[r][c] > params.maxThreshold && (state[r][c] & 3) == kUnproc) { labelContour(r, c); }
      }
    }

    // --- 5. Walk each chain from an interior pixel, both directions, to its terminators. ---
    auto getDir = [](uint8_t v, int fb) { return (fb == 0) ? ((v >> 2) & 7) : ((v >> 5) & 7); };
    auto makeEdgel = [&](int r, int c) {
      Edgel e;
      e.at = Point<float, 2>(float(r), float(c));
      e.grad = Vector<float, 2>(dRow[r][c], dCol[r][c]);
      e.magnitude = mag[r][c];
      return e;
    };
    for(int sr : inner[0]) {
      for(int sc : inner[1]) {
        if((state[sr][sc] & 3) != kInString) { continue; }
        std::deque<std::pair<int, int>> chain;
        const uint8_t sval = state[sr][sc];
        state[sr][sc] = uint8_t((sval & ~3) | kProc);// consume (link dirs preserved)
        chain.emplace_back(sr, sc);
        for(int fb = 0; fb <= 1; fb++) {
          int dir = getDir(sval, fb);
          int nr = sr + offRow(dir), nc = sc + offCol(dir);
          while((state[nr][nc] & 3) == kInString) {
            uint8_t &pval = state[nr][nc];
            pval = uint8_t((pval & ~3) | kProc);
            if(fb == 0) { chain.emplace_back(nr, nc); } else { chain.emplace_front(nr, nc); }
            const int rev = (dir + 4) & 7;
            int nd = getDir(pval, 0);
            if(nd == rev) { nd = getDir(pval, 1); }
            dir = nd;
            nr += offRow(dir);
            nc += offCol(dir);
          }
          if((state[nr][nc] & 3) == kJunct) {// include the terminating junction as an endpoint
            if(fb == 0) { chain.emplace_back(nr, nc); } else { chain.emplace_front(nr, nc); }
          }
        }
        if(int(chain.size()) < params.minChainLength) { continue; }
        std::vector<Edgel> out;
        out.reserve(chain.size());
        for(const auto &[r, c] : chain) { out.push_back(makeEdgel(r, c)); }
        chains.push_back(std::move(out));
      }
    }
    return chains;
  }

}// namespace Ravl2
