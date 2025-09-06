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

//! Forward declarations for specializations
template<typename PixelT>
class VideoFrameImpl;

//! Interface class for video frames with type erasure for template functions
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
    // This will be implemented in derived classes via type erasure
    if (pixelTypeName() != typeid(PixelT).name()) {
      throw std::runtime_error("Pixel type mismatch when accessing frame data");
    }

    // Each derived class will implement this method for its specific pixel type
    return getFrameDataTyped(static_cast<const PixelT*>(nullptr));
  }

  //! Get the pixel type name
  virtual std::string pixelTypeName() const = 0;

  //! Check if the frame has valid data
  virtual bool isValid() const override = 0;

protected:
  //! Constructor with ID and timestamp
  VideoFrame(StreamItemId id, MediaTime timestamp)
    : VideoFrameBase(id, timestamp)
  {}

  //! Default constructor
  VideoFrame() = default;

  //! Type-erased implementation for getting frame data of a specific type
  virtual Array<char, 2> getFrameDataRaw() const = 0;

  //! Template method to be specialized by derived classes
  template<typename PixelT>
  Array<PixelT, 2> getFrameDataTyped(const PixelT*) const {
    throw std::runtime_error("Unsupported pixel type");
  }
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

  //! Get the frame width
  int width() const override { return m_frameData.size(0); }

  //! Get the frame height
  int height() const override { return m_frameData.size(1); }

  //! Get the pixel type name
  std::string pixelTypeName() const override {
    return typeid(PixelT).name();
  }

  //! Check if the frame has valid data
  bool isValid() const override {
    return !m_frameData.empty();
  }

protected:
  //! Raw access to frame data for type erasure
  Array<char, 2> getFrameDataRaw() const override {
    // This is just a placeholder - in real code we'd need proper type conversion
    throw std::runtime_error("Raw data access not implemented");
  }

  //! Specialization for the actual pixel type
  Array<PixelT, 2> getFrameDataTyped(const PixelT*) const {
    return m_frameData;
  }

private:
  Array<PixelT, 2> m_frameData;  //!< The frame data
};

} // namespace Ravl2::Video
