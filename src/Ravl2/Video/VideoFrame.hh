//
// Created on September 6, 2025
//

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <type_traits>
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/Frame.hh"

namespace Ravl2::Video {

//! Base class for video frames, regardless of pixel type
class VideoFrameBase : public Frame {
public:
  //! Get the frame width
  virtual int width() const = 0;

  //! Get the frame height
  virtual int height() const = 0;

  //! Set the keyframe flag
  void setKeyFrame(bool isKeyFrame) {
    m_isKeyFrame = isKeyFrame;
  }

  //! Check if this is a keyframe
  bool isKeyFrame() const {
    return m_isKeyFrame;
  }

  //! Get the stream type
  StreamType streamType() const override {
    return StreamType::Video;
  }

protected:
  //! Constructor with ID and timestamp
  VideoFrameBase(StreamItemId id, MediaTime timestamp)
    : Frame(id, timestamp)
  {}

  //! Default constructor
  VideoFrameBase() = default;

  bool m_isKeyFrame = false;  //!< Whether this frame is a keyframe
};


//! Interface class for video frames with type erasure for template functions
template<typename PixelT>
class VideoFrame : public VideoFrameBase {
public:
  //! Virtual destructor
  ~VideoFrame() override = default;

  //! Constructor with frame data, ID, and timestamp
  VideoFrame(const Array<PixelT, 2>& frameData, StreamItemId id, MediaTime timestamp)
    : VideoFrameBase(id, timestamp)
    , m_frameData(frameData)
  {}

  //! Get the frame width
  [[nodiscard]] int width() const override { return m_frameData.range().size(1); }

  //! Get the frame height
  [[nodiscard]] int height() const override { return m_frameData.range().size(0); }

  //! The frame data
  const Array<PixelT, 2> &image() const
  {
    return m_frameData;
  }

  //! Check if this frame is valid
  [[nodiscard]] bool isValid() const override
  {
    return (m_frameData.range().size(0) > 0 && m_frameData.range().size(1) > 0);
  }

  //! Access data as std::any
  [[nodiscard]] std::any frameData() const override
  {
    return m_frameData;
  }

protected:
  //! Constructor with ID and timestamp
  VideoFrame(StreamItemId id, MediaTime timestamp)
    : VideoFrameBase(id, timestamp)
  {}

  //! Default constructor
  VideoFrame() = default;

private:
  Array<PixelT, 2> m_frameData;  //!< The frame data

};


} // namespace Ravl2::Video
