//
// Created on September 6, 2025
//

#include "Ravl2/Video/FfmpegStreamIterator.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include <iostream>
#include <stdexcept>

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace Ravl2::Video {

FfmpegStreamIterator::FfmpegStreamIterator(std::shared_ptr<FfmpegMediaContainer> container, std::size_t streamIndex)
    : StreamIterator(container, streamIndex),
      m_nextFrameId(0),
      m_isAtEnd(false),
      m_currentPositionIndex(0),
      m_wasSeekOperation(false)
  {

  if (!container || !container->isOpen()) {
    throw std::runtime_error("Cannot create iterator: container is not open");
  }

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
    throw std::runtime_error("No codec context available for stream");
  }

  // Read the first frame
  auto result = next();
  if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream) {
    av_frame_free(&m_frame);
    av_packet_free(&m_packet);
    throw std::runtime_error("Failed to read first frame");
  }
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
    return VideoResult<void>(VideoErrorCode::EndOfStream);
  }

  // Reset the last seek operation flag
  m_wasSeekOperation = false;

  // Read the next packet
  auto result = readNextPacket();
  if (!result.isSuccess()) {
    m_isAtEnd = true;
    return result;
  }

  // Decode the packet into a frame
  result = decodeCurrentPacket();
  if (!result.isSuccess()) {
    return result;
  }

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
      MediaTime timestamp(static_cast<int64_t>(1000000.0 * index / audioProps.value().sampleRate));
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

VideoResult<void> FfmpegStreamIterator::readNextPacket() {
  auto& container = ffmpegContainer();

  if (!container.isOpen()) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

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
      return VideoResult<void>(VideoErrorCode::Success);
    }

    // Free the packet if it's not from our stream
    av_packet_unref(m_packet);
  }
}

VideoResult<void> FfmpegStreamIterator::decodeCurrentPacket() {
  if (!m_codecContext || !m_packet || !m_frame) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Send the packet to the decoder
  int result = avcodec_send_packet(m_codecContext, m_packet);

  if (result < 0) {
    return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
  }

  // Receive a frame from the decoder
  result = avcodec_receive_frame(m_codecContext, m_frame);

  if (result < 0) {
    // If the decoder needs more data, that's not an error
    if (result == AVERROR(EAGAIN)) {
      return VideoResult<void>(VideoErrorCode::Success);
    } else {
      return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
    }
  }

  // Convert the FFmpeg frame to our Frame type
  auto frame = convertPacketToFrame();

  if (!frame) {
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
      return createMetadataFrame<std::vector<uint8_t>>();
    }
    default:
      return nullptr;
  }
}

template <typename PixelT>
std::shared_ptr<VideoFrame<PixelT>> FfmpegStreamIterator::createVideoFrame() {
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

  // For simplicity, assume conversion to RGB/YUV is handled elsewhere
  // In a full implementation, you would use swscale to convert from FFmpeg's pixel format

  // Create a 2D array for the frame data
  Array<PixelT, 2> frameData({static_cast<size_t>(m_frame->width), static_cast<size_t>(m_frame->height)});

  // Create a new video frame
  auto videoFrame = std::make_shared<VideoFrame<PixelT>>(frameData, id, timestamp);

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

} // namespace Ravl2::Video
