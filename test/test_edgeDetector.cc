// Tests for the Ravl2 edge detector / edge-linking reimplementation.

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "Ravl2/Image/EdgeDetector.hh"

namespace Ravl2
{
  // A single straight step edge should produce one dominant chain whose edgels lie along the edge
  // (here a vertical edge at col 30: left half dark, right half bright).
  TEST_CASE("EdgeChainStraightEdge")
  {
    const int H = 60, W = 60, edgeCol = 30;
    Array<uint8_t, 2> img(IndexRange<2>({{0, H - 1}, {0, W - 1}}));
    for(int r = 0; r < H; ++r) {
      for(int c = 0; c < W; ++c) { img[r][c] = uint8_t(c < edgeCol ? 30 : 220); }
    }
    const auto chains = detectEdgeChains(img);
    REQUIRE(!chains.empty());

    // The longest chain should span most of the image height and sit on the edge column.
    std::size_t best = 0;
    for(std::size_t i = 1; i < chains.size(); ++i) {
      if(chains[i].size() > chains[best].size()) { best = i; }
    }
    const auto &chain = chains[best];
    CHECK(chain.size() >= std::size_t(H / 2));
    float meanCol = 0, minRow = 1e9f, maxRow = -1e9f;
    for(const auto &e : chain) {
      meanCol += e.at[1];
      minRow = std::min(minRow, e.at[0]);
      maxRow = std::max(maxRow, e.at[0]);
    }
    meanCol /= float(chain.size());
    CHECK(std::abs(meanCol - float(edgeCol)) < 2.0f);// edgels sit on the edge (NMS puts it within ~1px)
    CHECK((maxRow - minRow) >= float(H / 2));         // and span the height (continuity)
  }

  // A blank image has no edges → no chains.
  TEST_CASE("EdgeChainBlankImage")
  {
    Array<uint8_t, 2> img(IndexRange<2>({{0, 39}, {0, 39}}));
    for(int r = 0; r < 40; ++r) { for(int c = 0; c < 40; ++c) { img[r][c] = uint8_t(120); } }
    const auto chains = detectEdgeChains(img);
    CHECK(chains.empty());
  }

  // A bright square's border should yield chains that together trace its four sides; total edgels
  // should be comparable to the perimeter.
  TEST_CASE("EdgeChainSquareBorder")
  {
    const int H = 80, W = 80;
    Array<uint8_t, 2> img(IndexRange<2>({{0, H - 1}, {0, W - 1}}));
    for(int r = 0; r < H; ++r) {
      for(int c = 0; c < W; ++c) {
        const bool inside = (r >= 20 && r < 60 && c >= 20 && c < 60);
        img[r][c] = uint8_t(inside ? 220 : 30);
      }
    }
    const auto chains = detectEdgeChains(img);
    REQUIRE(!chains.empty());
    std::size_t total = 0;
    for(const auto &ch : chains) { total += ch.size(); }
    CHECK(total >= std::size_t(4 * 40 - 16));// ~ perimeter of the 40x40 square (allow corner loss)
  }

}// namespace Ravl2
