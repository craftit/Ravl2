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
#include "Ravl2/IO/TypeConverter.hh"

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
  virtual int64_t positionIndex() const;

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

  //! Get the data element type held in the frames.
  [[nodiscard]] virtual std::type_info const &dataType() const;

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
  using ImageTypeT = Ravl2::Array<PixelT,2>;

  //! Constructor taking a StreamIterator
  explicit VideoStreamIterator(std::shared_ptr<StreamIterator> iterator)
    : m_iterator(std::move(iterator))
  {
    if (!m_iterator)
    {
      throw std::runtime_error("StreamIterator is null");
    }
    if (m_iterator->streamType() != StreamType::Video) {
      throw std::runtime_error("StreamIterator is not for a video stream");
    }
    auto &targetType = typeid(ImageTypeT);
    if (targetType != m_iterator->dataType())
    {
      mConversionChain = Ravl2::TypeConverterMap().find(targetType, m_iterator->dataType());
      if (!mConversionChain) {
        SPDLOG_WARN("No conversion available from {} to {}", Ravl2::typeName(m_iterator->dataType()), Ravl2::typeName(targetType));
        throw std::runtime_error("Cannot convert frames to the requested type");
      }
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

  //! Are we at a valid frame?
  bool isValid() const
  {
    return m_iterator->currentFrame() && m_iterator->currentFrame()->isValid();
  }

  //! Get the current frame
  VideoFrame<PixelT> &currentFrame() const
  {
    return *std::dynamic_pointer_cast<VideoFrame<PixelT>>(m_iterator->currentFrame());
  }

  //! Get the current frame
  ImageTypeT videoFrame()
  {
    if (mConversionChain) {
      return std::any_cast<ImageTypeT>(mConversionChain->convert(currentFrame().frameData()));
    }
    if (typeid(ImageTypeT) != m_iterator->dataType()) {
      throw std::runtime_error("Frame type does not match iterator data type and no conversion available");
    }
    return currentFrame().image();
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
  std::optional<ConversionChain> mConversionChain;
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
