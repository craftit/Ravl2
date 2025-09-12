// filepath: /home/charles/src/Ravl2/src/Ravl2/Video/FfmpegMultiStreamIterator.cc
//
// Created on September 12, 2025
//

#include "Ravl2/Video/FfmpegMultiStreamIterator.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <libswscale/swscale.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace Ravl2::Video {

FfmpegMultiStreamIterator::FfmpegMultiStreamIterator(std::shared_ptr<FfmpegMediaContainer> container,
                                                    const std::vector<std::size_t>& streamIndices)
    : StreamIterator(container, 0) // Temporary stream index, will be updated when we get the first frame
    , m_ffmpegContainer(std::move(container))
    , m_packetQueue() // Initialize the packet queue
{
  Ravl2::initPixel();

  // If no stream indices provided, include all available streams
  if (streamIndices.empty()) {
    for (std::size_t i = 0; i < m_ffmpegContainer->streamCount(); ++i) {
      m_streamIndices.push_back(i);
    }
  } else {
    // Verify each provided stream index is valid
    for (auto index : streamIndices) {
      if (index < m_ffmpegContainer->streamCount()) {
        m_streamIndices.push_back(index);
      } else {
        SPDLOG_WARN("Stream index {} is out of range (max: {})",
                   index, m_ffmpegContainer->streamCount() - 1);
      }
    }
  }

  if (m_streamIndices.empty()) {
    throw std::runtime_error("No valid streams available for multi-stream iterator");
  }

  // Allocate FFmpeg resources
  m_packet = av_packet_alloc();
  if (!m_packet) {
    throw std::runtime_error("Failed to allocate packet");
  }

  // Set up resources for each stream
  for (auto streamIndex : m_streamIndices) {
    // Get the FFmpeg stream
    AVStream* stream = m_ffmpegContainer->m_formatContext->streams[streamIndex];
    m_streams.push_back(stream);

    // Get the codec context
    AVCodecContext* codecContext = m_ffmpegContainer->m_codecContexts[streamIndex];
    if (!codecContext) {
      SPDLOG_WARN("No codec context available for stream {}", streamIndex);
      // Clean up
      av_packet_free(&m_packet);
      throw std::runtime_error("No codec context available for stream");
    }
    m_codecContexts.push_back(codecContext);

    // Allocate a frame for this stream
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
      SPDLOG_WARN("Failed to allocate frame for stream {}", streamIndex);
      // Clean up
      av_packet_free(&m_packet);
      for (auto* f : m_frames) {
        av_frame_free(&f);
      }
      throw std::runtime_error("Failed to allocate frame");
    }
    m_frames.push_back(frame);

    // Initialize frame ID counter for this stream
    m_nextFrameIds.push_back(0);
  }

  // Update StreamIterator's stream index to the first stream
  mStreamIndex = m_streamIndices[0];

  // Read the first frame
  auto result = next();
  if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream) {
    // Clean up
    av_packet_free(&m_packet);
    for (auto* f : m_frames) {
      av_frame_free(&f);
    }
    SPDLOG_WARN("Failed to read first frame: {}", toString(result.error()));
    throw std::runtime_error("Failed to read first frame");
  }
}

FfmpegMultiStreamIterator::~FfmpegMultiStreamIterator() {
  // Free FFmpeg resources
  for (auto* frame : m_frames) {
    av_frame_free(&frame);
  }

  if (m_packet) {
    av_packet_free(&m_packet);
  }

  // We don't free m_streams or m_codecContexts as they're owned by the container
}

bool FfmpegMultiStreamIterator::isAtEnd() const {
  return m_isAtEnd;
}

VideoResult<void> FfmpegMultiStreamIterator::next() {
  // If we're already at the end, return EndOfStream
  if (m_isAtEnd) {
    return VideoResult<void>(VideoErrorCode::EndOfStream);
  }

  // Reset the last seek operation flag
  m_wasSeekOperation = false;

  try {
    // Keep reading packets until we find one for a stream we're interested in
    while (true) {
      // Clear any previous packet data
      av_packet_unref(m_packet);

      // Read a packet
      auto& container = ffmpegContainer();
      int result = av_read_frame(container.m_formatContext, m_packet);

      if (result < 0) {
        // Check if we've reached the end of the stream
        if (result == AVERROR_EOF) {
          m_isAtEnd = true;
          return VideoResult<void>(VideoErrorCode::EndOfStream);
        } else {
          return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
        }
      }

      // Check if this packet belongs to one of our streams
      auto streamIndex = static_cast<size_t>(m_packet->stream_index);
      auto it = std::find(m_streamIndices.begin(), m_streamIndices.end(), streamIndex);

      if (it != m_streamIndices.end()) {
        // This packet is for a stream we're interested in
        auto localIndex = static_cast<std::size_t>(std::distance(m_streamIndices.begin(), it));

        // Try to decode the packet
        auto frameResult = decodePacket(m_packet, localIndex);

        if (frameResult.isSuccess()) {
          // Update the current stream index
          m_currentStreamIndex = localIndex;
          mStreamIndex = m_streamIndices[localIndex];

          // Update the current frame
          setCurrentFrame(frameResult.value());

          // Increment frame counter
          m_frameCounter++;

          return VideoResult<void>();
        }

        // If we need more data, continue reading packets
        if (frameResult.error() == VideoErrorCode::NeedMoreData) {
          continue;
        }

        // For other errors, return the error
        return VideoResult<void>(frameResult.error());
      }

      // This packet doesn't belong to any of our streams, discard it
      av_packet_unref(m_packet);
    }
  } catch (std::exception& e) {
    SPDLOG_ERROR("Exception caught: {}", e.what());
    return VideoResult<void>(VideoErrorCode::DecodingError);
  }
}

VideoResult<void> FfmpegMultiStreamIterator::previous() {
  // FFmpeg doesn't natively support backwards iteration
  // For proper support, we would need to maintain a buffer of recent frames
  // For now, just return an error
  return VideoResult<void>(VideoErrorCode::NotImplemented);
}

VideoResult<void> FfmpegMultiStreamIterator::seek(MediaTime timestamp, SeekFlags flags) {
  auto& container = ffmpegContainer();

  if (!container.isOpen()) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Seek in the container
  int avFlags = 0;
  if (flags == SeekFlags::Keyframe) {
    avFlags |= AVSEEK_FLAG_ANY; // Seek to any frame, not just keyframes
  }
  if (flags == SeekFlags::Previous) {
    avFlags |= AVSEEK_FLAG_BACKWARD; // Seek backwards
  }

  // Convert MediaTime to FFmpeg's time base
  int64_t timestamp_tb = av_rescale_q(timestamp.count(),
                                     AVRational{1, AV_TIME_BASE},
                                     AVRational{1, AV_TIME_BASE});

  // Seek to the timestamp
  int result = av_seek_frame(container.m_formatContext, -1, timestamp_tb, avFlags);
  if (result < 0) {
    return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Flush buffers for all codec contexts
  for (auto* codecContext : m_codecContexts) {
    avcodec_flush_buffers(codecContext);
  }

  // Set the flag indicating we've just performed a seek
  m_wasSeekOperation = true;
  m_isAtEnd = false;

  // Read the next frame at the new position
  return next();
}

VideoResult<void> FfmpegMultiStreamIterator::seekToIndex(int64_t index) {
  // FFmpeg doesn't have direct frame index seeking for most formats
  // We would need to estimate the timestamp and use timestamp-based seeking

  // For simplicity, we'll reset to the beginning and advance index times
  auto result = reset();
  if (!result.isSuccess()) {
    return result;
  }

  // Move forward index times
  for (int64_t i = 0; i < index && !m_isAtEnd; ++i) {
    result = next();
    if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream) {
      return result;
    }
  }

  return VideoResult<void>();
}

VideoResult<std::shared_ptr<Frame>> FfmpegMultiStreamIterator::getFrameById([[maybe_unused]] StreamItemId id) const {
  // FFmpeg doesn't have direct frame ID seeking
  // This would require either:
  // 1. Maintaining a cache of frames by ID
  // 2. Seeking to the start and reading until we find the frame with the right ID

  // For now, just return an error
  return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::NotImplemented);
}

VideoResult<void> FfmpegMultiStreamIterator::reset() {
  auto& container = ffmpegContainer();

  if (!container.isOpen()) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Seek to the beginning of the container
  int result = av_seek_frame(container.m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
  if (result < 0) {
    return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Flush buffers for all codec contexts
  for (auto* codecContext : m_codecContexts) {
    avcodec_flush_buffers(codecContext);
  }

  // Reset state
  m_isAtEnd = false;
  m_frameCounter = 0;

  // Reset frame ID counters
  for (auto& id : m_nextFrameIds) {
    id = 0;
  }

  // Read the first frame
  return next();
}

MediaTime FfmpegMultiStreamIterator::duration() const {
  auto& container = ffmpegContainer();

  if (!container.isOpen()) {
    return MediaTime(0);
  }

  // Find the longest duration among all our streams
  MediaTime maxDuration(0);

  for (size_t i = 0; i < m_streams.size(); ++i) {
    auto* stream = m_streams[i];

    if (stream->duration != AV_NOPTS_VALUE) {
      int64_t duration_us = av_rescale_q(stream->duration, stream->time_base, AVRational{1, AV_TIME_BASE});
      maxDuration = std::max(maxDuration, MediaTime(duration_us));
    }
  }

  // If we couldn't determine from streams, use container duration
  if (maxDuration.count() == 0) {
    maxDuration = container.duration();
  }

  return maxDuration;
}

bool FfmpegMultiStreamIterator::canSeek() const {
  auto& container = ffmpegContainer();

  if (!container.isOpen()) {
    SPDLOG_DEBUG("Cannot seek: container is not open");
    return false;
  }

  if (!container.m_formatContext || !container.m_formatContext->iformat) {
    SPDLOG_DEBUG("Cannot seek: format context or input format is null");
    return false;
  }

  // Check if the format supports seeking
  bool formatSupportsSeek = false;
#ifdef AVFMT_UNSEEKABLE
  formatSupportsSeek = !(container.m_formatContext->iformat->flags & AVFMT_UNSEEKABLE);
#endif

  // Some formats might report they don't support seeking but actually do
  // Force seeking to be enabled for common container formats that should support it
  const char* formatName = container.m_formatContext->iformat->name;
  bool forceSeekable = false;
  SPDLOG_DEBUG("Format name: {}", formatName ? formatName : "null");

  if (formatName) {
    // Common seekable formats
    static const std::array<const char*, 5> seekableFormats = {
      "mp4", "mov", "matroska", "mkv", "avi"
    };

    for (const auto* format : seekableFormats) {
      // Check if format name contains any of our known seekable formats
      // This handles compound format names like "mov,mp4,m4a,3gp,3g2,mj2"
      if (std::strstr(formatName, format) != nullptr) {
        forceSeekable = true;
        SPDLOG_DEBUG("Forcing seeking to be enabled for format: {} (matched: {})", formatName, format);
        break;
      }
    }
  }

  return formatSupportsSeek || forceSeekable;
}

int64_t FfmpegMultiStreamIterator::positionIndex() const {
  return m_frameCounter;
}

std::type_info const& FfmpegMultiStreamIterator::dataType() const {
  // Return the data type based on the current stream type
  if (m_currentStreamIndex < m_codecContexts.size()) {
    auto* codecContext = m_codecContexts[m_currentStreamIndex];
    auto streamType = ffmpegContainer().streamType(m_streamIndices[m_currentStreamIndex]);

    switch (streamType) {
      case StreamType::Video: {
        // Determine the pixel format to convert to based on FFmpeg's format
        switch (codecContext->pix_fmt) {
          case AV_PIX_FMT_RGB24:
            return typeid(RGBPlanarImage<uint8_t>);
          case AV_PIX_FMT_RGBA:
            return typeid(RGBAPlanarImage<uint8_t>);
          case AV_PIX_FMT_YUV420P:
            return typeid(YUV420Image<uint8_t>);
          case AV_PIX_FMT_YUV422P:
            return typeid(YUV422Image<uint8_t>);
          case AV_PIX_FMT_YUV444P:
            return typeid(YUV444Image<uint8_t>);
          default:
            // Default to RGB for other formats
            return typeid(RGBPlanarImage<uint8_t>);
        }
      }
      case StreamType::Audio: {
        // Determine the sample format to convert to based on FFmpeg's format
        switch (codecContext->sample_fmt) {
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
        return typeid(std::vector<uint8_t>);
    }
  }

  return StreamIterator::dataType();
}

std::size_t FfmpegMultiStreamIterator::currentStreamIndex() const {
  if (m_currentStreamIndex < m_streamIndices.size()) {
    return m_streamIndices[m_currentStreamIndex];
  }
  return 0;
}

VideoResult<std::shared_ptr<Frame>> FfmpegMultiStreamIterator::decodePacket(AVPacket* packet, std::size_t localIndex) {
  if (localIndex >= m_codecContexts.size() || localIndex >= m_frames.size()) {
    return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::InvalidArgument);
  }

  auto* codecContext = m_codecContexts[localIndex];
  auto* frame = m_frames[localIndex];

  if (!codecContext || !packet || !frame) {
    return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::InvalidOperation);
  }

  // Send the packet to the decoder
  int result = avcodec_send_packet(codecContext, packet);

  if (result < 0) {
    SPDLOG_WARN("Error sending packet to decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
    return VideoResult<std::shared_ptr<Frame>>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Receive a frame from the decoder
  result = avcodec_receive_frame(codecContext, frame);

  if (result < 0) {
    // If the decoder needs more data, that's not an error
    if (result == AVERROR(EAGAIN)) {
      return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::NeedMoreData);
    }
    SPDLOG_WARN("Error receiving frame from decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
    return VideoResult<std::shared_ptr<Frame>>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Get the next frame ID for this stream
  StreamItemId id = m_nextFrameIds[localIndex]++;

  // Convert the FFmpeg frame to our Frame type
  auto decodedFrame = convertFrameToFrame(frame, localIndex, id);

  if (!decodedFrame) {
    SPDLOG_WARN("Failed to convert frame to Frame");
    return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::DecodingError);
  }

  return VideoResult<std::shared_ptr<Frame>>(decodedFrame);
}

std::shared_ptr<Frame> FfmpegMultiStreamIterator::convertFrameToFrame(AVFrame* frame, std::size_t localIndex, StreamItemId id) {
  // Ensure localIndex is valid
  if (localIndex >= m_streamIndices.size() || localIndex >= m_codecContexts.size()) {
    SPDLOG_ERROR("Invalid local index: {}", localIndex);
    return nullptr;
  }

  auto streamIndex = m_streamIndices[localIndex];
  auto* codecContext = m_codecContexts[localIndex];
  auto streamType = ffmpegContainer().streamType(streamIndex);

  // Create the appropriate frame type based on the stream type
  switch (streamType) {
    case StreamType::Video: {
      // Determine the pixel format to convert to based on FFmpeg's format
      switch (codecContext->pix_fmt) {
        case AV_PIX_FMT_RGB24:
          return createVideoFrame<RGBPlanarImage<uint8_t>>(frame, localIndex, id);
        case AV_PIX_FMT_RGBA:
          return createVideoFrame<RGBAPlanarImage<uint8_t>>(frame, localIndex, id);
        case AV_PIX_FMT_YUV420P:
          return createVideoFrame<YUV420Image<uint8_t>>(frame, localIndex, id);
        case AV_PIX_FMT_YUV422P:
          return createVideoFrame<YUV422Image<uint8_t>>(frame, localIndex, id);
        case AV_PIX_FMT_YUV444P:
          return createVideoFrame<YUV444Image<uint8_t>>(frame, localIndex, id);
        default:
          // Default to RGB for other formats
          return createVideoFrame<RGBPlanarImage<uint8_t>>(frame, localIndex, id);
      }
    }
    case StreamType::Audio: {
      // Determine the sample format to convert to based on FFmpeg's format
      switch (codecContext->sample_fmt) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
          return createAudioChunk<uint8_t>(frame, localIndex, id);
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
          return createAudioChunk<int16_t>(frame, localIndex, id);
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
          return createAudioChunk<int32_t>(frame, localIndex, id);
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
          return createAudioChunk<float>(frame, localIndex, id);
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
          return createAudioChunk<double>(frame, localIndex, id);
        default:
          // Default to 16-bit PCM for other formats
          return createAudioChunk<int16_t>(frame, localIndex, id);
      }
    }
    case StreamType::Data: {
      // For metadata frames, use a binary blob
      return createMetadataFrame<std::vector<std::byte>>(frame, localIndex, id);
    }
    default:
      SPDLOG_WARN("Unknown stream type: {}", toString(streamType));
      return nullptr;
  }
}

template <typename ImageT>
std::shared_ptr<VideoFrame<ImageT>> FfmpegMultiStreamIterator::createVideoFrame(AVFrame* frame, std::size_t localIndex, StreamItemId id) {
  if (!frame || frame->width <= 0 || frame->height <= 0 || localIndex >= m_streams.size()) {
    return nullptr;
  }

  auto* stream = m_streams[localIndex];

  // Get the timestamp in our MediaTime format
  MediaTime timestamp(0);

  // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
  int64_t nopts = AV_NOPTS_VALUE;
  if (frame->pts != nopts) {
    int64_t pts_us = av_rescale_q(frame->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
    timestamp = MediaTime(pts_us);
  }

  // Create the image data from the frame
  ImageT frameData;
  makeImage(frameData, frame);

  // Create a new video frame
  auto videoFrame = std::make_shared<VideoFrame<ImageT>>(frameData, id, timestamp);

  // Set keyframe flag
  videoFrame->setKeyFrame(frame->pict_type == AV_PICTURE_TYPE_I);

  return videoFrame;
}

template <typename SampleT>
std::shared_ptr<AudioChunk<SampleT>> FfmpegMultiStreamIterator::createAudioChunk(AVFrame* frame, std::size_t localIndex, StreamItemId id) {
  if (!frame || frame->nb_samples <= 0 || localIndex >= m_streams.size() || localIndex >= m_codecContexts.size()) {
    return nullptr;
  }

  auto* stream = m_streams[localIndex];
  auto* codecContext = m_codecContexts[localIndex];

  // Get number of channels from codec context
  int channels = codecContext->ch_layout.nb_channels;
  if (channels <= 0) {
    return nullptr;
  }

  // Get the timestamp in our MediaTime format
  MediaTime timestamp(0);

  // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
  int64_t nopts = AV_NOPTS_VALUE;
  if (frame->pts != nopts) {
    int64_t pts_us = av_rescale_q(frame->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
    timestamp = MediaTime(pts_us);
  }

  // Create a 2D array for the audio data (samples x channels)
  Array<SampleT, 2> audioData({static_cast<size_t>(frame->nb_samples), static_cast<size_t>(channels)});

  // TODO: Copy audio data from frame to audioData
  // This would depend on the specific audio format and layout

  // Create a new audio chunk
  auto audioChunk = std::make_shared<AudioChunk<SampleT>>(audioData, id, timestamp);

  return audioChunk;
}

template <typename DataT>
std::shared_ptr<MetaDataFrame<DataT>> FfmpegMultiStreamIterator::createMetadataFrame(AVFrame* frame, std::size_t localIndex, StreamItemId id) {
  if (!frame || localIndex >= m_streams.size()) {
    return nullptr;
  }

  auto* stream = m_streams[localIndex];

  // Get the timestamp in our MediaTime format
  MediaTime timestamp(0);

  // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
  int64_t nopts = AV_NOPTS_VALUE;
  if (frame->pts != nopts) {
    int64_t pts_us = av_rescale_q(frame->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
    timestamp = MediaTime(pts_us);
  }

  // Create metadata container (implementation will vary depending on data type)
  DataT data;

  // Create a new metadata frame
  auto metadataFrame = std::make_shared<MetaDataFrame<DataT>>(data, id, timestamp);

  return metadataFrame;
}

template<typename ... PlaneTypes>
bool FfmpegMultiStreamIterator::makeImage(PlanarImage<2, PlaneTypes...>& img, const AVFrame* frame) const {
  int width = frame->width;
  int height = frame->height;
  IndexRange<2> range({{0,height}, {0, width}});

  // Make a new handle to the frame
  AVFrame* newFrame = av_frame_alloc();
  if (av_frame_ref(newFrame, frame) != 0) {
    SPDLOG_ERROR("Failed to allocate new frame");
    return false;
  }

  // Create a shared_ptr with a custom deleter to free the frame when done
  std::shared_ptr avFrameHandle = std::shared_ptr<uint8_t[]>(frame->data[0], [newFrame](uint8_t *data) mutable {
    (void) data;
    av_frame_free(&newFrame);
  });

  // Set up each plane in the PlanarImage
  int planeIndex = 0;
  img.forEachPlane([avFrameHandle,range,&planeIndex,frame]<typename PlaneArgT>(PlaneArgT& plane) {
    using PlaneT = std::decay_t<PlaneArgT>;
    auto localRange = PlaneT::scale_type::calculateRange(range);
    //SPDLOG_INFO("Setting up plane {} ({}) with range {} (master range {})", planeIndex, toString(plane.getChannelType()), localRange, range);
    plane.data() = Array<uint8_t, 2>(frame->data[planeIndex], localRange, {frame->linesize[planeIndex],1}, avFrameHandle);
    planeIndex++;
  });

  return true;
}

FfmpegMediaContainer& FfmpegMultiStreamIterator::ffmpegContainer() const {
  // Cast the container to FfmpegMediaContainer
  return *m_ffmpegContainer;
}

} // namespace Ravl2::Video
