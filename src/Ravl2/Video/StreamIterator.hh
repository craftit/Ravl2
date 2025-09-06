//
// Created on September 6, 2025
//

#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <optional>
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/Frame.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/Video/MetaDataFrame.hh"

namespace Ravl2 {
namespace Video {

//! Forward declaration
class MediaContainer;

//! Class representing an iterator for a specific stream in a media container
class StreamIterator {
public:
  //! Virtual destructor
  virtual ~StreamIterator() = default;

  //! Get the stream index this iterator is associated with
  virtual int streamIndex() const = 0;

  //! Get the stream type
  virtual StreamType streamType() const = 0;

  //! Get the current position in the stream (as a timestamp)
  virtual MediaTime position() const = 0;

  //! Get the current position as a frame/chunk index
  virtual int64_t positionIndex() const = 0;

  //! Check if the iterator is at the end of the stream
  virtual bool isAtEnd() const = 0;

  //! Move to the next frame/chunk
  virtual VideoResult<void> next() = 0;

  //! Move to the previous frame/chunk
  virtual VideoResult<void> previous() = 0;

  //! Seek to a specific timestamp
  virtual VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) = 0;

  //! Seek to a specific frame/chunk index
  virtual VideoResult<void> seekToIndex(int64_t index) = 0;

  //! Get the current frame (generic interface for all frame types)
  virtual VideoResult<std::shared_ptr<Frame>> currentFrame() const = 0;

  //! Get a specific frame by its unique ID
  virtual VideoResult<std::shared_ptr<Frame>> getFrameById(StreamItemId id) const = 0;

  //! Reset the iterator to the beginning of the stream
  virtual VideoResult<void> reset() = 0;

  //! Get current progress through the stream (0.0 to 1.0)
  virtual float progress() const = 0;

  //! Get the parent container
  virtual std::shared_ptr<MediaContainer> container() const = 0;

  //! Get the total duration of the stream
  virtual MediaTime duration() const = 0;

  //! Type-specific convenience methods

  //! Get the current frame as a video frame (only valid for video streams)
  VideoResult<std::shared_ptr<VideoFrame>> currentVideoFrame() const {
    if (streamType() != StreamType::Video) {
      return VideoResult<std::shared_ptr<VideoFrame>>(VideoErrorCode::InvalidOperation);
    }

    auto frameResult = currentFrame();
    if (!frameResult.isSuccess()) {
      return VideoResult<std::shared_ptr<VideoFrame>>(frameResult.error());
    }

    auto videoFrame = std::dynamic_pointer_cast<VideoFrame>(frameResult.value());
    if (!videoFrame) {
      return VideoResult<std::shared_ptr<VideoFrame>>(VideoErrorCode::InvalidOperation);
    }

    return VideoResult<std::shared_ptr<VideoFrame>>(videoFrame);
  }

  //! Get the current frame as an audio chunk (only valid for audio streams)
  VideoResult<std::shared_ptr<AudioChunk>> currentAudioChunk() const {
    if (streamType() != StreamType::Audio) {
      return VideoResult<std::shared_ptr<AudioChunk>>(VideoErrorCode::InvalidOperation);
    }

    auto frameResult = currentFrame();
    if (!frameResult.isSuccess()) {
      return VideoResult<std::shared_ptr<AudioChunk>>(frameResult.error());
    }

    auto audioChunk = std::dynamic_pointer_cast<AudioChunk>(frameResult.value());
    if (!audioChunk) {
      return VideoResult<std::shared_ptr<AudioChunk>>(VideoErrorCode::InvalidOperation);
    }

    return VideoResult<std::shared_ptr<AudioChunk>>(audioChunk);
  }

  //! Get the current frame as a metadata frame (only valid for data streams)
  VideoResult<std::shared_ptr<MetaDataFrame>> currentMetaDataFrame() const {
    if (streamType() != StreamType::Data) {
      return VideoResult<std::shared_ptr<MetaDataFrame>>(VideoErrorCode::InvalidOperation);
    }

    auto frameResult = currentFrame();
    if (!frameResult.isSuccess()) {
      return VideoResult<std::shared_ptr<MetaDataFrame>>(frameResult.error());
    }

    auto metaDataFrame = std::dynamic_pointer_cast<MetaDataFrame>(frameResult.value());
    if (!metaDataFrame) {
      return VideoResult<std::shared_ptr<MetaDataFrame>>(VideoErrorCode::InvalidOperation);
    }

    return VideoResult<std::shared_ptr<MetaDataFrame>>(metaDataFrame);
  }

  //! Configuration options

  //! Enable or disable frame prefetching for sequential access optimization
  virtual void setPrefetchEnabled(bool enable) = 0;

  //! Set the prefetch size (number of frames/chunks to prefetch)
  virtual void setPrefetchSize(int size) = 0;

  //! Check if prefetching is enabled
  virtual bool isPrefetchEnabled() const = 0;

  //! Get the prefetch size
  virtual int prefetchSize() const = 0;

  //! Check if the iterator can seek
  virtual bool canSeek() const = 0;

  //! Set a callback to be called when a new frame/chunk is reached
  virtual void setProgressCallback(std::function<void(MediaTime, float)> callback) = 0;
};

//! Helper class for simpler type-safe iteration over video frames
class VideoStreamIterator {
public:
  //! Constructor taking a StreamIterator
  explicit VideoStreamIterator(std::shared_ptr<StreamIterator> iterator)
    : m_iterator(std::move(iterator)) {
    if (m_iterator->streamType() != StreamType::Video) {
      throw std::runtime_error("StreamIterator is not for a video stream");
    }
  }

  //! Move to the next frame
  VideoResult<void> next() {
    return m_iterator->next();
  }

  //! Move to the previous frame
  VideoResult<void> previous() {
    return m_iterator->previous();
  }

  //! Get the current frame
  VideoResult<std::shared_ptr<VideoFrame>> currentFrame() const {
    return m_iterator->currentVideoFrame();
  }

  //! Seek to a specific timestamp
  VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) {
    return m_iterator->seek(timestamp, flags);
  }

  //! Get the underlying iterator
  std::shared_ptr<StreamIterator> iterator() const {
    return m_iterator;
  }

private:
  std::shared_ptr<StreamIterator> m_iterator;
};

//! Helper class for simpler type-safe iteration over audio chunks
class AudioStreamIterator {
public:
  //! Constructor taking a StreamIterator
  explicit AudioStreamIterator(std::shared_ptr<StreamIterator> iterator)
    : m_iterator(std::move(iterator)) {
    if (m_iterator->streamType() != StreamType::Audio) {
      throw std::runtime_error("StreamIterator is not for an audio stream");
    }
  }

  //! Move to the next chunk
  VideoResult<void> next() {
    return m_iterator->next();
  }

  //! Move to the previous chunk
  VideoResult<void> previous() {
    return m_iterator->previous();
  }

  //! Get the current audio chunk
  VideoResult<std::shared_ptr<AudioChunk>> currentChunk() const {
    return m_iterator->currentAudioChunk();
  }

  //! Seek to a specific timestamp
  VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) {
    return m_iterator->seek(timestamp, flags);
  }

  //! Get the underlying iterator
  std::shared_ptr<StreamIterator> iterator() const {
    return m_iterator;
  }

private:
  std::shared_ptr<StreamIterator> m_iterator;
};

//! Helper class for simpler type-safe iteration over metadata frames
class MetaDataStreamIterator {
public:
  //! Constructor taking a StreamIterator
  explicit MetaDataStreamIterator(std::shared_ptr<StreamIterator> iterator)
    : m_iterator(std::move(iterator)) {
    if (m_iterator->streamType() != StreamType::Data) {
      throw std::runtime_error("StreamIterator is not for a metadata stream");
    }
  }

  //! Move to the next frame
  VideoResult<void> next() {
    return m_iterator->next();
  }

  //! Move to the previous frame
  VideoResult<void> previous() {
    return m_iterator->previous();
  }

  //! Get the current metadata frame
  VideoResult<std::shared_ptr<MetaDataFrame>> currentFrame() const {
    return m_iterator->currentMetaDataFrame();
  }

  //! Seek to a specific timestamp
  VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) {
    return m_iterator->seek(timestamp, flags);
  }

  //! Get the underlying iterator
  std::shared_ptr<StreamIterator> iterator() const {
    return m_iterator;
  }

private:
  std::shared_ptr<StreamIterator> m_iterator;
};

} // namespace Video
} // namespace Ravl2
