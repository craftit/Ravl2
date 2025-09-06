//
// Created on September 6, 2025
//

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <variant>
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/Video/MetaDataFrame.hh"

namespace Ravl2::Video {

// Forward declaration
class StreamIterator;

//! Class representing a media container (file, memory, network stream)
class MediaContainer : public std::enable_shared_from_this<MediaContainer> {
public:
  //! Virtual destructor
  virtual ~MediaContainer() = default;

  //! Open a media container from a file path
  static VideoResult<std::shared_ptr<MediaContainer>> openFile(const std::string& filePath) {
    // This is a placeholder implementation
    // In a real implementation, this would create the appropriate container type based on the file extension
    return VideoResult<std::shared_ptr<MediaContainer>>(VideoErrorCode::UnsupportedFormat);
  }

  //! Open a media container from memory
  static VideoResult<std::shared_ptr<MediaContainer>> openMemory(const uint8_t* data, size_t size) {
    // This is a placeholder implementation
    // In a real implementation, this would create an in-memory container
    return VideoResult<std::shared_ptr<MediaContainer>>(VideoErrorCode::UnsupportedFormat);
  }

  //! Check if the container is open
  virtual bool isOpen() const = 0;

  //! Close the container and release resources
  virtual VideoResult<void> close() = 0;

  //! Get the number of streams in the container
  virtual int streamCount() const = 0;

  //! Get the type of a stream at the specified index
  virtual StreamType streamType(int streamIndex) const = 0;

  //! Get properties for a video stream
  virtual VideoResult<VideoProperties> videoProperties(int streamIndex) const = 0;

  //! Get properties for an audio stream
  virtual VideoResult<AudioProperties> audioProperties(int streamIndex) const = 0;

  //! Get properties for a data stream
  virtual VideoResult<DataProperties> dataProperties(int streamIndex) const = 0;

  //! Get total duration of the container (longest stream)
  virtual MediaTime duration() const = 0;

  //! Create an iterator for a specific stream
  virtual VideoResult<std::shared_ptr<StreamIterator>> createIterator(int streamIndex) = 0;

  //! Get global container metadata
  virtual std::map<std::string, std::string> metadata() const = 0;

  //! Get specific metadata value
  virtual std::string metadata(const std::string& key) const = 0;

  //! Check if a specific metadata key exists
  virtual bool hasMetadata(const std::string& key) const = 0;

protected:
  //! Protected constructor to prevent direct instantiation
  MediaContainer() = default;

  //! Mutex for thread-safe operations
  mutable std::mutex m_mutex;
};

} // namespace Ravl2::Video
