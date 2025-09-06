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

namespace Ravl2::Video {

//! Base class for all media frames (video frames, audio chunks, etc.)
class Frame {
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
  [[nodiscard]] virtual StreamType streamType() const = 0;

  //! Access data as std::any
  [[nodiscard]] virtual std::any frameData() const = 0;

protected:
  //! Constructor with ID and timestamp
  Frame(StreamItemId id, MediaTime timestamp)
    : m_id(id)
    , m_timestamp(timestamp)
  {}

  //! Default constructor
  Frame() = default;

  StreamItemId m_id = 0;        //!< Unique identifier for this frame
  MediaTime m_timestamp{0};   //!< Timestamp of when this frame starts.
};

} // namespace Ravl2::Video
