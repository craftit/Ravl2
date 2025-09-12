// filepath: /home/charles/src/Ravl2/src/Ravl2/Video/MemoryMediaContainer.hh
//
// Created on September 6, 2025
// An in-memory implementation of MediaContainer
//

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <variant>
#include "Ravl2/Video/MediaContainer.hh"
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/Video/MetaDataFrame.hh"
#include "Ravl2/Video/Frame.hh"
#include "Ravl2/Video/StreamIterator.hh"

namespace Ravl2::Video {

// Forward declaration
class MemoryStreamIterator;

//! A structure to hold a single stream's data
struct StreamData {
  StreamType mType = StreamType::Unknown;       //!< Type of this stream
  std::vector<std::shared_ptr<Frame>> mFrames;  //!< Frames/chunks in this stream

  // Stream properties based on type
  std::variant<VideoProperties, AudioProperties, DataProperties> mProperties;

  //! Default constructor
  StreamData() = default;

  //! Constructor for video stream
  StreamData(StreamType type, const std::vector<std::shared_ptr<Frame>>& frames, const VideoProperties& props)
    : mType(type), mFrames(frames), mProperties(props) {}

  //! Constructor for audio stream
  StreamData(StreamType type, const std::vector<std::shared_ptr<Frame>>& frames, const AudioProperties& props)
    : mType(type), mFrames(frames), mProperties(props) {}

  //! Constructor for data stream
  StreamData(StreamType type, const std::vector<std::shared_ptr<Frame>>& frames, const DataProperties& props)
    : mType(type), mFrames(frames), mProperties(props) {}
};

//! Class representing an in-memory media container constructed from vectors of frames
class MemoryMediaContainer : public MediaContainer {
public:
  //! Constructor with streams data
  explicit MemoryMediaContainer(const std::vector<StreamData>& streams,
                               const std::map<std::string, std::string>& metadata = {});

  //! Destructor
  ~MemoryMediaContainer() override = default;

  // Friend declaration to allow MemoryStreamIterator to access private members
  friend class MemoryStreamIterator;

  //! Check if the container is open
  [[nodiscard]] bool isOpen() const override;

  //! Close the container and release resources
  VideoResult<void> close() override;

  //! Get the number of streams in the container
  [[nodiscard]] std::size_t streamCount() const override;

  //! Get the type of stream at the specified index
  [[nodiscard]] StreamType streamType(std::size_t streamIndex) const override;

  //! Get properties for a video stream
  [[nodiscard]] VideoResult<VideoProperties> videoProperties(std::size_t streamIndex) const override;

  //! Get properties for an audio stream
  [[nodiscard]] VideoResult<AudioProperties> audioProperties(std::size_t streamIndex) const override;

  //! Get properties for a data stream
  [[nodiscard]] VideoResult<DataProperties> dataProperties(std::size_t streamIndex) const override;

  //! Get the total duration of the container (the longest stream)
  [[nodiscard]] MediaTime duration() const override;

  //! Create an iterator for a specific stream
  [[nodiscard]] VideoResult<std::shared_ptr<StreamIterator>> createIterator(std::size_t streamIndex) override;

  //! Get global container metadata
  [[nodiscard]] std::map<std::string, std::string> metadata() const override;

  //! Get specific metadata value
  [[nodiscard]] std::string metadata(const std::string& key) const override;

  //! Check if a specific metadata key exists
  [[nodiscard]] bool hasMetadata(const std::string& key) const override;

  //! Factory method to create memory container from streams
  [[nodiscard]] static std::shared_ptr<MemoryMediaContainer> create(const std::vector<StreamData>& streams,
                                                    const std::map<std::string, std::string>& metadata = {});

private:
  //! Check if a stream index is valid
  bool isValidStreamIndex(std::size_t streamIndex) const;

  std::vector<StreamData> m_streams;                      //!< The streams data
  std::map<std::string, std::string> m_metadata;          //!< Container metadata
  bool m_isOpen = true;                                   //!< Flag indicating if the container is open
  MediaTime m_duration{0};                                //!< Container duration (cached)
};

//! Iterator implementation for memory container streams
class MemoryStreamIterator : public StreamIterator {
public:
  //! Constructor
  MemoryStreamIterator(std::shared_ptr<MemoryMediaContainer> container, std::size_t streamIndex);

  //! Check if the iterator is at the end of the stream
  bool isAtEnd() const override;

  //! Move to the next frame/chunk
  VideoResult<void> next() override;

  //! Move to the previous frame/chunk
  VideoResult<void> previous() override;

  //! Seek to a specific timestamp
  VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) override;

  //! Seek to a specific frame/chunk index
  VideoResult<void> seekToIndex(int64_t index) override;

  //! Get a specific frame by its unique ID
  [[nodiscard]] VideoResult<std::shared_ptr<Frame>> getFrameById(StreamItemId id) const override;

  //! Reset the iterator to the beginning of the stream
  VideoResult<void> reset() override;

  //! Check if the iterator can seek
  [[nodiscard]] bool canSeek() const override
  {
    return true;
  }

  //! Get the total duration of the stream
  MediaTime duration() const override;

  //! Access the position.
  int64_t positionIndex() const override
  { return m_currentPosition; }

private:
  const std::vector<std::shared_ptr<Frame>>* m_frames = nullptr;    //!< Pointer to frames in the stream
  std::ptrdiff_t m_currentPosition = 0;
};

} // namespace Ravl2::Video
