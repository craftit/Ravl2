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
#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2::Video
{
  // Forward declarations
  class MediaContainer;
  class FfmpegMultiStreamIterator;

  //! Class representing an iterator for a specific stream in a media container
  class StreamIterator
  {
  public:
    //! Virtual destructor
    virtual ~StreamIterator() = default;

    //! Get the stream index this iterator is associated with
    [[nodiscard]] std::size_t streamIndex() const
    {
      return mStreamIndex;
    }

    //! Get the stream type
    [[nodiscard]] StreamType streamType() const;

    //! Get the current position in the stream (as a timestamp)
    [[nodiscard]] MediaTime position() const
    {
      return mPosition;
    }

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
    [[nodiscard]] const std::shared_ptr<Frame>& currentFrame() const
    {
      return mCurrentFrame;
    }

    //! Get a specific frame by its unique ID
    virtual VideoResult<std::shared_ptr<Frame>> getFrameById(StreamItemId id) const = 0;

    //! Reset the iterator to the beginning of the stream
    virtual VideoResult<void> reset() = 0;

    //! Get the parent container
    const std::shared_ptr<MediaContainer>& container() const
    {
      return mContainer;
    }

    //! Get the total duration of the stream
    virtual MediaTime duration() const = 0;

    //! Check if the iterator can seek
    [[nodiscard]] virtual bool canSeek() const = 0;

    //! Get the data element type held in the frames.
    [[nodiscard]] virtual std::type_info const& dataType() const;

  protected:
    StreamIterator(std::shared_ptr<MediaContainer> container, std::size_t streamIndex)
      : mStreamIndex(streamIndex)
        , mContainer(std::move(container))
    {
      RavlAlwaysAssertMsg(mContainer, "MediaContainer pointer is null");
    }

    void setCurrentFrame(std::shared_ptr<Frame> frame)
    {
      mCurrentFrame = std::move(frame);
      if (mCurrentFrame)
      {
        mPosition = mCurrentFrame->timestamp();
      }
      else
      {
        mPosition = MediaTime(0);
      }
    }

    [[nodiscard]] MediaContainer& media()
    {
      return *mContainer;
    }

    [[nodiscard]] const MediaContainer& media() const
    {
      return *mContainer;
    }

    [[nodiscard]] bool isValid() const
    {
      return mContainer && mCurrentFrame;
    }

    friend class FfmpegMultiStreamIterator;

  private:
    std::size_t mStreamIndex = 0;
    MediaTime mPosition {};
    std::shared_ptr<MediaContainer> mContainer;
    std::shared_ptr<Frame> mCurrentFrame;
  };

  //! Helper class for simpler type-safe iteration over data frames
  template<typename ImageTypeT>
  class TypedStreamIterator
  {
  public:
    //! Constructor taking a StreamIterator
    explicit TypedStreamIterator(std::shared_ptr<StreamIterator> iterator)
      : m_iterator(std::move(iterator))
    {
      if (!m_iterator)
      {
        throw std::runtime_error("StreamIterator is null");
      }
      auto&targetType = typeid(ImageTypeT);
      if (targetType != m_iterator->dataType())
      {
        mConversionChain = Ravl2::typeConverterMap().find(targetType, m_iterator->dataType());
        if (!mConversionChain)
        {
          SPDLOG_WARN("No conversion available from {} to {}",
                      Ravl2::typeName(m_iterator->dataType()),
                      Ravl2::typeName(targetType)
          );
          // Dump available conversions for debugging
          SPDLOG_WARN("Available conversions:");
          Ravl2::typeConverterMap().dump();
          SPDLOG_WARN("Done.");
          throw std::runtime_error("Cannot convert frames to the requested type");
        }
      }
    }

    //! Move to the next frame
    VideoResult<void> next()
    {
      return m_iterator->next();
    }

    //! Move to the previous frame
    VideoResult<void> previous()
    {
      return m_iterator->previous();
    }

    //! Are we at a valid frame?
    bool isValid() const
    {
      return m_iterator->currentFrame() && m_iterator->currentFrame()->isValid();
    }

    //! Get the current frame
    [[nodiscard]] FrameData<ImageTypeT>& currentFrame() const
    {
      assert(m_iterator->currentFrame());
      auto ptr = std::dynamic_pointer_cast<FrameData<ImageTypeT>>(m_iterator->currentFrame());
      if(!ptr) {
        SPDLOG_ERROR("Unexpected frame type.");
        RavlAlwaysAssertMsg(false, "Unexpected frame type.");
      }
      return *ptr;
    }

    //! Get the current frame
    [[nodiscard]] ImageTypeT videoFrame()
    {
      if (mConversionChain)
      {
        return std::any_cast<ImageTypeT>(mConversionChain->convert(m_iterator->currentFrame()->frameData()));
      }
      if (typeid(ImageTypeT) != m_iterator->dataType())
      {
        throw std::runtime_error("Frame type does not match iterator data type and no conversion available");
      }
      return currentFrame().image();
    }

    //! Seek to a specific timestamp
    VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise)
    {
      return m_iterator->seek(timestamp, flags);
    }

    //! Get the underlying iterator
    [[nodiscard]] std::shared_ptr<StreamIterator> iterator() const
    {
      return m_iterator;
    }

  private:
    std::shared_ptr<StreamIterator> m_iterator;
    std::optional<ConversionChain> mConversionChain;
  };

} // namespace Ravl2::Video
