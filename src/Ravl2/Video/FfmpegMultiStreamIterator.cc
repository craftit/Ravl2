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

namespace Ravl2::Video
{
  FfmpegMultiStreamIterator::FfmpegMultiStreamIterator(std::shared_ptr<FfmpegMediaContainer> container,
                                                       const std::vector<std::size_t>&streamIndices)
    : StreamIterator(container, 0) // Temporary stream index, will be updated when we get the first frame
      , m_ffmpegContainer(std::move(container))
      , m_packetQueue() // Initialize the packet queue
  {
    Ravl2::initPixel();

    // If no stream indices provided, include all available streams
    if (streamIndices.empty())
    {
      for (std::size_t i = 0; i < m_ffmpegContainer->streamCount(); ++i)
      {
        m_streamIndices.push_back(i);
      }
    }
    else
    {
      // Verify each provided stream index is valid
      for (auto index : streamIndices)
      {
        if (index < m_ffmpegContainer->streamCount())
        {
          m_streamIndices.push_back(index);
        }
        else
        {
          SPDLOG_WARN("Stream index {} is out of range (max: {})",
                      index,
                      m_ffmpegContainer->streamCount() - 1
          );
        }
      }
    }

    if (m_streamIndices.empty())
    {
      throw std::runtime_error("No valid streams available for multi-stream iterator");
    }

    {
      // Calculate how many bits we need for the stream index
      // Number of bits = ceil(log2(number of streams))
      const std::size_t numStreams = m_streamIndices.size();
      m_streamBits = 0;
      for (std::size_t temp = numStreams; temp > 0; temp >>= 1)
      {
        m_streamBits++;
      }

      // Ensure we have at least 4 bits for stream index (up to 16 streams)
      // and at most 16 bits (supporting up to 65536 streams)
      m_streamBits = std::max(std::size_t(4), m_streamBits);
      m_streamBits = std::min(std::size_t(16), m_streamBits);

      SPDLOG_DEBUG("Using {} bits for stream index in frame IDs", m_streamBits);
    }

    // Allocate FFmpeg resources
    m_packet = av_packet_alloc();
    if (!m_packet)
    {
      throw std::runtime_error("Failed to allocate packet");
    }

    // Set up resources for each stream
    for (auto streamIndex : m_streamIndices)
    {
      // Get the FFmpeg stream
      AVStream* stream = m_ffmpegContainer->m_formatContext->streams[streamIndex];
      m_streams.push_back(stream);

      // Get the codec context
      AVCodecContext* codecContext = m_ffmpegContainer->m_codecContexts[streamIndex];
      if (!codecContext)
      {
        SPDLOG_WARN("No codec context available for stream {}", streamIndex);
        // Clean up
        av_packet_free(&m_packet);
        throw std::runtime_error("No codec context available for stream");
      }
      m_codecContexts.push_back(codecContext);

      // Allocate a frame for this stream
      AVFrame* frame = av_frame_alloc();
      if (!frame)
      {
        SPDLOG_WARN("Failed to allocate frame for stream {}", streamIndex);
        // Clean up
        av_packet_free(&m_packet);
        for (auto* f : m_frames)
        {
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

    // Pre-fill the packet queue before reading the first frame
    auto queueResult = fillPacketQueue();
    if (!queueResult.isSuccess() && queueResult.error() != VideoErrorCode::EndOfStream)
    {
      SPDLOG_WARN("Failed to pre-fill packet queue: {}", toString(queueResult.error()));
    }

    // Read the first frame
    auto result = next();
    if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream)
    {
      // Clean up
      av_packet_free(&m_packet);
      for (auto* f : m_frames)
      {
        av_frame_free(&f);
      }
      SPDLOG_WARN("Failed to read first frame: {}", toString(result.error()));
      throw std::runtime_error("Failed to read first frame");
    }
  }

  FfmpegMultiStreamIterator::~FfmpegMultiStreamIterator()
  {
    // Free FFmpeg resources
    for (auto* frame : m_frames)
    {
      av_frame_free(&frame);
    }

    if (m_packet)
    {
      av_packet_free(&m_packet);
    }

    // We don't free m_streams or m_codecContexts as they're owned by the container
  }

  bool FfmpegMultiStreamIterator::isAtEnd() const
  {
    return m_isAtEnd;
  }

  VideoResult<void> FfmpegMultiStreamIterator::next()
  {
    // If we're already at the end, return EndOfStream
    if (m_isAtEnd)
    {
      return VideoResult<void>(VideoErrorCode::EndOfStream);
    }

    // Reset the last seek operation flag
    m_wasSeekOperation = false;

    try
    {
      // Keep reading packets until we find one for a stream we're interested in
      while (true)
      {
        // Clear any previous packet data
        av_packet_unref(m_packet);

        // Read a packet
        auto&container = ffmpegContainer();
        int result = av_read_frame(container.m_formatContext, m_packet);

        if (result < 0)
        {
          // Check if we've reached the end of the stream
          if (result == AVERROR_EOF)
          {
            m_isAtEnd = true;
            return VideoResult<void>(VideoErrorCode::EndOfStream);
          }
          else
          {
            return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
          }
        }

        // Check if this packet belongs to one of our streams
        auto streamIndex = static_cast<size_t>(m_packet->stream_index);
        auto it = std::find(m_streamIndices.begin(), m_streamIndices.end(), streamIndex);

        if (it != m_streamIndices.end())
        {
          // This packet is for a stream we're interested in
          auto localIndex = static_cast<std::size_t>(std::distance(m_streamIndices.begin(), it));

          // Try to decode the packet
          auto frameResult = decodePacket(m_packet, localIndex);

          if (frameResult.isSuccess())
          {
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
          if (frameResult.error() == VideoErrorCode::NeedMoreData)
          {
            continue;
          }

          // For other errors, return the error
          return VideoResult<void>(frameResult.error());
        }

        // This packet doesn't belong to any of our streams, discard it
        av_packet_unref(m_packet);
      }
    }
    catch (std::exception&e)
    {
      SPDLOG_ERROR("Exception caught: {}", e.what());
      return VideoResult<void>(VideoErrorCode::DecodingError);
    }
  }

  VideoResult<void> FfmpegMultiStreamIterator::previous()
  {
    // FFmpeg doesn't natively support backwards iteration
    // For proper support, we would need to maintain a buffer of recent frames
    // For now, just return an error
    return VideoResult<void>(VideoErrorCode::NotImplemented);
  }

  VideoResult<void> FfmpegMultiStreamIterator::seek(MediaTime timestamp, SeekFlags flags)
  {
    auto&container = ffmpegContainer();

    if (!container.isOpen())
    {
      SPDLOG_ERROR("Failed to seek: container is not open");
      return VideoResult<void>(VideoErrorCode::InvalidOperation);
    }

    // Build keyframe index if needed and get the nearest keyframe
    KeyframeInfo keyframe = findNearestKeyframe(timestamp, flags);

    // If we couldn't find a suitable keyframe, fall back to traditional seeking
    if (keyframe.pts < 0 || keyframe.pos < 0)
    {
      SPDLOG_DEBUG("Keyframe-aware seeking failed, falling back to traditional seeking");
      return traditionalSeek(timestamp, flags);
    }

    // Log the found keyframe for debugging
    SPDLOG_DEBUG("Found keyframe at PTS: {}, position: {}, stream: {}",
                 keyframe.pts,
                 keyframe.pos,
                 m_streamIndices[keyframe.streamIndex]
    );

    // Use direct byte position seeking if the format supports it
    if (container.m_formatContext->pb && container.m_formatContext->pb->seekable)
    {
      // Seek to the file position of the keyframe
      SPDLOG_DEBUG("Performing byte-position seeking to position {}", keyframe.pos);
      auto result = avio_seek(container.m_formatContext->pb, keyframe.pos, SEEK_SET);

      if (result < 0)
      {
        SPDLOG_WARN("Byte position seeking failed, falling back to traditional seeking");
        return traditionalSeek(timestamp, flags);
      }

      // Flush buffers for all codec contexts
      for (auto* codecContext : m_codecContexts)
      {
        avcodec_flush_buffers(codecContext);
      }

      // Set the flag indicating we've just performed a seek
      m_wasSeekOperation = true;
      m_isAtEnd = false;

      // If we're seeking to a specific keyframe but not exactly at the requested timestamp,
      // we may need to advance to get closer to the target
      if (MediaTime(keyframe.pts) < timestamp && flags != SeekFlags::Previous)
      {
        // Read frames until we reach or exceed the target timestamp
        SPDLOG_DEBUG("Advancing to target timestamp {}", timestamp.count());

        // Read the first frame at the new position
        auto nextResult = next();
        if (!nextResult.isSuccess())
        {
          return nextResult;
        }

        // Keep advancing until we reach or exceed the target timestamp
        while (currentFrame() && currentFrame()->timestamp() < timestamp && !m_isAtEnd)
        {
          nextResult = next();
          if (!nextResult.isSuccess() && nextResult.error() != VideoErrorCode::EndOfStream)
          {
            return nextResult;
          }
        }

        return VideoResult<void>();
      }

      // Read the first frame at the new position
      return next();
    }

    // If byte position seeking isn't supported, fall back to timestamp-based seeking
    SPDLOG_DEBUG("Format doesn't support byte position seeking, falling back to timestamp-based seeking");
    return traditionalSeek(MediaTime(keyframe.pts), flags);
  }

  VideoResult<void> FfmpegMultiStreamIterator::traditionalSeek(MediaTime timestamp, SeekFlags flags)
  {
    auto&container = ffmpegContainer();

    // Convert MediaTime to FFmpeg's time base
    int64_t timestamp_tb = av_rescale_q(timestamp.count(),
                                        AVRational{1, AV_TIME_BASE},
                                        AVRational{1, AV_TIME_BASE}
    );

    // Seek in the container
    int avFlags = 0;
    if (flags == SeekFlags::Keyframe)
    {
      avFlags |= AVSEEK_FLAG_ANY; // Seek to any frame, not just keyframes
    }
    if (flags == SeekFlags::Previous)
    {
      avFlags |= AVSEEK_FLAG_BACKWARD; // Seek backwards
    }

    // Define a range for seeking
    // For precise seeking, we create a small window around the target timestamp
    int64_t min_ts = timestamp_tb - (AV_TIME_BASE / 10); // 100ms before
    int64_t max_ts = timestamp_tb + (AV_TIME_BASE / 10); // 100ms after

    // Ensure min timestamp is not negative
    min_ts = std::max(int64_t(0), min_ts);

    // First try with avformat_seek_file for better precision
    int result = avformat_seek_file(
      container.m_formatContext,
      -1,
      // stream index, -1 for auto selection
      min_ts,
      // minimum timestamp
      timestamp_tb,
      // target timestamp
      max_ts,
      // maximum timestamp
      avFlags // flags
    );

    // If avformat_seek_file fails, fall back to av_seek_frame
    if (result < 0)
    {
      SPDLOG_DEBUG("avformat_seek_file failed, falling back to av_seek_frame: {}",
                   toString(FfmpegMediaContainer::convertFfmpegError(result))
      );

      result = av_seek_frame(container.m_formatContext, -1, timestamp_tb, avFlags);

      if (result < 0)
      {
        SPDLOG_WARN("Error seeking to timestamp: {} ({}) ",
                    toString(FfmpegMediaContainer::convertFfmpegError(result)),
                    result
        );

        // One more attempt: try seeking to the beginning as a last resort
        if (timestamp.count() <= 0)
        {
          SPDLOG_DEBUG("Seeking to beginning as fallback");
          result = av_seek_frame(container.m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);

          if (result < 0)
          {
            return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
          }
        }
        else
        {
          return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
        }
      }
    }

    // Flush buffers for all codec contexts
    for (auto* codecContext : m_codecContexts)
    {
      avcodec_flush_buffers(codecContext);
    }

    // Set the flag indicating we've just performed a seek
    m_wasSeekOperation = true;
    m_isAtEnd = false;

    // Reset frame ID counters for frames without PTS after seeking
    // This ensures we generate appropriate frame IDs that reflect the new position
    for (size_t i = 0; i < m_nextFrameIds.size(); ++i)
    {
      // Estimate the new counter value based on the target timestamp
      // If we know the average frame rate, we could calculate a more precise value
      auto* stream = m_streams[i];
      if (stream && stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0)
      {
        // Calculate approximate frame number based on timestamp and framerate
        double seconds = static_cast<double>(timestamp.count()) / AV_TIME_BASE;
        double fps = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
        m_nextFrameIds[i] = static_cast<StreamItemId>(seconds * fps);
        SPDLOG_DEBUG("Reset frame ID counter for stream {} to {} based on seek to {} seconds",
                    i, m_nextFrameIds[i], seconds);
      }
      else if (timestamp.count() <= 0)
      {
        // If seeking to the beginning, reset to 0
        m_nextFrameIds[i] = 0;
        SPDLOG_DEBUG("Reset frame ID counter for stream {} to 0", i);
      }
      // Otherwise leave the counter as is, which might be better than an arbitrary value
    }

    // Read the next frame at the new position
    return next();
  }

  VideoResult<void> FfmpegMultiStreamIterator::seekToIndex(int64_t index)
  {
    // FFmpeg doesn't have direct frame index seeking for most formats
    // We would need to estimate the timestamp and use timestamp-based seeking

    // For simplicity, we'll reset to the beginning and advance index times
    auto result = reset();
    if (!result.isSuccess())
    {
      return result;
    }

    // Move forward index times
    for (int64_t i = 0; i < index && !m_isAtEnd; ++i)
    {
      result = next();
      if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream)
      {
        return result;
      }
    }

    return VideoResult<void>();
  }

  VideoResult<std::shared_ptr<Frame>> FfmpegMultiStreamIterator::getFrameById(StreamItemId id) const
  {
    // Extract stream local index and PTS from the ID
    // The lower m_streamBits bits contain the stream local index
    std::size_t localIndex = id & ((1LL << m_streamBits) - 1);
    // The higher bits contain the PTS value
    int64_t pts = id >> m_streamBits;

    SPDLOG_DEBUG("Extracting frame with ID: {}, localIndex: {}, PTS: {}", id, localIndex, pts);

    // Verify that the local index is valid
    if (localIndex >= m_streamIndices.size())
    {
      SPDLOG_WARN("Invalid stream local index in frame ID: {}", localIndex);
      return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::InvalidArgument);
    }

    // Create a non-const copy of this to perform seeking and reading
    // This avoids changing the state of the original iterator
    auto iteratorCopy = std::make_shared<FfmpegMultiStreamIterator>(m_ffmpegContainer, m_streamIndices);

    // Create a MediaTime from the PTS
    MediaTime targetTime(pts);

    // First try precise seeking to the timestamp
    auto seekResult = iteratorCopy->seek(targetTime, SeekFlags::Precise);
    if (!seekResult.isSuccess())
    {
      SPDLOG_WARN("Failed to seek to PTS {}: {}", pts, toString(seekResult.error()));
      return VideoResult<std::shared_ptr<Frame>>(seekResult.error());
    }

    // Search for the frame with the matching ID
    // We might need to read a few frames around the target position
    // since our seeking might not land exactly on the right frame

    // Start with the current frame
    if (iteratorCopy->currentFrame() && iteratorCopy->currentFrame()->id() == id)
    {
      return VideoResult<std::shared_ptr<Frame>>(iteratorCopy->currentFrame());
    }

    // Try reading a few frames forward and backward to find the exact match
    // First try reading forward (typically we'll be close but slightly before the target)
    for (int i = 0; i < 30 && !iteratorCopy->isAtEnd(); i++)
    {
      auto nextResult = iteratorCopy->next();
      if (!nextResult.isSuccess() && nextResult.error() != VideoErrorCode::EndOfStream)
      {
        SPDLOG_WARN("Error while searching forward for frame ID {}: {}", id, toString(nextResult.error()));
        break;
      }

      if (iteratorCopy->currentFrame() && iteratorCopy->currentFrame()->id() == id)
      {
        return VideoResult<std::shared_ptr<Frame>>(iteratorCopy->currentFrame());
      }

      // If we've gone past the target PTS by a significant margin, stop searching
      if (iteratorCopy->currentFrame() &&
          iteratorCopy->currentFrame()->timestamp() > targetTime + MediaTime(AV_TIME_BASE / 2)) // 500ms past
      {
        break;
      }
    }

    // If we didn't find it going forward, try going backward
    // First seek again to get back near our target
    seekResult = iteratorCopy->seek(targetTime, SeekFlags::Previous);
    if (!seekResult.isSuccess())
    {
      SPDLOG_WARN("Failed to seek backward to PTS {}: {}", pts, toString(seekResult.error()));
      return VideoResult<std::shared_ptr<Frame>>(seekResult.error());
    }

    // Now search backward
    for (int i = 0; i < 30 && !iteratorCopy->isAtEnd(); i++)
    {
      if (iteratorCopy->currentFrame() && iteratorCopy->currentFrame()->id() == id)
      {
        return VideoResult<std::shared_ptr<Frame>>(iteratorCopy->currentFrame());
      }

      // Try to read previous (if implemented) or break
      auto prevResult = iteratorCopy->previous();
      if (!prevResult.isSuccess())
      {
        // If previous() is not implemented, we can't search backward
        break;
      }

      // If we've gone too far back, stop searching
      if (iteratorCopy->currentFrame() &&
          iteratorCopy->currentFrame()->timestamp() < targetTime - MediaTime(AV_TIME_BASE / 2)) // 500ms before
      {
        break;
      }
    }

    // If all attempts failed, return not found
    SPDLOG_WARN("Could not find frame with ID: {}", id);
    return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::NotFound);
  }

  VideoResult<void> FfmpegMultiStreamIterator::reset()
  {
    auto&container = ffmpegContainer();

    if (!container.isOpen())
    {
      return VideoResult<void>(VideoErrorCode::InvalidOperation);
    }

    // Seek to the beginning of the container
    int result = av_seek_frame(container.m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
    if (result < 0)
    {
      return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
    }

    // Flush buffers for all codec contexts
    for (auto* codecContext : m_codecContexts)
    {
      avcodec_flush_buffers(codecContext);
    }

    // Reset state
    m_isAtEnd = false;
    m_frameCounter = 0;

    // Reset frame ID counters
    for (auto&id : m_nextFrameIds)
    {
      id = 0;
    }

    // Read the first frame
    return next();
  }

  MediaTime FfmpegMultiStreamIterator::duration() const
  {
    auto&container = ffmpegContainer();

    if (!container.isOpen())
    {
      return MediaTime(0);
    }

    // Find the longest duration among all our streams
    MediaTime maxDuration(0);

    for (size_t i = 0; i < m_streams.size(); ++i)
    {
      auto* stream = m_streams[i];

      if (stream->duration != AV_NOPTS_VALUE)
      {
        int64_t duration_us = av_rescale_q(stream->duration, stream->time_base, AVRational{1, AV_TIME_BASE});
        maxDuration = std::max(maxDuration, MediaTime(duration_us));
      }
    }

    // If we couldn't determine from streams, use container duration
    if (maxDuration.count() == 0)
    {
      maxDuration = container.duration();
    }

    return maxDuration;
  }

  bool FfmpegMultiStreamIterator::canSeek() const
  {
    auto&container = ffmpegContainer();

    if (!container.isOpen())
    {
      SPDLOG_DEBUG("Cannot seek: container is not open");
      return false;
    }

    if (!container.m_formatContext || !container.m_formatContext->iformat)
    {
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

    if (formatName)
    {
      // Common seekable formats
      static const std::array<const char *, 5> seekableFormats = {
        "mp4", "mov", "matroska", "mkv", "avi"
      };

      for (const auto* format : seekableFormats)
      {
        // Check if format name contains any of our known seekable formats
        // This handles compound format names like "mov,mp4,m4a,3gp,3g2,mj2"
        if (std::strstr(formatName, format) != nullptr)
        {
          forceSeekable = true;
          SPDLOG_DEBUG("Forcing seeking to be enabled for format: {} (matched: {})", formatName, format);
          break;
        }
      }
    }

    return formatSupportsSeek || forceSeekable;
  }

  int64_t FfmpegMultiStreamIterator::positionIndex() const
  {
    return m_frameCounter;
  }

  std::type_info const& FfmpegMultiStreamIterator::dataType() const
  {
    // Return the data type based on the current stream type
    if (m_currentStreamIndex < m_codecContexts.size())
    {
      auto* codecContext = m_codecContexts[m_currentStreamIndex];
      auto streamType = ffmpegContainer().streamType(m_streamIndices[m_currentStreamIndex]);

      switch (streamType)
      {
        case StreamType::Video:
          {
            // Determine the pixel format to convert to based on FFmpeg's format
            switch (codecContext->pix_fmt)
            {
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
        case StreamType::Audio:
          {
            // Determine the sample format to convert to based on FFmpeg's format
            switch (codecContext->sample_fmt)
            {
              case AV_SAMPLE_FMT_U8:
              case AV_SAMPLE_FMT_U8P:
                return typeid(Ravl2::Array<uint8_t, 2>);
              case AV_SAMPLE_FMT_S16:
              case AV_SAMPLE_FMT_S16P:
                return typeid(Ravl2::Array<int16_t, 2>);
              case AV_SAMPLE_FMT_S32:
              case AV_SAMPLE_FMT_S32P:
                return typeid(Ravl2::Array<int32_t, 2>);
              case AV_SAMPLE_FMT_FLT:
              case AV_SAMPLE_FMT_FLTP:
                return typeid(Ravl2::Array<float, 2>);
              case AV_SAMPLE_FMT_DBL:
              case AV_SAMPLE_FMT_DBLP:
                return typeid(Ravl2::Array<double, 2>);
              default:
                // Default to 16-bit PCM for other formats
                return typeid(Ravl2::Array<int16_t, 2>);
            }
          }
        default:
          return typeid(std::vector<uint8_t>);
      }
    }

    return StreamIterator::dataType();
  }

  std::size_t FfmpegMultiStreamIterator::currentStreamIndex() const
  {
    if (m_currentStreamIndex < m_streamIndices.size())
    {
      return m_streamIndices[m_currentStreamIndex];
    }
    return 0;
  }

  VideoResult<std::shared_ptr<Frame>> FfmpegMultiStreamIterator::decodePacket(AVPacket* packet, std::size_t localIndex)
  {
    if (localIndex >= m_codecContexts.size() || localIndex >= m_frames.size())
    {
      return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::InvalidArgument);
    }

    auto* codecContext = m_codecContexts[localIndex];
    auto* frame = m_frames[localIndex];

    if (!codecContext || !packet || !frame)
    {
      return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::InvalidOperation);
    }

    // Send the packet to the decoder
    int result = avcodec_send_packet(codecContext, packet);

    if (result < 0)
    {
      SPDLOG_WARN("Error sending packet to decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
      return VideoResult<std::shared_ptr<Frame>>(FfmpegMediaContainer::convertFfmpegError(result));
    }

    // Receive a frame from the decoder
    result = avcodec_receive_frame(codecContext, frame);

    if (result < 0)
    {
      // If the decoder needs more data, that's not an error
      if (result == AVERROR(EAGAIN))
      {
        return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::NeedMoreData);
      }
      SPDLOG_WARN("Error receiving frame from decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
      return VideoResult<std::shared_ptr<Frame>>(FfmpegMediaContainer::convertFfmpegError(result));
    }

    // Check if we need to increment the counter for frames without PTS
    if (frame->pts == AV_NOPTS_VALUE)
    {
      // Increment the frame ID counter for this stream since the frame has no PTS
      m_nextFrameIds[localIndex]++;
    }

    // Generate a unique frame ID based on PTS and stream index
    StreamItemId id = generateUniqueFrameId(frame, localIndex);

    // Convert the FFmpeg frame to our Frame type
    auto decodedFrame = convertFrameToFrame(frame, localIndex, id);

    if (!decodedFrame)
    {
      SPDLOG_WARN("Failed to convert frame to Frame");
      return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::DecodingError);
    }

    return VideoResult<std::shared_ptr<Frame>>(decodedFrame);
  }

  StreamItemId FfmpegMultiStreamIterator::generateUniqueFrameId(AVFrame* frame, std::size_t localIndex)

  {
    // Get the pts value
    int64_t pts = 0;
    if (frame->pts != AV_NOPTS_VALUE)
    {
      // Use the frame's PTS
      auto* stream = m_streams[localIndex];
      pts = av_rescale_q(frame->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
    }
    else
    {
      // If no PTS available, use the counter as a fallback
      pts = m_nextFrameIds[localIndex]++;
    }

    // Create a unique ID by combining the pts and stream index
    // Format: [pts value in higher bits] | [stream index in lower bits]
    StreamItemId id = (pts << m_streamBits) | (static_cast<int64_t>(localIndex) & ((1LL << m_streamBits) - 1));

    SPDLOG_DEBUG("Generated frame ID: {} from PTS: {}, stream index: {}, using {} bits for stream index",
                 id, pts, localIndex, streamBits);

    return id;
  }

  std::shared_ptr<Frame> FfmpegMultiStreamIterator::convertFrameToFrame(
    AVFrame* frame,
    std::size_t localIndex,
    StreamItemId id)
  {
    // Ensure localIndex is valid
    if (localIndex >= m_streamIndices.size() || localIndex >= m_codecContexts.size())
    {
      SPDLOG_ERROR("Invalid local index: {}", localIndex);
      return nullptr;
    }

    auto streamIndex = m_streamIndices[localIndex];
    auto* codecContext = m_codecContexts[localIndex];
    auto streamType = ffmpegContainer().streamType(streamIndex);

    // Create the appropriate frame type based on the stream type
    switch (streamType)
    {
      case StreamType::Video:
        {
          // Determine the pixel format to convert to based on FFmpeg's format
          switch (codecContext->pix_fmt)
          {
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
      case StreamType::Audio:
        {
          // Determine the sample format to convert to based on FFmpeg's format
          switch (codecContext->sample_fmt)
          {
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
      case StreamType::Data:
        {
          // For metadata frames, use a binary blob
          return createMetadataFrame<std::vector<std::byte>>(frame, localIndex, id);
        }
      default:
        SPDLOG_WARN("Unknown stream type: {}", toString(streamType));
        return nullptr;
    }
  }

  template<typename ImageT> std::shared_ptr<VideoFrame<ImageT>> FfmpegMultiStreamIterator::createVideoFrame(
    AVFrame* frame,
    std::size_t localIndex,
    StreamItemId id)
  {
    if (!frame || frame->width <= 0 || frame->height <= 0 || localIndex >= m_streams.size())
    {
      return nullptr;
    }

    auto* stream = m_streams[localIndex];

    // Get the timestamp in our MediaTime format
    MediaTime timestamp(0);

    // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
    int64_t nopts = AV_NOPTS_VALUE;
    if (frame->pts != nopts)
    {
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

  template<typename SampleT> std::shared_ptr<AudioChunk<SampleT>> FfmpegMultiStreamIterator::createAudioChunk(
    AVFrame* frame,
    std::size_t localIndex,
    StreamItemId id)
  {
    if (!frame || frame->nb_samples <= 0 || localIndex >= m_streams.size() || localIndex >= m_codecContexts.size())
    {
      return nullptr;
    }

    auto* stream = m_streams[localIndex];
    auto* codecContext = m_codecContexts[localIndex];

    // Get number of channels from codec context
    int channels = codecContext->ch_layout.nb_channels;
    if (channels <= 0)
    {
      return nullptr;
    }

    // Get the timestamp in our MediaTime format
    MediaTime timestamp(0);

    // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
    int64_t nopts = AV_NOPTS_VALUE;
    if (frame->pts != nopts)
    {
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

  template<typename DataT> std::shared_ptr<MetaDataFrame<DataT>> FfmpegMultiStreamIterator::createMetadataFrame(
    AVFrame* frame,
    std::size_t localIndex,
    StreamItemId id)
  {
    if (!frame || localIndex >= m_streams.size())
    {
      return nullptr;
    }

    auto* stream = m_streams[localIndex];

    // Get the timestamp in our MediaTime format
    MediaTime timestamp(0);

    // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
    int64_t nopts = AV_NOPTS_VALUE;
    if (frame->pts != nopts)
    {
      int64_t pts_us = av_rescale_q(frame->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
      timestamp = MediaTime(pts_us);
    }

    // Create metadata container (implementation will vary depending on data type)
    DataT data;

    // Create a new metadata frame
    auto metadataFrame = std::make_shared<MetaDataFrame<DataT>>(data, id, timestamp);

    return metadataFrame;
  }

  template<typename... PlaneTypes> bool FfmpegMultiStreamIterator::makeImage(
    PlanarImage<2, PlaneTypes...>&img,
    const AVFrame* frame) const
  {
    int width = frame->width;
    int height = frame->height;
    IndexRange<2> range({{0, height}, {0, width}});

    // Make a new handle to the frame
    AVFrame* newFrame = av_frame_alloc();
    if (av_frame_ref(newFrame, frame) != 0)
    {
      SPDLOG_ERROR("Failed to allocate new frame");
      return false;
    }

    // Create a shared_ptr with a custom deleter to free the frame when done
    std::shared_ptr avFrameHandle = std::shared_ptr<uint8_t[]>(frame->data[0],
                                                               [newFrame](uint8_t* data) mutable
                                                               {
                                                                 (void)data;
                                                                 av_frame_free(&newFrame);
                                                               }
    );

    // Set up each plane in the PlanarImage
    int planeIndex = 0;
    img.forEachPlane([avFrameHandle,range,&planeIndex,frame]<typename PlaneArgT>(PlaneArgT&plane)
      {
        using PlaneT = std::decay_t<PlaneArgT>;
        auto localRange = PlaneT::scale_type::calculateRange(range);
        //SPDLOG_INFO("Setting up plane {} ({}) with range {} (master range {})", planeIndex, toString(plane.getChannelType()), localRange, range);
        plane.data() = Array<uint8_t, 2>(frame->data[planeIndex],
                                         localRange,
                                         {frame->linesize[planeIndex], 1},
                                         avFrameHandle
        );
        planeIndex++;
      }
    );

    return true;
  }

  FfmpegMediaContainer& FfmpegMultiStreamIterator::ffmpegContainer() const
  {
    // Cast the container to FfmpegMediaContainer
    return *m_ffmpegContainer;
  }

  VideoResult<void> FfmpegMultiStreamIterator::fillPacketQueue()
  {
    // Clear any existing items in the queue if we just did a seek operation
    if (m_wasSeekOperation)
    {
      // Empty the queue after a seek operation
      std::priority_queue<PacketInfo> emptyQueue;
      m_packetQueue.swap(emptyQueue);

      // Reset the seek flag after handling it
      m_wasSeekOperation = false;
    }

    // If we're at the end of all streams, don't try to read more
    if (m_isAtEnd)
    {
      return VideoResult<void>(VideoErrorCode::EndOfStream);
    }

    // Keep reading packets until we have enough in the queue
    while (m_packetQueue.size() < MIN_QUEUE_SIZE)
    {
      // Clear any previous packet data
      av_packet_unref(m_packet);

      // Read a packet
      auto&container = ffmpegContainer();
      int result = av_read_frame(container.m_formatContext, m_packet);

      if (result < 0)
      {
        // Check if we've reached the end of the stream
        if (result == AVERROR_EOF)
        {
          m_isAtEnd = true;
          // We may still have frames in the queue
          return m_packetQueue.empty() ? VideoResult<void>(VideoErrorCode::EndOfStream) : VideoResult<void>();
        }
        else
        {
          // Only return an error if the queue is empty, otherwise we can still use what we have
          return m_packetQueue.empty()
                   ? VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result))
                   : VideoResult<void>();
        }
      }

      // Check if this packet belongs to one of our streams
      auto streamIndex = static_cast<size_t>(m_packet->stream_index);
      auto it = std::find(m_streamIndices.begin(), m_streamIndices.end(), streamIndex);

      if (it != m_streamIndices.end())
      {
        // This packet is for a stream we're interested in
        auto localIndex = static_cast<std::size_t>(std::distance(m_streamIndices.begin(), it));

        // Try to decode the packet
        auto frameResult = decodePacket(m_packet, localIndex);

        if (frameResult.isSuccess())
        {
          // Get the frame's presentation timestamp
          int64_t pts = 0;
          auto* stream = m_streams[localIndex];

          // Get the timestamp from the frame
          if (frameResult.value() && frameResult.value()->timestamp().count() != 0)
          {
            pts = frameResult.value()->timestamp().count();
          }
          else if (m_packet->pts != AV_NOPTS_VALUE)
          {
            // If the frame doesn't have a timestamp, use the packet's timestamp
            pts = av_rescale_q(m_packet->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
          }
          else if (m_packet->dts != AV_NOPTS_VALUE)
          {
            // As a last resort, use the decoding timestamp
            pts = av_rescale_q(m_packet->dts, stream->time_base, AVRational{1, AV_TIME_BASE});
          }

          // Add the frame to the queue
          PacketInfo packetInfo{
            frameResult.value(), // The decoded frame
            localIndex, // Local stream index
            pts // Presentation timestamp
          };
          m_packetQueue.push(packetInfo);
        }
        else if (frameResult.error() == VideoErrorCode::NeedMoreData)
        {
          // If we need more data, continue reading packets
          continue;
        }
        else
        {
          // Log the error but continue with other packets
          SPDLOG_WARN("Error decoding packet: {}", toString(frameResult.error()));
        }
      }

      // Free the packet resources
      av_packet_unref(m_packet);
    }

    return VideoResult<void>();
  }

  FfmpegMultiStreamIterator::KeyframeInfo FfmpegMultiStreamIterator::findNearestKeyframe(
    MediaTime timestamp,
    SeekFlags flags)
  {
    // Initialize with invalid values
    KeyframeInfo nearestKeyframe{-1, -1, false, 0};

    // Build keyframe index if it hasn't been built yet
    if (!m_keyframeIndexBuilt)
    {
      auto result = buildKeyframeIndex();
      if (!result.isSuccess())
      {
        SPDLOG_WARN("Failed to build keyframe index: {}", toString(result.error()));
        return nearestKeyframe;
      }
    }

    // If keyframe index is empty, return invalid keyframe
    if (m_keyframeIndex.empty())
    {
      SPDLOG_DEBUG("Keyframe index is empty");
      return nearestKeyframe;
    }

    // Convert MediaTime to microseconds
    int64_t target_us = timestamp.count();

    // Find the appropriate keyframe based on seek flags
    if (flags == SeekFlags::Previous)
    {
      // Find the nearest keyframe before the target timestamp
      KeyframeInfo bestMatch{-1, -1, false, 0};

      // Check each stream's keyframes
      for (size_t streamIdx = 0; streamIdx < m_keyframeIndex.size(); ++streamIdx)
      {
        const auto&streamKeyframes = m_keyframeIndex[streamIdx];

        // Find the last keyframe with pts <= target_us
        for (const auto&keyframe : streamKeyframes)
        {
          if (keyframe.pts <= target_us && keyframe.pts > bestMatch.pts)
          {
            bestMatch = keyframe;
          }
        }
      }

      return bestMatch;
    }
    else if (flags == SeekFlags::Next)
    {
      // Find the nearest keyframe after the target timestamp
      KeyframeInfo bestMatch{INT64_MAX, -1, false, 0};

      // Check each stream's keyframes
      for (size_t streamIdx = 0; streamIdx < m_keyframeIndex.size(); ++streamIdx)
      {
        const auto&streamKeyframes = m_keyframeIndex[streamIdx];

        // Find the first keyframe with pts >= target_us
        for (const auto&keyframe : streamKeyframes)
        {
          if (keyframe.pts >= target_us && keyframe.pts < bestMatch.pts)
          {
            bestMatch = keyframe;
          }
        }
      }

      // If no keyframe found after the target, return invalid
      if (bestMatch.pts == INT64_MAX)
      {
        return nearestKeyframe;
      }

      return bestMatch;
    }
    else
    {
      // For precise or keyframe seeking, find the closest keyframe
      KeyframeInfo bestMatch{-1, -1, false, 0};
      int64_t minDistance = INT64_MAX;

      // Check each stream's keyframes
      for (size_t streamIdx = 0; streamIdx < m_keyframeIndex.size(); ++streamIdx)
      {
        const auto&streamKeyframes = m_keyframeIndex[streamIdx];

        for (const auto&keyframe : streamKeyframes)
        {
          int64_t distance = std::abs(keyframe.pts - target_us);

          // For precise seeking, prefer keyframes before the target time
          if (flags == SeekFlags::Precise && keyframe.pts > target_us)
          {
            // Add a penalty for keyframes after the target for precise seeking
            distance += AV_TIME_BASE / 4; // Add 250ms penalty
          }

          if (distance < minDistance)
          {
            minDistance = distance;
            bestMatch = keyframe;
          }
        }
      }

      return bestMatch;
    }
  }

  VideoResult<void> FfmpegMultiStreamIterator::buildKeyframeIndex()
  {
    // If index is already built, no need to rebuild
    if (m_keyframeIndexBuilt)
    {
      return VideoResult<void>();
    }

    SPDLOG_DEBUG("Building keyframe index");

    auto&container = ffmpegContainer();

    // Clear any existing index
    m_keyframeIndex.clear();
    m_keyframeIndex.resize(m_streamIndices.size());

    // Need to temporarily store current position
    int64_t currentPos = 0;
    if (container.m_formatContext->pb)
    {
      currentPos = avio_tell(container.m_formatContext->pb);
    }

    // Seek to the beginning
    int result = av_seek_frame(container.m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
    if (result < 0)
    {
      SPDLOG_WARN("Error seeking to beginning for keyframe indexing: {}",
                  toString(FfmpegMediaContainer::convertFfmpegError(result))
      );
      return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
    }

    // Allocate a packet for reading
    AVPacket* packet = av_packet_alloc();
    if (!packet)
    {
      SPDLOG_ERROR("Failed to allocate packet for keyframe indexing");
      return VideoResult<void>(VideoErrorCode::MemoryError);
    }

    // Set for cleanup
    std::unique_ptr<AVPacket, void(*)(AVPacket*)> packetGuard(
      packet,
      [](AVPacket* p) { av_packet_free(&p); }
    );

    // Track the number of keyframes found
    size_t keyframesFound = 0;

    // Read all packets to build keyframe index
    while (av_read_frame(container.m_formatContext, packet) >= 0)
    {
      // Check if this packet is from one of our streams
      auto streamIndex = static_cast<size_t>(packet->stream_index);
      auto it = std::find(m_streamIndices.begin(), m_streamIndices.end(), streamIndex);

      if (it != m_streamIndices.end())
      {
        auto localIndex = static_cast<std::size_t>(std::distance(m_streamIndices.begin(), it));

        // Check if this is a keyframe
        if (packet->flags & AV_PKT_FLAG_KEY)
        {
          // Convert timestamp to microseconds
          int64_t pts_us;
          if (packet->pts != AV_NOPTS_VALUE)
          {
            auto* stream = m_streams[localIndex];
            pts_us = av_rescale_q(packet->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
          }
          else if (packet->dts != AV_NOPTS_VALUE)
          {
            auto* stream = m_streams[localIndex];
            pts_us = av_rescale_q(packet->dts, stream->time_base, AVRational{1, AV_TIME_BASE});
          }
          else
          {
            // Skip packets without timestamp information
            av_packet_unref(packet);
            continue;
          }

          // Store keyframe information
          KeyframeInfo keyframe;
          keyframe.pts = pts_us;
          keyframe.pos = packet->pos;
          keyframe.isKeyframe = true;
          keyframe.streamIndex = localIndex;

          m_keyframeIndex[localIndex].push_back(keyframe);
          keyframesFound++;

          // Limit the number of keyframes to avoid excessive memory usage
          if (keyframesFound > 10000)
          {
            SPDLOG_WARN("Keyframe index reached limit (10000), stopping early");
            break;
          }
        }
      }

      av_packet_unref(packet);
    }

    // Sort keyframes by presentation timestamp for each stream
    for (auto&streamKeyframes : m_keyframeIndex)
    {
      std::sort(streamKeyframes.begin(), streamKeyframes.end());
    }

    // Restore original position
    if (container.m_formatContext->pb && currentPos > 0)
    {
      avio_seek(container.m_formatContext->pb, currentPos, SEEK_SET);
    }

    // Flush all codec contexts to reset state
    for (auto* codecContext : m_codecContexts)
    {
      avcodec_flush_buffers(codecContext);
    }

    SPDLOG_DEBUG("Keyframe index built with {} keyframes across {} streams",
                 keyframesFound,
                 m_keyframeIndex.size()
    );

    // Mark as built
    m_keyframeIndexBuilt = true;

    return VideoResult<void>();
  }
} // namespace Ravl2::Video
