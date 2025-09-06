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

namespace Ravl2 {
namespace Video {

// Forward declaration
class StreamIterator;

//! Unified stream properties variant type
using StreamProperties = std::variant<VideoProperties, AudioProperties, DataProperties>;

//! Class representing a media container (file, memory, network stream)
class MediaContainer : public std::enable_shared_from_this<MediaContainer> {
public:
  //! Virtual destructor
  virtual ~MediaContainer() = default;

  //! Check if the container is open
  virtual bool isOpen() const = 0;

  //! Close the container and release resources
  virtual VideoResult<void> close() = 0;

  //! Get the number of streams in the container
  virtual int streamCount() const = 0;

  //! Get the type of a stream at the specified index
  virtual StreamType streamType(int streamIndex) const = 0;

  //! Get properties for any stream type (unified interface)
  virtual VideoResult<StreamProperties> streamProperties(int streamIndex) const = 0;

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

  //! Convenience method for getting video properties (uses streamProperties internally)
  VideoResult<VideoProperties> videoProperties(int streamIndex) const {
    auto result = streamProperties(streamIndex);
    if (!result.isSuccess()) {
      return VideoResult<VideoProperties>(result.error());
    }

    if (streamType(streamIndex) != StreamType::Video) {
      return VideoResult<VideoProperties>(VideoErrorCode::InvalidOperation);
    }

    try {
      return VideoResult<VideoProperties>(std::get<VideoProperties>(result.value()));
    } catch (const std::bad_variant_access&) {
      return VideoResult<VideoProperties>(VideoErrorCode::InvalidOperation);
    }
  }

  //! Convenience method for getting audio properties (uses streamProperties internally)
  VideoResult<AudioProperties> audioProperties(int streamIndex) const {
    auto result = streamProperties(streamIndex);
    if (!result.isSuccess()) {
      return VideoResult<AudioProperties>(result.error());
    }

    if (streamType(streamIndex) != StreamType::Audio) {
      return VideoResult<AudioProperties>(VideoErrorCode::InvalidOperation);
    }

    try {
      return VideoResult<AudioProperties>(std::get<AudioProperties>(result.value()));
    } catch (const std::bad_variant_access&) {
      return VideoResult<AudioProperties>(VideoErrorCode::InvalidOperation);
    }
  }

  //! Convenience method for getting data properties (uses streamProperties internally)
  VideoResult<DataProperties> dataProperties(int streamIndex) const {
    auto result = streamProperties(streamIndex);
    if (!result.isSuccess()) {
      return VideoResult<DataProperties>(result.error());
    }

    if (streamType(streamIndex) != StreamType::Data) {
      return VideoResult<DataProperties>(VideoErrorCode::InvalidOperation);
    }

    try {
      return VideoResult<DataProperties>(std::get<DataProperties>(result.value()));
    } catch (const std::bad_variant_access&) {
      return VideoResult<DataProperties>(VideoErrorCode::InvalidOperation);
    }
  }

protected:
  //! Protected constructor to prevent direct instantiation
  MediaContainer() = default;

  //! Mutex for thread-safe operations
  mutable std::mutex m_mutex;
};

} // namespace Video
} // namespace Ravl2
