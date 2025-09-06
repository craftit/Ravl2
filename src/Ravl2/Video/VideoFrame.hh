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

namespace Ravl2 {
namespace Video {

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

//! Forward declarations for specializations
template<typename PixelT>
class VideoFrameImpl;

//! Interface class for video frames
class VideoFrame : public VideoFrameBase {
public:
  //! Virtual destructor
  virtual ~VideoFrame() = default;

  //! Create a video frame with a specific pixel type
  template<typename PixelT>
  static std::shared_ptr<VideoFrame> create(const Array<PixelT, 2>& frameData, StreamItemId id, MediaTime timestamp) {
    return std::make_shared<VideoFrameImpl<PixelT>>(frameData, id, timestamp);
  }

  //! Get the frame data as a specific pixel type
  template<typename PixelT>
  Array<PixelT, 2> frameData() const {
    // This will throw if the pixel type doesn't match
    return getFrameDataImpl<PixelT>();
  }

  //! Get the pixel type name
  virtual std::string pixelTypeName() const = 0;

  //! Check if the frame has valid data
  virtual bool isValid() const = 0;

protected:
  //! Constructor with ID and timestamp
  VideoFrame(StreamItemId id, MediaTime timestamp)
    : VideoFrameBase(id, timestamp)
  {}

  //! Default constructor
  VideoFrame() = default;

  //! Implementation of getting frame data for a specific pixel type
  template<typename PixelT>
  virtual Array<PixelT, 2> getFrameDataImpl() const = 0;
};

//! Template implementation of VideoFrame for a specific pixel type
template<typename PixelT>
class VideoFrameImpl : public VideoFrame {
public:
  //! Constructor with frame data, ID, and timestamp
  VideoFrameImpl(const Array<PixelT, 2>& frameData, StreamItemId id, MediaTime timestamp)
    : VideoFrame(id, timestamp)
    , m_frameData(frameData)
  {}

  //! Get the frame width - computed from frame data to avoid redundancy
  int width() const override { return m_frameData.size(0); }

  //! Get the frame height - computed from frame data to avoid redundancy
  int height() const override { return m_frameData.size(1); }

  //! Get the pixel type name
  std::string pixelTypeName() const override {
    return typeid(PixelT).name();
  }

  //! Check if the frame has valid data
  bool isValid() const override {
    return !m_frameData.empty(); // Actually check if we have data
  }

protected:
  //! Implementation of getting frame data for a specific pixel type
  template<typename T>
  Array<T, 2> getFrameDataImpl() const override {
    if (typeid(T) != typeid(PixelT)) {
      throw std::runtime_error("Pixel type mismatch when accessing frame data");
    }
    return m_frameData;
  }

private:
  Array<PixelT, 2> m_frameData;  //!< The frame data
};

} // namespace Video
} // namespace Ravl2
