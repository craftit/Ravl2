//
// The GPS/frame join in VideoGpsSampler.hh. Sampling a real file needs media, but the two pairing
// functions are pure and are where an off-by-one silently produces a plausible track, so they are
// what is worth pinning here.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/GoPro/VideoGpsSampler.hh"

namespace Ravl2
{
  using GoPro::FramePoint;
  using GoPro::GpsPoint;

  namespace
  {
    //! Fixes at a fixed rate; the fix payload is irrelevant to the timing logic under test.
    std::vector<GpsPoint> track(double t0, double step, std::size_t n)
    {
      std::vector<GpsPoint> v;
      v.reserve(n);
      for(std::size_t i = 0; i < n; ++i)
        v.push_back(GpsPoint {t0 + step * double(i), {}});
      return v;
    }
  }// namespace

  TEST_CASE("subsampleGpsByHz")
  {
    // 100 fixes at 10 Hz -> 10 s of track. Asking for 1 Hz should leave ~one per second.
    auto in = track(0.0, 0.1, 100);
    auto out = GoPro::subsampleGpsByHz(in, 1.0);
    CHECK(out.size() >= 9);
    CHECK(out.size() <= 11);

    // Output stays time-ordered and is a subset of the input times.
    for(std::size_t i = 1; i < out.size(); ++i)
      CHECK(out[i].tSec > out[i - 1].tSec);

    // It keeps the fix nearest each window CENTRE, not the first in the window -- the distinction
    // that makes a thinned track sit on the original path rather than lag it.
    CHECK(std::abs(out.front().tSec - 0.5) < 0.11);

    // Degenerate inputs pass through rather than throwing or emptying.
    CHECK(GoPro::subsampleGpsByHz(in, 0.0).size() == in.size());
    CHECK(GoPro::subsampleGpsByHz({}, 1.0).empty());
  }

  TEST_CASE("matchNearestFrames")
  {
    // Frames at 1 Hz, fixes offset by 0.4 s: every fix has a frame 0.4 s away.
    std::vector<FramePoint> frames;
    for(std::size_t i = 0; i < 10; ++i)
      frames.push_back(FramePoint {double(i), i});
    auto gps = track(0.4, 1.0, 10);

    auto m = GoPro::matchNearestFrames(gps, frames, std::nullopt);
    REQUIRE(m.size() == gps.size());
    for(std::size_t i = 0; i < m.size(); ++i) {
      CHECK(m[i].gpsIdx == i);
      CHECK(m[i].frameIdx == i);// 0.4 s ahead is nearer than 0.6 s behind
      CHECK(std::abs(m[i].dt - 0.4) < 1e-9);
    }

    // maxDtSec DROPS a match rather than accepting a distant one -- a fix with no frame near it is
    // absent from the output, not silently paired with whatever was closest.
    CHECK(GoPro::matchNearestFrames(gps, frames, 0.2).empty());
    CHECK(GoPro::matchNearestFrames(gps, frames, 0.5).size() == gps.size());

    // The last fix falls past the last frame; it must clamp to it, not run off the end.
    auto late = track(20.0, 1.0, 1);
    auto lm = GoPro::matchNearestFrames(late, frames, std::nullopt);
    REQUIRE(lm.size() == 1);
    CHECK(lm[0].frameIdx == frames.back().frameIndex);

    CHECK(GoPro::matchNearestFrames({}, frames, std::nullopt).empty());
    CHECK(GoPro::matchNearestFrames(gps, {}, std::nullopt).empty());
  }

}// namespace Ravl2
