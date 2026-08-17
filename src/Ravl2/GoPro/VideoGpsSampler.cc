//
// Implementation of the GPMF GPS / video-frame join. See VideoGpsSampler.hh.
//

#include "Ravl2/GoPro/VideoGpsSampler.hh"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "Ravl2/Video/FfmpegMediaContainer.hh"

namespace Ravl2::GoPro
{

  std::vector<GpsPoint> subsampleGpsByHz(const std::vector<GpsPoint> &in, double hz)
  {
    if(hz <= 0.0 || in.empty())
      return in;
    const double window = 1.0 / hz;
    std::vector<GpsPoint> out;
    double nextWindowStart = in.front().tSec;
    double nextWindowEnd = nextWindowStart + window;
    std::optional<std::size_t> bestIdx;
    auto flush = [&]() {
      if(bestIdx) {
        out.push_back(in[*bestIdx]);
        bestIdx.reset();
      }
    };
    for(std::size_t i = 0; i < in.size(); ++i) {
      const double t = in[i].tSec;
      while(t >= nextWindowEnd) {
        flush();
        nextWindowStart = nextWindowEnd;
        nextWindowEnd = nextWindowStart + window;
      }
      const double center = nextWindowStart + window * 0.5;
      if(!bestIdx) {
        bestIdx = i;
      } else {
        double db = std::abs(in[*bestIdx].tSec - center);
        double di = std::abs(t - center);
        if(di < db)
          bestIdx = i;
      }
    }
    flush();
    return out;
  }

  std::vector<MatchPair> matchNearestFrames(const std::vector<GpsPoint> &gps,
                                            const std::vector<FramePoint> &frames,
                                            const std::optional<double> &maxDtSec)
  {
    std::vector<MatchPair> out;
    if(gps.empty() || frames.empty())
      return out;
    std::vector<double> ftimes(frames.size());
    for(std::size_t i = 0; i < frames.size(); ++i)
      ftimes[i] = frames[i].tSec;
    for(std::size_t gi = 0; gi < gps.size(); ++gi) {
      double gt = gps[gi].tSec;
      auto it = std::lower_bound(ftimes.begin(), ftimes.end(), gt);
      std::size_t best = 0;
      if(it == ftimes.begin()) {
        best = 0;
      } else if(it == ftimes.end()) {
        best = ftimes.size() - 1;
      } else {
        std::size_t j = static_cast<std::size_t>(it - ftimes.begin());
        std::size_t jm1 = j - 1;
        best = (std::abs(ftimes[j] - gt) < std::abs(ftimes[jm1] - gt)) ? j : jm1;
      }
      double dt = std::abs(ftimes[best] - gt);
      if(maxDtSec && dt > *maxDtSec)
        continue;
      out.push_back(MatchPair {gi, frames[best].frameIndex, dt});
    }
    return out;
  }

  VideoGpsSampler::VideoGpsSampler(std::filesystem::path mediaPath)
      : mPath(std::move(mediaPath))
  {}

  bool VideoGpsSampler::ensureOpened()
  {
    if(mContainer)
      return true;
    auto openRes = Video::FfmpegMediaContainer::openFile(mPath.string());
    if(!openRes.isSuccess()) {
      SPDLOG_ERROR("Failed to open media: {}", mPath.string());
      return false;
    }
    mContainer = openRes.value();
    return true;
  }

  bool VideoGpsSampler::ensureVideoIterator()
  {
    if(mVideoIter)
      return true;
    if(!ensureOpened())
      return false;
    for(std::size_t si = 0; si < mContainer->streamCount(); ++si) {
      if(mContainer->streamType(si) == Video::StreamType::Video) {
        mVideoStreamIndex = si;
        break;
      }
    }
    if(mVideoStreamIndex == static_cast<std::size_t>(-1)) {
      SPDLOG_ERROR("No video stream in: {}", mPath.string());
      return false;
    }
    auto vres = mContainer->createIterator(mVideoStreamIndex);
    if(!vres.isSuccess()) {
      SPDLOG_ERROR("Failed to create video iterator for: {}", mPath.string());
      return false;
    }
    mVideoIter = vres.value();
    return true;
  }

  bool VideoGpsSampler::ensureGpsIterator()
  {
    if(mGpsIter)
      return true;
    if(mGpsSearchDone)
      return false;
    mGpsSearchDone = true;
    auto openRes = Video::FfmpegMediaContainer::openFile(mPath.string());
    if(!openRes.isSuccess()) {
      SPDLOG_WARN("No GPS metadata stream found in: {}", mPath.string());
      return false;
    }
    mGpsContainer = openRes.value();
    // GoPro GPMF data streams carry codec_tag 'gpmd'. Other data tracks (e.g. 'tmcd' timecode) must
    // be skipped BEFORE an iterator is made for them: a non-GPMF iterator reads the whole file
    // looking for its stream and leaves the shared format context at EOF, starving later iterators.
    constexpr uint32_t kGpmdTag =
      (static_cast<uint32_t>('g') << 0) | (static_cast<uint32_t>('p') << 8) | (static_cast<uint32_t>('m') << 16) | (static_cast<uint32_t>('d') << 24);
    auto *ffmpegContainer = dynamic_cast<Video::FfmpegMediaContainer *>(mGpsContainer.get());
    for(std::size_t si = 0; si < mGpsContainer->streamCount(); ++si) {
      if(mGpsContainer->streamType(si) != Video::StreamType::Data)
        continue;
      if(ffmpegContainer && ffmpegContainer->streamCodecTag(si) != kGpmdTag) {
        SPDLOG_DEBUG("VideoGpsSampler: skipping non-GPMD data stream {} (tag=0x{:08x})", si,
                     ffmpegContainer->streamCodecTag(si));
        continue;
      }
      auto ires = mGpsContainer->createIterator(si);
      if(!ires.isSuccess())
        continue;
      auto it = ires.value();
      for(int k = 0; k < 64 && !it->isAtEnd(); ++k) {
        auto f = it->currentFrame();
        if(!f)
          break;
        if(dynamic_cast<Video::FrameData<GpsFix> *>(f.get()) != nullptr) {
          mGpsIter = it;
          mGpsStreamIndex = si;
          return true;
        }
        auto nr = it->next();
        if(!nr.isSuccess())
          break;
      }
    }
    // Fallback: a combined iterator over every stream.
    auto allRes = mGpsContainer->createIterator(std::vector<std::size_t> {});
    if(allRes.isSuccess()) {
      auto it = allRes.value();
      for(int k = 0; k < 256 && !it->isAtEnd(); ++k) {
        auto f = it->currentFrame();
        if(!f)
          break;
        if(dynamic_cast<Video::FrameData<GpsFix> *>(f.get()) != nullptr) {
          mGpsIter = it;
          mGpsStreamIndex = static_cast<std::size_t>(-1);
          return true;
        }
        auto nr = it->next();
        if(!nr.isSuccess())
          break;
      }
    }
    SPDLOG_WARN("No GPS metadata stream found in: {}", mPath.string());
    return false;
  }

  bool VideoGpsSampler::open()
  {
    if(!ensureOpened())
      return false;
    mDurationSec = std::chrono::duration<double>(mContainer->duration()).count();
    if(!ensureVideoIterator())
      return false;
    // The GPS iterator is optional and made on the first gpsAt() call.
    return true;
  }

  void VideoGpsSampler::trimGpsBufferAround(double tSec)
  {
    while(!mGpsBuf.empty() && (mGpsBuf.front().tSec < (tSec - mGpsBufferWindowSec))) {
      mGpsBuf.pop_front();
    }
  }

  void VideoGpsSampler::fillGpsBufferUntil(double tSec)
  {
    if(!mGpsIter)
      return;
    using namespace std::chrono;
    // Starting fresh, seek near the target rather than scanning from the beginning.
    if(mGpsBuf.empty()) {
      auto mt = duration_cast<Video::MediaTime>(duration<double>(tSec));
      mGpsIter->seek(mt, Video::SeekFlags::Precise);
    }
    const double lookAhead = 0.5;// seconds past the target, so the upper sample is present
    const double target = tSec + lookAhead;
    int guard = 0;
    while(guard++ < 20000 && !mGpsIter->isAtEnd()) {
      auto f = mGpsIter->currentFrame();
      if(!f)
        break;
      double fTime = std::chrono::duration<double>(f->timestamp()).count();
      if(auto *gpsF = dynamic_cast<Video::FrameData<GpsFix> *>(f.get())) {
        if(mGpsBuf.empty() || fTime > mGpsBuf.back().tSec) {
          GpsPoint gp {fTime, gpsF->data()};
          mGpsBuf.push_back(gp);
          mGpsAll.push_back(gp);
        }
        if(fTime >= target)
          break;
      }
      auto nr = mGpsIter->next();
      if(!nr.isSuccess())
        break;
    }
  }

  std::optional<GpsFix> VideoGpsSampler::interpolateGps(double tSec)
  {
    if(!mGpsIter) {
      if(!ensureGpsIterator())
        return std::nullopt;
    }
    trimGpsBufferAround(tSec);
    fillGpsBufferUntil(tSec);
    if(mGpsBuf.empty())
      return std::nullopt;
    auto cmp = [](const GpsPoint &a, double t) { return a.tSec < t; };
    auto it = std::lower_bound(mGpsBuf.begin(), mGpsBuf.end(), tSec, cmp);
    if(it == mGpsBuf.begin())
      return mGpsBuf.front().fix;
    if(it == mGpsBuf.end())
      return mGpsBuf.back().fix;
    const GpsPoint &b = *it;
    const GpsPoint &a = *(it - 1);
    double dt = b.tSec - a.tSec;
    if(dt <= 0.0)
      return a.fix;
    double alpha = (tSec - a.tSec) / dt;
    GpsFix out = a.fix;
    auto flerp = [](float x, float y, double f) {
      const double xd = static_cast<double>(x);
      const double yd = static_cast<double>(y);
      return static_cast<float>(xd + (yd - xd) * f);
    };
    out.location = GPSCoordinate::bilinearInterpolate(static_cast<GPSCoordinate::RealT>(alpha),
                                                      a.fix.location, b.fix.location);
    out.speed[0] = flerp(a.fix.speed2d(), b.fix.speed2d(), alpha);
    out.speed[1] = flerp(a.fix.speed3d(), b.fix.speed3d(), alpha);
    // Quality fields are categorical -- take them from the nearer fix rather than blending.
    if((tSec - a.tSec) < (b.tSec - tSec)) {
      out.fix = a.fix.fix;
      out.satellites = a.fix.satellites;
      out.precision = a.fix.precision;
    } else {
      out.fix = b.fix.fix;
      out.satellites = b.fix.satellites;
      out.precision = b.fix.precision;
    }
    return out;
  }

  bool VideoGpsSampler::decodeFrameAt(double tSec, std::shared_ptr<Video::Frame> &outFrame,
                                      std::size_t &outIndex)
  {
    using namespace std::chrono;
    Video::MediaTime mt = duration_cast<Video::MediaTime>(duration<double>(tSec));
    if(!ensureVideoIterator())
      return false;
    auto sr = mVideoIter->seek(mt, Video::SeekFlags::Precise);
    if(!sr.isSuccess())
      return false;
    // Step forward until a video frame appears; defensive against interleaved non-video frames.
    for(int i = 0; i < 5; ++i) {
      auto f = mVideoIter->currentFrame();
      if(!f)
        return false;
      if(f->streamType() == Video::StreamType::Video) {
        outFrame = std::move(f);
        auto idx = mVideoIter->positionIndex();
        outIndex = idx >= 0 ? static_cast<std::size_t>(idx) : static_cast<std::size_t>(-1);
        return true;
      }
      auto nr = mVideoIter->next();
      if(!nr.isSuccess())
        break;
    }
    return false;
  }

  std::size_t VideoGpsSampler::sampleByHz(double hz, const std::function<bool(const Sample &)> &callback)
  {
    if(hz <= 0.0)
      return 0;
    return sampleByStep(1.0 / hz, callback);
  }

  std::size_t VideoGpsSampler::sampleByStep(double stepSeconds,
                                            const std::function<bool(const Sample &)> &callback)
  {
    if(stepSeconds <= 0.0)
      return 0;
    if(!open())
      return 0;
    using namespace std::chrono;
    if(!ensureVideoIterator())
      return 0;

    auto rewindRes = mVideoIter->seek(Video::MediaTime {0}, Video::SeekFlags::Precise);
    if(!rewindRes.isSuccess()) {
      SPDLOG_WARN("sampleByStep: could not seek to start, falling back to per-sample seek");
      std::size_t count = 0;
      for(double t = 0.0; t <= mDurationSec; t += stepSeconds) {
        Sample s;
        s.tSec = t;
        std::size_t fidx = static_cast<std::size_t>(-1);
        if(!decodeFrameAt(t, s.frame, fidx))
          continue;
        s.frameIndex = fidx;
        s.gps = gpsAt(t);
        ++count;
        if(!callback(s))
          break;
      }
      return count;
    }

    // Sequential forward pass: emit a sample whenever a frame timestamp crosses the next boundary.
    std::size_t count = 0;
    double nextSampleT = 0.0;
    while(!mVideoIter->isAtEnd() && nextSampleT <= mDurationSec) {
      auto f = mVideoIter->currentFrame();
      if(!f) {
        mVideoIter->next();
        continue;
      }
      if(f->streamType() != Video::StreamType::Video) {
        mVideoIter->next();
        continue;
      }
      double fTime = duration<double>(f->timestamp()).count();
      if(fTime >= nextSampleT) {
        Sample s;
        s.tSec = nextSampleT;
        s.frame = f;
        auto idx = mVideoIter->positionIndex();
        s.frameIndex = idx >= 0 ? static_cast<std::size_t>(idx) : static_cast<std::size_t>(-1);
        s.gps = gpsAt(nextSampleT);
        ++count;
        nextSampleT += stepSeconds;
        if(!callback(s))
          break;
      }
      auto nr = mVideoIter->next();
      if(!nr.isSuccess())
        break;
    }
    return count;
  }

}// namespace Ravl2::GoPro
