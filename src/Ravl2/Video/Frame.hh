//
// Created on September 6, 2025
//

#pragma once

#include <chrono>
#include <memory>
#include <map>
#include <string>
#include <any>
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2::Video
{
  //! Base class for all media frames (video frames, audio chunks, etc.)
  //! We don't want to use a variant here as it means all types would need
  //! to be known in advance.

  class Frame
  {
  public:
    //! Virtual destructor
    virtual ~Frame() = default;

    //! Get the frame's unique identifier
    [[nodiscard]] StreamItemId id() const { return m_id; }

    //! Get the frame's timestamp
    [[nodiscard]] MediaTime timestamp() const { return m_timestamp; }

    //! Check if this frame is valid
    [[nodiscard]] virtual bool isValid() const = 0;

    //! Get the stream type this frame belongs to
    [[nodiscard]] StreamType streamType() const
    { return mStreamType; }

    //! Access data as std::any
    [[nodiscard]] virtual std::any frameData() const = 0;

    //! Access the frame data type.
    [[nodiscard]] virtual const std::type_info &dataType() const = 0;

    //! Convert a frame to a different type.
    //! This will throw std::bad_any_cast if the conversion is not possible.
    template <typename ToT>
    ToT cast() const
    { return typeConvert<ToT>(frameData()); }

    //! Convert a frame to a different type.
    //! This will return an empty optional if the type can't be converted.
    template <typename ToT>
    std::optional<ToT> castOpt() const
    { return typeConvertOpt<ToT>(frameData()); }

  protected:
    //! Constructor with ID and timestamp
    Frame(StreamItemId id, MediaTime timestamp,StreamType streamType)
      : m_id(id)
        , m_timestamp(timestamp)
        , mStreamType(streamType)
    {
    }

    //! Default constructor
    Frame() = default;

  private:
    StreamItemId m_id = 0; //!< Unique identifier for this frame
    MediaTime m_timestamp{0}; //!< Timestamp of when this frame starts.
    StreamType mStreamType = StreamType::Unknown;
  };


  //! Interface class for data contained in the frames.

  template<typename DataT>
  class FrameData : public Frame
  {
  public:
    //! Virtual destructor
    ~FrameData() override = default;

    //! Constructor with data, format, ID, and timestamp
    FrameData(const DataT &data, StreamItemId id, MediaTime timestamp,StreamType streamType)
      : Frame(id, timestamp,streamType),
        mData(data)
    {}

    //! Constructor with data, format, ID, and timestamp
    FrameData(DataT &&data, StreamItemId id, MediaTime timestamp,StreamType streamType)
      : Frame(id, timestamp,streamType),
        mData(std::move(data))
    {}

    //! Access the frame data type.
    [[nodiscard]] const std::type_info &dataType() const override
    { return typeid(DataT); }

    //! Get the metadata as a specific type
    [[nodiscard]] const DataT& data() const
    { return mData; }

    //! Get the data type name
    [[nodiscard]] virtual std::string dataTypeName() const
    { return typeName(typeid(DataT)); }

    //! Check if the frame has valid data
    [[nodiscard]] bool isValid() const override
    { return true; }

    //! Access data
    [[nodiscard]] std::any frameData() const override
    { return mData; }

  protected:
    //! Constructor with ID and timestamp
    FrameData(StreamItemId id, MediaTime timestamp,StreamType streamType)
      : Frame(id, timestamp, streamType)
    {
    }

    //! Default constructor
    FrameData() = default;

  private:
    DataT mData; //!< The metadata
  };

} // namespace Ravl2::Video
