//
// Read the newest frame from a live stream, dropping any the consumer was too slow to take.
//

#pragma once

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

#include "Ravl2/Video/Frame.hh"
#include "Ravl2/Video/StreamIterator.hh"

namespace Ravl2::Video
{
  //! Reads a stream on a background thread and keeps only the newest frame.
  //!
  //! For live capture, when processing is slower than the camera: next() returns the most recent frame rather
  //! than a backlog. Frames that arrive while an earlier one is still unread replace it and are counted as
  //! dropped. Each frame keeps the timestamp its iterator gave it (for FFmpeg V4L2 capture, the driver's capture
  //! time, in microseconds of wall-clock time).
  //!
  //! Thread safety: next(), droppedFrames() and atEnd() may be called from any thread. The iterator is used
  //! only by the background thread from construction until destruction; don't use it elsewhere meanwhile.
  //! Not suitable for audio, where every chunk matters.
  class LatestFrameReader
  {
  public:
    //! Start reading. 'iterator' should be positioned on its first frame (as createIterator() leaves it).
    explicit LatestFrameReader(std::shared_ptr<StreamIterator> iterator);

    //! Stops the background thread. It may wait for the iterator's current read (at most one frame period for
    //! a live device).
    ~LatestFrameReader();

    LatestFrameReader(const LatestFrameReader &) = delete;
    LatestFrameReader &operator=(const LatestFrameReader &) = delete;

    //! Wait for a frame newer than the last one returned.
    //! @return the frame, or null once the stream has ended (or failed) and no unread frame remains.
    [[nodiscard]] std::shared_ptr<Frame> next();

    //! Frames replaced before they were read.
    [[nodiscard]] uint64_t droppedFrames() const;

    //! True once the stream has ended and the last frame has been read.
    [[nodiscard]] bool atEnd() const;

  private:
    void run(const std::stop_token &stop);

    std::shared_ptr<StreamIterator> mIterator;
    mutable std::mutex mMutex;
    std::condition_variable_any mChanged;
    std::shared_ptr<Frame> mNewest;//!< Unread frame, if any.
    bool mEnded = false;
    uint64_t mDropped = 0;
    std::jthread mThread;//!< Declared last: started after the members it uses, stopped first.
  };

}// namespace Ravl2::Video
