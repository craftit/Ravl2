//
// Created on September 6, 2025
//

#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <optional>

#include "Ravl2/Assert.hh"
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/Frame.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/Video/MetaDataFrame.hh"

namespace Ravl2::Video {

// Forward declaration
class MediaContainer;

//! Class representing an iterator for a specific stream in a media container
class StreamIterator {
public:
  //! Virtual destructor
  virtual ~StreamIterator() = default;

  //! Get the stream index this iterator is associated with
  [[nodiscard]] std::size_t streamIndex() const
  { return mStreamIndex; }

  //! Get the stream type
  [[nodiscard]] StreamType streamType() const;

  //! Get the current position in the stream (as a timestamp)
  [[nodiscard]] MediaTime position() const
  { return mPosition; }

  //! Get the current position as a frame/chunk index
  int64_t positionIndex() const;

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
  [[nodiscard]] const std::shared_ptr<Frame> &currentFrame() const
  { return mCurrentFrame; }

  //! Get a specific frame by its unique ID
  virtual VideoResult<std::shared_ptr<Frame>> getFrameById(StreamItemId id) const = 0;

  //! Reset the iterator to the beginning of the stream
  virtual VideoResult<void> reset() = 0;

  //! Get the parent container
  const std::shared_ptr<MediaContainer> &container() const
  { return mContainer; }

  //! Get the total duration of the stream
  virtual MediaTime duration() const = 0;

  //! Check if the iterator can seek
  [[nodiscard]] virtual bool canSeek() const = 0;

protected:
  StreamIterator(std::shared_ptr<MediaContainer> container, std::size_t streamIndex)
    : mStreamIndex(streamIndex)
    , mContainer(std::move(container)) {
    RavlAlwaysAssertMsg(mContainer, "MediaContainer pointer is null");
  }

  void setCurrentFrame(std::shared_ptr<Frame> frame) {
    mCurrentFrame = std::move(frame);
    if (mCurrentFrame) {
      mPosition = mCurrentFrame->timestamp();
    } else {
      mPosition = MediaTime(0);
    }
  }

  [[nodiscard]] MediaContainer &media()
  { return *mContainer; }

  [[nodiscard]] const MediaContainer &media() const
  { return *mContainer; }

  [[nodiscard]] bool isValid() const
  { return mContainer && mCurrentFrame; }
private:
  std::size_t mStreamIndex = 0;
  std::shared_ptr<MediaContainer> mContainer;
  MediaTime mPosition {};
  std::shared_ptr<Frame> mCurrentFrame;
};

//! Helper class for simpler type-safe iteration over video frames
template <typename PixelT>
class VideoStreamIterator
{
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
  VideoFrame<PixelT> &currentFrame() const
  {
    return *std::dynamic_pointer_cast<VideoFrame<PixelT>>(m_iterator->currentFrame());
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
template <typename SampleT>
class AudioStreamIterator
{
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

  //! Get the current frame
  AudioChunk<SampleT> &currentFrame() const
  {
    return *std::dynamic_pointer_cast<AudioChunk<SampleT>>(m_iterator->currentFrame());
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
template <typename DataT>
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
  [[nodiscard]] const MetaDataFrame<DataT> &currentFrame() const {
    return *std::dynamic_pointer_cast<MetaDataFrame<DataT>>(m_iterator->currentFrame());
  }

  //! Seek to a specific timestamp
  VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) {
    return m_iterator->seek(timestamp, flags);
  }

  //! Get the underlying iterator
  [[nodiscard]] std::shared_ptr<StreamIterator> iterator() const {
    return m_iterator;
  }

private:
  std::shared_ptr<StreamIterator> m_iterator;
};

} // namespace Ravl2::Video
