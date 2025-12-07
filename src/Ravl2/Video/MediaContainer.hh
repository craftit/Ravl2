//
// Created on September 6, 2025
//

#pragma once

#include "StreamIterator.hh"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <variant>
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/Frame.hh"

namespace Ravl2::Video
{
  // Forward declaration
  class StreamIterator;

  //! Class representing a media container (file, memory, network stream)
  class MediaContainer : public std::enable_shared_from_this<MediaContainer>
  {
  public:
    //! Virtual destructor
    virtual ~MediaContainer() = default;

    //! Register a type
    static bool registerDataType(const std::type_index& type,StreamType streamType);

    //! Open a media container from a file path
    static VideoResult<std::shared_ptr<MediaContainer>> openFile(const std::string&filePath);

    //! Open a capture device (e.g., webcam)
    static VideoResult<std::shared_ptr<MediaContainer>> openDevice(const DeviceParameters&params);

    //! Check if the container is open
    virtual bool isOpen() const = 0;

    //! Close the container and release resources
    virtual VideoResult<void> close() = 0;

    //! Get the number of streams in the container
    virtual std::size_t streamCount() const = 0;

    //! Get the type of stream at the specified index
    virtual StreamType streamType(std::size_t streamIndex) const = 0;

    //! Get properties for a video stream
    virtual VideoResult<VideoProperties> videoProperties(std::size_t streamIndex) const = 0;

    //! Get properties for an audio stream
    virtual VideoResult<AudioProperties> audioProperties(std::size_t streamIndex) const = 0;

    //! Get properties for a data stream
    virtual VideoResult<DataProperties> dataProperties(std::size_t streamIndex) const = 0;

    //! Get the total duration of the container (the longest stream)
    virtual MediaTime duration() const = 0;

    //! Create an iterator for a specific stream with a target type
    virtual VideoResult<std::shared_ptr<StreamIterator>> createIterator(const std::type_info &dataType,std::size_t streamIndex = std::numeric_limits<std::size_t>::max());

    //! Create an iterator for a specific stream
    virtual VideoResult<std::shared_ptr<StreamIterator>> createIterator(std::size_t streamIndex) = 0;

    //! Create an iterator for a set of streams.
    virtual VideoResult<std::shared_ptr<StreamIterator>> createIterator(std::vector<std::size_t> streams) = 0;

    template<typename DataT>
    TypedStreamIterator<DataT> createIterator(std::size_t streamIndex = std::numeric_limits<std::size_t>::max())
    {
      return TypedStreamIterator<DataT>(createIterator(typeid(DataT),streamIndex).value());
    }

    //! Get global container metadata
    virtual std::map<std::string, std::string> metadata() const = 0;

    //! Get specific metadata value
    virtual std::string metadata(const std::string&key) const = 0;

    //! Check if a specific metadata key exists
    virtual bool hasMetadata(const std::string&key) const = 0;

  protected:
    //! Protected constructor to prevent direct instantiation
    MediaContainer() = default;

    //! Mutex for thread-safe operations
    mutable std::shared_mutex m_mutex;
  };

  //! Enumerate available capture devices
  //! @return List of available capture devices
  VideoResult<std::vector<DeviceInfo>> enumerateDevices();

} // namespace Ravl2::Video
