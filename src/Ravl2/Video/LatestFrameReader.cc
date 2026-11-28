//
// Read the newest frame from a live stream, dropping any the consumer was too slow to take.
//

#include "Ravl2/Video/LatestFrameReader.hh"

#include <spdlog/spdlog.h>

namespace Ravl2::Video
{
  LatestFrameReader::LatestFrameReader(std::shared_ptr<StreamIterator> iterator)
      : mIterator(std::move(iterator)),
        mThread([this](const std::stop_token &stop) { run(stop); })
  {}

  LatestFrameReader::~LatestFrameReader()
  {
    mThread.request_stop();
    mChanged.notify_all();
    // ~jthread joins.
  }

  void LatestFrameReader::run(const std::stop_token &stop)
  {
    if(!mIterator) {
      std::lock_guard lock(mMutex);
      mEnded = true;
      mChanged.notify_all();
      return;
    }
    while(!stop.stop_requested()) {
      std::shared_ptr<Frame> frame = mIterator->currentFrame();
      if(frame) {
        std::lock_guard lock(mMutex);
        if(mNewest)
          ++mDropped;
        mNewest = std::move(frame);
        mChanged.notify_all();
      }
      auto result = mIterator->next();
      if(!result.isSuccess()) {
        if(result.error() != VideoErrorCode::EndOfStream)
          SPDLOG_WARN("LatestFrameReader: stream ended with error {}", static_cast<int>(result.error()));
        break;
      }
    }
    std::lock_guard lock(mMutex);
    mEnded = true;
    mChanged.notify_all();
  }

  std::shared_ptr<Frame> LatestFrameReader::next()
  {
    std::unique_lock lock(mMutex);
    mChanged.wait(lock, [this] { return mNewest != nullptr || mEnded; });
    return std::exchange(mNewest, nullptr);
  }

  uint64_t LatestFrameReader::droppedFrames() const
  {
    std::lock_guard lock(mMutex);
    return mDropped;
  }

  bool LatestFrameReader::atEnd() const
  {
    std::lock_guard lock(mMutex);
    return mEnded && mNewest == nullptr;
  }

}// namespace Ravl2::Video
