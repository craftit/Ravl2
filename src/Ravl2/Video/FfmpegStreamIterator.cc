//
// Created on September 6, 2025
//

#include "Ravl2/Video/FfmpegStreamIterator.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"
#include <iostream>
#include <stdexcept>
#include <libswscale/swscale.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace Ravl2::Video {

FfmpegStreamIterator::FfmpegStreamIterator(std::shared_ptr<FfmpegMediaContainer> container, std::size_t streamIndex)
    : StreamIterator(container, streamIndex),
      m_nextFrameId(0),
      m_isAtEnd(false),
      m_currentPositionIndex(0),
      m_wasSeekOperation(false)
  {
  Ravl2::initPixel();

  // Allocate FFmpeg resources
  m_packet = av_packet_alloc();
  if (!m_packet) {
    throw std::runtime_error("Failed to allocate packet");
  }

  m_frame = av_frame_alloc();
  if (!m_frame) {
    av_packet_free(&m_packet);
    throw std::runtime_error("Failed to allocate frame");
  }

  // Get the FFmpeg stream
  m_stream = ffmpegContainer().m_formatContext->streams[streamIndex];

  // Get the codec context
  m_codecContext = ffmpegContainer().m_codecContexts[streamIndex];
  if (!m_codecContext) {
    av_frame_free(&m_frame);
    av_packet_free(&m_packet);
    SPDLOG_WARN("No codec context available for stream {}", streamIndex);
    throw std::runtime_error("No codec context available for stream");
  }

  // Read the first frame
  auto result = FfmpegStreamIterator::next();
  if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream) {
    av_frame_free(&m_frame);
    av_packet_free(&m_packet);
    SPDLOG_WARN("Failed to read first frame: {}", toString(result.error()));
    throw std::runtime_error("Failed to read first frame");
  }
  SPDLOG_INFO("Stream setup. First frame: {}", m_frame->pts);

}

FfmpegStreamIterator::~FfmpegStreamIterator() {
  // Free FFmpeg resources
  if (m_frame) {
    av_frame_free(&m_frame);
  }

  if (m_packet) {
    av_packet_free(&m_packet);
  }

  // We don't free m_stream or m_codecContext as they're owned by the container
}

bool FfmpegStreamIterator::isAtEnd() const {
  return m_isAtEnd;
}

VideoResult<void> FfmpegStreamIterator::next() {
  // If we're already at the end, return EndOfStream
  if (m_isAtEnd) {
    SPDLOG_INFO("Already at end of stream");
    return VideoResult<void>(VideoErrorCode::EndOfStream);
  }

  // Reset the last seek operation flag
  m_wasSeekOperation = false;

  try
  {
    while (true)
    {
      // Read the next packet
      auto result = readNextPacket();
      if (!result.isSuccess()) {
        SPDLOG_INFO("Failed to read next packet: {}", toString(result.error()));
        m_isAtEnd = true;
        return result;
      }

      // Decode the packet into a frame
      result = decodeCurrentPacket();
      if (result.isSuccess()) {
        break;
      }
      if (result.error() == VideoErrorCode::NeedMoreData){
        SPDLOG_INFO("Need more data to decode frame");
        continue;
      }
      SPDLOG_INFO("Failed to decode frame: {}", toString(result.error()));
      return result;
    }
  } catch (std::exception& e)
  {
    SPDLOG_INFO("Exception caught: {}", e.what());
    return VideoResult<void>(VideoErrorCode::DecodingError);
  }
  SPDLOG_INFO("Decoded frame: {}", m_frame->pts);

  // Increment the position index
  m_currentPositionIndex++;

  return VideoResult<void>(VideoErrorCode::Success);
}

VideoResult<void> FfmpegStreamIterator::previous() {
  // FFmpeg doesn't natively support backwards iteration
  // For proper support, we would need to maintain a buffer of recent frames
  // For now, just return an error
  return VideoResult<void>(VideoErrorCode::NotImplemented);
}

VideoResult<void> FfmpegStreamIterator::seek(MediaTime timestamp, SeekFlags flags) {
  auto& container = ffmpegContainer();

  if (!container.isOpen() || !m_stream) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Convert MediaTime to FFmpeg's time base
  int64_t timestamp_tb = av_rescale_q(timestamp.count(),
                                     AVRational{1, AV_TIME_BASE},
                                     m_stream->time_base);

  // Set FFmpeg seek flags
  int avFlags = 0;
  if (flags == SeekFlags::Keyframe) {
    avFlags |= AVSEEK_FLAG_ANY; // Seek to any frame, not just keyframes
  }
  if (flags == SeekFlags::Previous) {
    avFlags |= AVSEEK_FLAG_BACKWARD; // Seek backwards
  }

  // Perform the seek operation
  int result = av_seek_frame(container.m_formatContext, static_cast<int>(streamIndex()), timestamp_tb, avFlags);
  if (result < 0) {
    return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Flush codec buffers
  avcodec_flush_buffers(m_codecContext);

  // Set the flag indicating we've just performed a seek
  m_wasSeekOperation = true;
  m_isAtEnd = false;

  // Read the next frame at the new position
  return next();
}

VideoResult<void> FfmpegStreamIterator::seekToIndex(int64_t index) {
  // FFmpeg doesn't have direct frame index seeking for most formats
  // We would need to estimate the timestamp and use timestamp-based seeking

  auto& container = ffmpegContainer();

  if (!container.isOpen() || !m_stream) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Get video properties to estimate frame duration
  VideoResult<VideoProperties> propsResult;

  if (streamType() == StreamType::Video) {
    propsResult = container.videoProperties(streamIndex());
  } else if (streamType() == StreamType::Audio) {
    auto audioProps = container.audioProperties(streamIndex());
    if (audioProps.isSuccess() && audioProps.value().sampleRate > 0) {
      // For audio, convert sample index to timestamp
      MediaTime timestamp(static_cast<int64_t>(1000000.0 * static_cast<double>(index) / audioProps.value().sampleRate));
      return seek(timestamp);
    }
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  } else {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  if (!propsResult.isSuccess()) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  auto props = propsResult.value();

  if (props.frameRate <= 0) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Estimate timestamp from frame index
  MediaTime timestamp(static_cast<int64_t>(1000000.0 * static_cast<double>(index) / static_cast<double>(props.frameRate)));

  // Perform the seek
  return seek(timestamp);
}

VideoResult<std::shared_ptr<Frame>> FfmpegStreamIterator::getFrameById(StreamItemId id) const {
  (void) id;
  // FFmpeg doesn't have direct frame ID seeking
  // This would require either:
  // 1. Maintaining a cache of frames by ID
  // 2. Seeking to the start and reading until we find the frame with the right ID

  // For now, just return an error
  return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::NotImplemented);
}

VideoResult<void> FfmpegStreamIterator::reset() {
  // Seek to the beginning of the stream
  return seekToIndex(0);
}

MediaTime FfmpegStreamIterator::duration() const {
  auto& container = ffmpegContainer();

  if (!container.isOpen() || !m_stream) {
    return MediaTime(0);
  }

  // Get stream duration
  if (m_stream->duration != AV_NOPTS_VALUE) {
    int64_t duration_us = av_rescale_q(m_stream->duration, m_stream->time_base, AVRational{1, AV_TIME_BASE});
    return MediaTime(duration_us);
  } else {
    // Fall back to container duration
    return container.duration();
  }
}

bool FfmpegStreamIterator::canSeek() const {
  auto& container = ffmpegContainer();

  if (!container.isOpen() || !m_stream) {
    return false;
  }

  // Check if the format supports seeking
  // Define AVFMT_UNSEEKABLE if not already defined
  constexpr int AVFMT_UNSEEKABLE_FLAG = 0x0008; // Value from ffmpeg's avformat.h
  return !(container.m_formatContext->iformat->flags & AVFMT_UNSEEKABLE_FLAG);
}

int64_t FfmpegStreamIterator::positionIndex() const {
  return m_currentPositionIndex;
}

std::type_info const& FfmpegStreamIterator::dataType() const
{
  switch (streamType())
  {
    case StreamType::Video: {
        // Determine the pixel format to convert to based on FFmpeg's format
        switch (m_codecContext->pix_fmt) {
          case AV_PIX_FMT_RGB24:
            return typeid(Ravl2::Array<PixelRGB8,2>);
          case AV_PIX_FMT_RGBA:
            return typeid(Ravl2::Array<PixelRGBA8,2>);
          case AV_PIX_FMT_YUV420P:
          case AV_PIX_FMT_YUV422P:
          case AV_PIX_FMT_YUV444P:
            return typeid(Ravl2::Array<PixelYUV8,2>);
            //return typeid(Ravl2::Array<PixelYUV8,2>);
          default:
            // Default to RGB for other formats
            return typeid(Ravl2::Array<PixelRGB8,2>);
        }
    }
    case StreamType::Audio:
      {
        // Determine the sample format to convert to based on FFmpeg's format
        switch (m_codecContext->sample_fmt) {
          case AV_SAMPLE_FMT_U8:
          case AV_SAMPLE_FMT_U8P:
            return typeid(Ravl2::Array<uint8_t,2>);
          case AV_SAMPLE_FMT_S16:
          case AV_SAMPLE_FMT_S16P:
            return typeid(Ravl2::Array<int16_t,2>);
          case AV_SAMPLE_FMT_S32:
          case AV_SAMPLE_FMT_S32P:
            return typeid(Ravl2::Array<int32_t,2>);
          case AV_SAMPLE_FMT_FLT:
          case AV_SAMPLE_FMT_FLTP:
            return typeid(Ravl2::Array<float,2>);
          case AV_SAMPLE_FMT_DBL:
          case AV_SAMPLE_FMT_DBLP:
            return typeid(Ravl2::Array<double,2>);
          default:
            // Default to 16-bit PCM for other formats
            return typeid(Ravl2::Array<int16_t,2>);
        }
      }
    default:
      {
        SPDLOG_WARN("Unknown stream type: {}", toString(streamType()));
        return typeid(std::vector<uint8_t>);
      }
  }

  return StreamIterator::dataType();
}


template<typename PixelT>
bool FfmpegStreamIterator::makeImageWithSwScale(Array<PixelT, 2>& img, const AVFrame* frame) const
{
  int width = frame->width;
  int height = frame->height;

  // Create target frame data
  Array<PixelT, 2> frameData({static_cast<size_t>(height), static_cast<size_t>(width)});
  img = frameData;

  // Determine the appropriate target pixel format based on PixelT
  AVPixelFormat targetFormat;

  if constexpr (std::is_same_v<PixelT, PixelRGB8>) {
    targetFormat = AV_PIX_FMT_RGB24;
  } else if constexpr (std::is_same_v<PixelT, PixelRGBA8>) {
    targetFormat = AV_PIX_FMT_RGBA;
  } else if constexpr (std::is_same_v<PixelT, PixelBGR8>) {
    targetFormat = AV_PIX_FMT_BGR24;
  } else if constexpr (std::is_same_v<PixelT, PixelBGRA8>) {
    targetFormat = AV_PIX_FMT_BGRA;
  } else if constexpr (std::is_same_v<PixelT, PixelYUV8>) {
    // Direct copy for YUV format
    return makeImage(img, frame);
  } else {
    // Default to RGB format for other pixel types
    targetFormat = AV_PIX_FMT_RGB24;
  }

  // Get a scaling context
  SwsContext* swsContext = getSwsContext(
      width, height, m_codecContext->pix_fmt,
      width, height, targetFormat);

  if (!swsContext) {
    return false;
  }

  // Create a temporary AVFrame to hold the converted data
  AVFrame* dstFrame = av_frame_alloc();
  if (!dstFrame) {
    sws_freeContext(swsContext);
    SPDLOG_ERROR("Failed to allocate destination frame");
    return false;
  }

  // Set up the destination frame
  dstFrame->width = width;
  dstFrame->height = height;
  dstFrame->format = targetFormat;

  // Allocate buffer for the destination frame
  int ret = av_frame_get_buffer(dstFrame, 0);
  if (ret < 0) {
    av_frame_free(&dstFrame);
    sws_freeContext(swsContext);
    SPDLOG_ERROR("Failed to allocate buffer for destination frame: {}", ret);
    return false;
  }

  // Perform the conversion
  ret = sws_scale(swsContext, 
                 frame->data, frame->linesize, 0, height,
                 dstFrame->data, dstFrame->linesize);

  if (ret <= 0) {
    av_frame_free(&dstFrame);
    sws_freeContext(swsContext);
    SPDLOG_ERROR("Failed to convert frame: {}", ret);
    return false;
  }

  // Copy the converted data to our image array
  for (int y = 0; y < height; y++) {
    uint8_t* srcLine = dstFrame->data[0] + y * dstFrame->linesize[0];

    for (int x = 0; x < width; x++) {
      if constexpr (std::is_same_v<PixelT, PixelRGB8>) {
        uint8_t r = srcLine[x * 3];
        uint8_t g = srcLine[x * 3 + 1];
        uint8_t b = srcLine[x * 3 + 2];
        frameData[y][x] = PixelRGB8(r, g, b);
      } 
      else if constexpr (std::is_same_v<PixelT, PixelRGBA8>) {
        uint8_t r = srcLine[x * 4];
        uint8_t g = srcLine[x * 4 + 1];
        uint8_t b = srcLine[x * 4 + 2];
        uint8_t a = srcLine[x * 4 + 3];
        frameData[y][x] = PixelRGBA8(r, g, b, a);
      }
      else if constexpr (std::is_same_v<PixelT, PixelBGR8>) {
        uint8_t b = srcLine[x * 3];
        uint8_t g = srcLine[x * 3 + 1];
        uint8_t r = srcLine[x * 3 + 2];
        frameData[y][x] = PixelBGR8(b, g, r);
      }
      else if constexpr (std::is_same_v<PixelT, PixelBGRA8>) {
        uint8_t b = srcLine[x * 4];
        uint8_t g = srcLine[x * 4 + 1];
        uint8_t r = srcLine[x * 4 + 2];
        uint8_t a = srcLine[x * 4 + 3];
        frameData[y][x] = PixelBGRA8(b, g, r, a);
      }
      else {
        // For other formats, convert to RGB and then use color conversion
        uint8_t r = srcLine[x * 3];
        uint8_t g = srcLine[x * 3 + 1];
        uint8_t b = srcLine[x * 3 + 2];
        PixelRGB8 rgbPixel(r, g, b);
        Ravl2::assign(frameData[y][x], rgbPixel);
      }
    }
  }

  // Clean up
  av_frame_free(&dstFrame);
  sws_freeContext(swsContext);

  return true;
}

template<typename PixelT>
bool FfmpegStreamIterator::makeImage(Array<PixelT, 2>&img, const AVFrame* frame) const
{
#if 0
  // Use the SwScale implementation for all formats except YUV
  if (m_codecContext->pix_fmt == AV_PIX_FMT_YUV420P || 
      m_codecContext->pix_fmt == AV_PIX_FMT_YUV422P || 
      m_codecContext->pix_fmt == AV_PIX_FMT_YUV444P) {
    if constexpr (!std::is_same_v<PixelT, PixelYUV8>) {
      // For non-YUV target formats, use SwScale for YUV source
      return makeImageWithSwScale(img, frame);
    }
  }
#endif

  // For planar YUV formats (YUV420P, YUV422P, YUV444P), we need to convert from planar to packed
  int width = frame->width;
  int height = frame->height;

  // Allocate memory for packed YUV pixels
  // Create a 2D array from the packed data
  Array<PixelT, 2> frameData({static_cast<size_t>(height), static_cast<size_t>(width)});
  img = frameData;

  // Determine the chroma subsampling based on the pixel format
  int horizontalSubsample = 1;
  int verticalSubsample = 1;

  if (m_codecContext->pix_fmt == AV_PIX_FMT_YUV420P) {
    horizontalSubsample = 2;
    verticalSubsample = 2;
  } else if (m_codecContext->pix_fmt == AV_PIX_FMT_YUV422P) {
    horizontalSubsample = 2;
    verticalSubsample = 1;
  } else if (m_codecContext->pix_fmt == AV_PIX_FMT_YUV444P) {
    horizontalSubsample = 1;
    verticalSubsample = 1;
  } else
  {
    SPDLOG_ERROR("Unsupported pixel format for YUV conversion: {}", static_cast<int>(m_codecContext->pix_fmt));
    return false;
  }
  //m_codecContext->
  SPDLOG_INFO("Using subsampling: {}x{} Sizes:{} {}  {} {} {}  Target: {} ", horizontalSubsample, verticalSubsample, width, height, frame->linesize[0], frame->linesize[1], frame->linesize[2],typeName(typeid(PixelT)));

  // Get pointers to Y, U, V planes
  uint8_t* yPlane = frame->data[0];
  uint8_t* uPlane = frame->data[1];
  uint8_t* vPlane = frame->data[2];

  // Get strides for each plane
  int yStride = frame->linesize[0];
  int uStride = frame->linesize[1];
  int vStride = frame->linesize[2];

  // Convert planar YUV to packed YUV
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // Get Y value directly
      uint8_t yValue = yPlane[y * yStride + x];

      // Get U and V values based on subsampling
      int uIndex = (y / verticalSubsample) * uStride + (x / horizontalSubsample);
      int vIndex = (y / verticalSubsample) * vStride + (x / horizontalSubsample);

      // Get the chrominance values - these should already be offset by 128 in FFmpeg's YUV420P format
      uint8_t uValue = uPlane[uIndex];
      uint8_t vValue = vPlane[vIndex];

      // Set the pixel values from (Y, U, V, A)
      if constexpr (std::is_same_v<PixelT, PixelYUV8>)
      {
        frameData[y][x] = PixelYUV8(yValue, uValue, vValue);
      } else
      {
        Ravl2::assign(frameData[y][x],PixelYUVA8(yValue, uValue, vValue, 255));
      }
    }
  }
  return true;
}

VideoResult<void> FfmpegStreamIterator::readNextPacket() {
  auto& container = ffmpegContainer();

  if (!container.isOpen()) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }
  SPDLOG_INFO("Reading next packet");

  // Read packets until we find one from our stream
  while (true) {
    // Clear any previous packet data
    av_packet_unref(m_packet);

    // Read a packet
    int result = av_read_frame(container.m_formatContext, m_packet);

    if (result < 0) {
      // Check if we've reached the end of the stream
      if (result == AVERROR_EOF) {
        return VideoResult<void>(VideoErrorCode::EndOfStream);
      } else {
        return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
      }
    }

    // Check if the packet belongs to our stream
    if (m_packet->stream_index == static_cast<int>(streamIndex())) {
      break;
    }
    SPDLOG_INFO("Skipping packet from stream {}", m_packet->stream_index);

    // Free the packet if it's not from our stream
    av_packet_unref(m_packet);
  }
  SPDLOG_INFO("Read packet from stream {}", m_packet->stream_index);
  return VideoResult<void>(VideoErrorCode::Success);
}

VideoResult<void> FfmpegStreamIterator::decodeCurrentPacket() {
  if (!m_codecContext || !m_packet || !m_frame) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Send the packet to the decoder
  int result = avcodec_send_packet(m_codecContext, m_packet);

  if (result < 0) {
    SPDLOG_WARN("Error sending packet to decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
    return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Receive a frame from the decoder
  result = avcodec_receive_frame(m_codecContext, m_frame);

  if (result < 0) {
    // If the decoder needs more data, that's not an error
    if (result == AVERROR(EAGAIN)) {
      return VideoResult<void>(VideoErrorCode::NeedMoreData);
    }
    SPDLOG_WARN("Error receiving frame from decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
    return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Convert the FFmpeg frame to our Frame type
  auto frame = convertPacketToFrame();

  if (!frame) {
    SPDLOG_WARN("Failed to convert packet to frame");
    return VideoResult<void>(VideoErrorCode::DecodingError);
  }

  // Set the current frame
  setCurrentFrame(frame);

  return VideoResult<void>(VideoErrorCode::Success);
}

std::shared_ptr<Frame> FfmpegStreamIterator::convertPacketToFrame() {
  // Create the appropriate frame type based on the stream type
  switch (streamType()) {
    case StreamType::Video: {
      // Determine the pixel format to convert to based on FFmpeg's format
      switch (m_codecContext->pix_fmt) {
        case AV_PIX_FMT_RGB24:
          return createVideoFrame<PixelRGB8>();
        case AV_PIX_FMT_RGBA:
          return createVideoFrame<PixelRGBA8>();
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUV444P:
          // Using RGB8 format with SwScale conversion for most reliable results
          return createVideoFrame<PixelYUV8>();
        default:
          // Default to RGB for other formats
          return createVideoFrame<PixelRGB8>();
      }
    }
    case StreamType::Audio: {
      // Determine the sample format to convert to based on FFmpeg's format
      switch (m_codecContext->sample_fmt) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
          return createAudioChunk<uint8_t>();
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
          return createAudioChunk<int16_t>();
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
          return createAudioChunk<int32_t>();
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
          return createAudioChunk<float>();
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
          return createAudioChunk<double>();
        default:
          // Default to 16-bit PCM for other formats
          return createAudioChunk<int16_t>();
      }
    }
    case StreamType::Data: {
      // For metadata frames, use a binary blob
      return createMetadataFrame<std::vector<std::byte>>();
    }
    default:
      return nullptr;
  }
}

template <typename PixelT>
std::shared_ptr<VideoFrame<Array<PixelT,2>>> FfmpegStreamIterator::createVideoFrame() {
  if (!m_frame || m_frame->width <= 0 || m_frame->height <= 0) {
    return nullptr;
  }

  // Get the timestamp in our MediaTime format
  MediaTime timestamp(0);

  // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
  int64_t nopts = AV_NOPTS_VALUE;
  if (m_frame->pts != nopts) {
    int64_t pts_us = av_rescale_q(m_frame->pts, m_stream->time_base, AVRational{1, AV_TIME_BASE});
    timestamp = MediaTime(pts_us);
  }

  // Create a new frame ID
  StreamItemId id = m_nextFrameId++;

  Array<PixelT, 2> frameData;
  makeImage(frameData, m_frame);

  // Create a new video frame
  auto videoFrame = std::make_shared<VideoFrame<Array<PixelT,2>>>(frameData, id, timestamp);

  // Set keyframe flag - check the picture type instead of key_frame
  // In newer FFmpeg versions, use the pict_type field to determine if it's a key frame
  videoFrame->setKeyFrame(m_frame->pict_type == AV_PICTURE_TYPE_I);

  return videoFrame;
}

template <typename SampleT>
std::shared_ptr<AudioChunk<SampleT>> FfmpegStreamIterator::createAudioChunk() {
  if (!m_frame || m_frame->nb_samples <= 0) {
    return nullptr;
  }

  // Get number of channels from codec context since it's not directly in the frame
  int channels = m_codecContext->ch_layout.nb_channels;
  if (channels <= 0) {
    return nullptr;
  }

  // Get the timestamp in our MediaTime format
  MediaTime timestamp(0);

  // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
  int64_t nopts = AV_NOPTS_VALUE;
  if (m_frame->pts != nopts) {
    int64_t pts_us = av_rescale_q(m_frame->pts, m_stream->time_base, AVRational{1, AV_TIME_BASE});
    timestamp = MediaTime(pts_us);
  }

  // Create a new frame ID
  StreamItemId id = m_nextFrameId++;

  // Create a 2D array for the audio data (samples x channels)
  Array<SampleT, 2> audioData({static_cast<size_t>(m_frame->nb_samples), static_cast<size_t>(channels)});

  // Create a new audio chunk
  auto audioChunk = std::make_shared<AudioChunk<SampleT>>(audioData, id, timestamp);

  return audioChunk;
}

template <typename DataT>
std::shared_ptr<MetaDataFrame<DataT>> FfmpegStreamIterator::createMetadataFrame() {
  if (!m_frame) {
    return nullptr;
  }

  // Get the timestamp in our MediaTime format
  MediaTime timestamp(0);

  // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
  int64_t nopts = AV_NOPTS_VALUE;
  if (m_frame->pts != nopts) {
    int64_t pts_us = av_rescale_q(m_frame->pts, m_stream->time_base, AVRational{1, AV_TIME_BASE});
    timestamp = MediaTime(pts_us);
  }

  // Create a new frame ID
  StreamItemId id = m_nextFrameId++;

  // Create metadata container (implementation will vary depending on data type)
  DataT data;

  // Create a new metadata frame
  auto metadataFrame = std::make_shared<MetaDataFrame<DataT>>(data, id, timestamp);

  return metadataFrame;
}

FfmpegMediaContainer& FfmpegStreamIterator::ffmpegContainer() const {
  // Cast the container to FfmpegMediaContainer
  return *std::static_pointer_cast<FfmpegMediaContainer>(container());
}

SwsContext* FfmpegStreamIterator::getSwsContext(int srcWidth, int srcHeight, AVPixelFormat srcFormat, 
                                              int dstWidth, int dstHeight, AVPixelFormat dstFormat) const {
  // Create a software scaler context for the conversion
  SwsContext* swsContext = sws_getContext(
      srcWidth, srcHeight, srcFormat,
      dstWidth, dstHeight, dstFormat,
      SWS_BILINEAR, nullptr, nullptr, nullptr);

  if (!swsContext) {
    SPDLOG_ERROR("Failed to create SwsContext for format conversion");
  }

  return swsContext;
}

} // namespace Ravl2::Video
