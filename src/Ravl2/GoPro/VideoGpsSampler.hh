//
// Pairing GPMF GPS fixes with video frames, and sampling a video at a fixed rate with the GPS
// interpolated to each sample time.
//
// GpmfUtilities.hh already turns a GPS track into local coordinates, measures distances along it
// and integrates the gyro. What was missing is the join between that track and the pictures: which
// frame was I looking at when this fix was recorded, and where was I when this frame was shown.
//
// Moved here from an experiment tree on 2026-08-17, dropping the model-manifest loading it used to
// carry, which belonged to a retired training pipeline rather than to video/GPS.
//

#pragma once

#include <array>
#include <deque>
#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

#include <spdlog/spdlog.h>

#include "Ravl2/GoPro/GpmfTypes.hh"
#include "Ravl2/Video/MediaContainer.hh"
#include "Ravl2/Video/StreamIterator.hh"

namespace Ravl2::GoPro
{
  //! @brief A GPS fix with the container-timeline time it was recorded at, in seconds.
  struct GpsPoint {
    double tSec {};
    GpsFix fix {};
  };

  //! @brief A video frame reference with its container-timeline time, in seconds.
  struct FramePoint {
    double tSec {};
    std::size_t frameIndex {};
  };

  //! @brief One GPS fix matched to its nearest video frame.
  struct MatchPair {
    std::size_t gpsIdx {};
    std::size_t frameIdx {};
    double dt {};//!< Absolute time difference, seconds.
  };

  //! @brief Thin out a GPS track to approximately @p hz, keeping the fix closest to each window
  //! centre rather than the first in the window.
  //! @param in Time-ordered fixes.
  //! @param hz Target rate; <= 0 or an empty input returns @p in unchanged.
  [[nodiscard]] std::vector<GpsPoint> subsampleGpsByHz(const std::vector<GpsPoint> &in, double hz);

  //! @brief Match every GPS fix to the video frame closest to it in time.
  //! @param gps Time-ordered fixes.
  //! @param frames Time-ordered frame references.
  //! @param maxDtSec If set, drop matches further apart than this instead of accepting them.
  //! @return One entry per accepted match; empty if either input is empty.
  [[nodiscard]] std::vector<MatchPair> matchNearestFrames(const std::vector<GpsPoint> &gps,
                                                          const std::vector<FramePoint> &frames,
                                                          const std::optional<double> &maxDtSec);

  //! @brief Walk a media file at a fixed rate, handing back each frame with the GPS position
  //! interpolated to that frame's time.
  //!
  //! Sampling is a single sequential forward pass, not a seek per sample: random seeks are
  //! expensive on HEVC because of the B-frame GOP structure, and a forward pass keeps the decoder
  //! cache warm. It falls back to seek-per-sample only if the initial rewind fails.
  //!
  //! Video and GPS are read through two separate containers on purpose. They share no format
  //! context, so advancing one cannot leave the other stranded at EOF.
  //!
  //! @warning **Times are per-container, and a GoPro recording split into chapters does not restart
  //! its clock.** The GPMF STMP timeline is cumulative across chapters (chapter 2 might start at
  //! 2114 s) while frame *numbering* restarts at 0 in each file. Anything that concatenates
  //! chapters and rebuilds a time axis from the frame index — `t = i * stride / fps` — puts every
  //! query before the second chapter's own GPS range. Interpolation then clamps, and the whole
  //! chapter collapses to one repeated fix: a plausible-looking track, no error raised. Build the
  //! axis from the chapter's own first timestamp instead. This class is correct for one file; the
  //! trap is in the caller that joins them.
  //!
  //! @note Not thread-safe: sampling advances iterators and a GPS buffer held in the object.
  class VideoGpsSampler
  {
  public:
    //! @brief One sample handed to the callback.
    struct Sample {
      double tSec {};                                        //!< Requested sample time, seconds.
      std::shared_ptr<Video::Frame> frame;                   //!< Decoded video frame.
      std::optional<GpsFix> gps;                             //!< Interpolated fix, if GPS is present.
      std::size_t frameIndex = static_cast<std::size_t>(-1); //!< Frame index, if known.
    };

    //! @brief Construct for a media file. The container is opened lazily.
    explicit VideoGpsSampler(std::filesystem::path mediaPath);

    //! @brief Open the container and read its duration.
    //! @return False if the file cannot be opened or holds no video stream.
    bool open();

    //! @brief Sample at a fixed rate, invoking @p callback per sample; return false from it to stop.
    //! @return Number of samples handed to the callback.
    std::size_t sampleByHz(double hz, const std::function<bool(const Sample &)> &callback);

    //! @brief Sample every @p stepSeconds of media time. @see sampleByHz
    std::size_t sampleByStep(double stepSeconds, const std::function<bool(const Sample &)> &callback);

    //! @brief Interpolated GPS at an arbitrary media time, seconds.
    //! @note Advances the internal GPS iterator and keeps a bounded buffer, so calls should move
    //! forward in time; going backwards re-seeks.
    [[nodiscard]] std::optional<GpsFix> gpsAt(double tSec) { return interpolateGps(tSec); }

    //! @brief Every GPS point seen so far. Partial until sampling completes.
    [[nodiscard]] const std::vector<GpsPoint> &gpsTimeline() const noexcept { return mGpsAll; }

    //! @brief Media duration in seconds; 0 until open() succeeds.
    [[nodiscard]] double durationSec() const noexcept { return mDurationSec; }

    //! @brief Seconds of GPS kept either side of the current time for interpolation.
    void setGpsBufferWindow(double seconds) noexcept { mGpsBufferWindowSec = seconds; }

  private:
    std::optional<GpsFix> interpolateGps(double tSec);
    bool ensureOpened();
    bool ensureVideoIterator();
    bool ensureGpsIterator();
    bool decodeFrameAt(double tSec, std::shared_ptr<Video::Frame> &outFrame, std::size_t &outIndex);
    void trimGpsBufferAround(double tSec);
    void fillGpsBufferUntil(double tSec);

    std::filesystem::path mPath;
    std::shared_ptr<Video::MediaContainer> mContainer;
    std::shared_ptr<Video::StreamIterator> mVideoIter;
    std::size_t mVideoStreamIndex = static_cast<std::size_t>(-1);
    //! GPS reads through its own container so the two iterators share no format context.
    std::shared_ptr<Video::MediaContainer> mGpsContainer;
    std::shared_ptr<Video::StreamIterator> mGpsIter;
    bool mGpsSearchDone = false;//!< True once the GPS probe has run, so it is not retried per frame.
    std::deque<GpsPoint> mGpsBuf;  //!< Recent fixes for interpolation, time-ordered.
    std::vector<GpsPoint> mGpsAll; //!< Everything seen, for an optional track output.
    std::size_t mGpsStreamIndex = static_cast<std::size_t>(-1);
    double mGpsBufferWindowSec = 60.0;
    double mDurationSec = 0.0;
  };

}// namespace Ravl2::GoPro
