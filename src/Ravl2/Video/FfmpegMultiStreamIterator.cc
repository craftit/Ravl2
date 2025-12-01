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
#include <fmt/format.h>

// Optional GoPro GPMF support
#ifdef WITH_GPMF
#include "Ravl2/GoPro/GpmfParser.hh"
#endif

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace Ravl2::Video
{
  namespace
  {
    std::string fourCC2str(uint32_t fourCC)
    {
      // Interpret FourCC as four bytes in little-endian order (FFmpeg stores codec_tag this way).
      char cc[5];
      cc[0] = static_cast<char>((fourCC      ) & 0xFF);
      cc[1] = static_cast<char>((fourCC >>  8) & 0xFF);
      cc[2] = static_cast<char>((fourCC >> 16) & 0xFF);
      cc[3] = static_cast<char>((fourCC >> 24) & 0xFF);
      cc[4] = '\0';

      // If all characters are printable ASCII, return the 4-char string
      bool printable = true;
      for (int i = 0; i < 4; ++i)
      {
        unsigned char ch = static_cast<unsigned char>(cc[i]);
        if (ch < 0x20 || ch > 0x7E) { printable = false; break; }
      }

      if (printable)
      {
        return std::string(cc);
      }

      // Otherwise return a hex representation so callers can still see the value
      return fmt::format("0x{:08x}", fourCC);
    }

    template<typename SampleT>
    bool copyPlanarAudioSamples(const AVFrame* frame, Array<SampleT, 2>& audioData, int channels)
    {
      auto* dest = audioData.origin_address();
      const int sampleStride = audioData.stride(0);
      const int channelStride = audioData.stride(1);

      for (int channel = 0; channel < channels; ++channel)
      {
        const auto* channelSrc = reinterpret_cast<const SampleT*>(frame->extended_data[channel]);
        if (!channelSrc)
        {
          SPDLOG_ERROR("Missing planar audio data for channel {}", channel);
          return false;
        }
        for (int sample = 0; sample < frame->nb_samples; ++sample)
        {
          dest[sample * sampleStride + channel * channelStride] = channelSrc[sample];
        }
      }
      return true;
    }

    template<typename SampleT>
    bool copyInterleavedAudioSamples(const AVFrame* frame, Array<SampleT, 2>& audioData, int channels)
    {
      auto* dest = audioData.origin_address();
      const int sampleStride = audioData.stride(0);
      const int channelStride = audioData.stride(1);
      const auto* src = reinterpret_cast<const SampleT*>(frame->extended_data[0]);
      if (!src)
      {
        SPDLOG_ERROR("Missing interleaved audio data");
        return false;
      }

      for (int sample = 0; sample < frame->nb_samples; ++sample)
      {
        const int baseIndex = sample * channels;
        for (int channel = 0; channel < channels; ++channel)
        {
          dest[sample * sampleStride + channel * channelStride] = src[baseIndex + channel];
        }
      }
      return true;
    }

    template<typename SampleT>
    bool copyAudioSamples(const AVFrame* frame, Array<SampleT, 2>& audioData, int channels, AVSampleFormat fmt)
    {
      const bool isPlanar = av_sample_fmt_is_planar(fmt) != 0;
      if (isPlanar)
      {
        return copyPlanarAudioSamples(frame, audioData, channels);
      }
      return copyInterleavedAudioSamples(frame, audioData, channels);
    }
  }

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

      // DATA streams (like GPMF) may not have a codec context - that's okay
      bool isDataStream = stream && stream->codecpar->codec_type == AVMEDIA_TYPE_DATA;

      if (!codecContext && !isDataStream)
      {
        SPDLOG_WARN("No codec context available for stream {} (type: {} '{}')", streamIndex, static_cast<int>(stream->codecpar->codec_type),fourCC2str(stream->codecpar->codec_tag));
        // Clean upDop
        av_packet_free(&m_packet);
        throw std::runtime_error("No codec context available for stream");
      }
      m_codecContexts.push_back(codecContext);  // May be nullptr for DATA streams

      // Allocate a frame for this stream (not needed for DATA streams, but keep consistent)
      AVFrame* frame = nullptr;
      if (!isDataStream)
      {
        frame = av_frame_alloc();
        if (!frame)
        {
          SPDLOG_WARN("Failed to allocate frame for stream {}", streamIndex);
          // Clean up
          av_packet_free(&m_packet);
          for (auto* f : m_frames)
          {
            if (f) av_frame_free(&f);
          }
          throw std::runtime_error("Failed to allocate frame");
        }
      }
      m_frames.push_back(frame);  // Maybe nullptr for DATA streams

      // Initialise frame ID counter for this stream
      m_nextFrameIds.push_back(0);
    }

    // Update StreamIterator's stream index to the first stream
    mStreamIndex = m_streamIndices[0];

    // Detect if we need to clone frames immediately to avoid buffer pool exhaustion
    // This is required for capture devices with limited buffer pools
    if (m_ffmpegContainer->m_formatContext && m_ffmpegContainer->m_formatContext->iformat)
    {
      const char* formatName = m_ffmpegContainer->m_formatContext->iformat->name;
      if (formatName)
      {
        std::string format(formatName);
        // Check if this is a device input format with limited buffer pools
        // AVFoundation on macOS is known to have buffer pool exhaustion issues
        // Other capture formats (V4L2, DirectShow) may work without cloning
        if (format.find("avfoundation") != std::string::npos)
        {
          m_needsFrameClone = true;
          SPDLOG_DEBUG("Detected AVFoundation input - enabling frame cloning to avoid buffer exhaustion");
        }
        else if (format.find("v4l2") != std::string::npos ||
                 format.find("video4linux") != std::string::npos ||
                 format.find("dshow") != std::string::npos)
        {
          // These formats don't seem to require cloning in practice, but log for awareness
          SPDLOG_DEBUG("Detected device input format '{}' - frame cloning disabled (not required)", format);
        }
      }
    }

#ifdef WITH_GPMF
    // Initialize GPMF parser BEFORE filling packet queue or reading frames
    // because fillPacketQueue() and next() may call decodePacket() which needs the parser
    m_gpmfParser = std::make_unique<GoPro::GpmfParser>();
    SPDLOG_DEBUG("GPMF parser initialized");
#endif

    // Pre-fill the packet queue before reading the first frame
    auto queueResult = fillPacketQueue();
    if (!queueResult.isSuccess() && queueResult.error() != VideoErrorCode::EndOfStream)
    {
      SPDLOG_WARN("Failed to pre-fill packet queue: {}", toString(queueResult.error()));
    }

    // Calculate appropriate queue size based on stream properties
    calculateQueueSize();

    // Read the first frame
    auto result = next();
    if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream)
    {
      // Clean up
      av_packet_free(&m_packet);
      for (auto* f : m_frames)
      {
        if (f) av_frame_free(&f);
      }
      SPDLOG_WARN("Failed to read first frame: {}", toString(result.error()));
      throw std::runtime_error("Failed to read first frame");
    }
  }

  void FfmpegMultiStreamIterator::calculateQueueSize()
  {
    // Start with a reasonable minimum
    m_minQueueSize = 32;

    // Check each video stream for B-frame reordering requirements
    for (size_t i = 0; i < m_streamIndices.size(); ++i)
    {
      if (i >= m_streams.size() || i >= m_codecContexts.size())
        continue;

      auto* stream = m_streams[i];
      if (!stream || stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
        continue;

      auto* codecContext = m_codecContexts[i];
      if (!codecContext)
        continue;

      // Get actual B-frame reordering distance (NOT GOP size!)
      // GOP size is keyframe interval and is irrelevant for frame ordering
      int maxReorder = 0;

      // Check max_b_frames - this is the actual reordering distance
      if (codecContext->max_b_frames > 0)
      {
        // Need buffer for B-frames plus reference frames
        maxReorder = codecContext->max_b_frames + 2;
        SPDLOG_DEBUG("Video stream {} has max_b_frames: {} (reorder distance: {})",
                     i, codecContext->max_b_frames, maxReorder);
      }
      else if (codecContext->has_b_frames)
      {
        // Codec reports it has B-frames but doesn't specify count
        // Use conservative default
        maxReorder = 16;
        SPDLOG_DEBUG("Video stream {} has B-frames but unknown count, using default: {}",
                     i, maxReorder);
      }
      else
      {
        // No B-frames (e.g., baseline H.264, MJPEG, most webcams)
        // Minimal buffering needed
        maxReorder = 2;
        SPDLOG_DEBUG("Video stream {} has no B-frames, minimal buffering: {}", i, maxReorder);
      }

      // Calculate required size: reordering distance × safety factor
      // Safety factor accounts for multi-frame packets and decoder delays
      std::size_t requiredSize = static_cast<std::size_t>(maxReorder * 4);

      // Multi-stream: add extra buffer for interleaving different streams
      if (m_streamIndices.size() > 1)
      {
        requiredSize += 32;
      }

      m_minQueueSize = std::max(m_minQueueSize, requiredSize);
    }

    // Cap at reasonable maximum (128 frames = ~4s at 30fps, ~600MB for 4K)
    m_minQueueSize = std::min(m_minQueueSize, std::size_t(128));

    SPDLOG_INFO("Set packet queue size to: {} (based on B-frame reordering, not GOP size)",
                m_minQueueSize);
  }

  FfmpegMultiStreamIterator::~FfmpegMultiStreamIterator()
  {
    // Free FFmpeg resources
    for (auto* frame : m_frames)
    {
      if (frame) {
        av_frame_free(&frame);
      }
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
    // Reset the last seek operation flag
    m_wasSeekOperation = false;

    try
    {
      // Ensure the priority queue has enough frames for proper temporal ordering
      // Fill the queue if it's empty or below the minimum threshold
      if (m_packetQueue.empty() || (!m_isAtEnd && m_packetQueue.size() < m_minQueueSize))
      {
        auto fillResult = fillPacketQueue();

        // If filling failed with something other than EndOfStream, return the error
        if (!fillResult.isSuccess() && fillResult.error() != VideoErrorCode::EndOfStream)
        {
          return fillResult;
        }
      }

      // If the queue is empty, we've reached the end
      if (m_packetQueue.empty())
      {
        m_isAtEnd = true;
        return VideoResult<void>(VideoErrorCode::EndOfStream);
      }

      // Pop the next frame in temporal (PTS) order from the priority queue
      PacketInfo nextPacket = m_packetQueue.top();
      m_packetQueue.pop();

      SPDLOG_TRACE("Popped frame from queue: PTS={} us, streamIndex={}, queue size now={}",
                   nextPacket.pts, nextPacket.streamIndex, m_packetQueue.size());

#ifndef NDEBUG
      // Validate timestamp ordering in debug builds
      if (m_lastDeliveredPts >= 0 && nextPacket.pts < m_lastDeliveredPts)
      {
        SPDLOG_WARN("Non-monotonic PTS detected: current={} < previous={}, delta={} us (stream {})",
                    nextPacket.pts, m_lastDeliveredPts,
                    nextPacket.pts - m_lastDeliveredPts, nextPacket.streamIndex);
      }
      m_lastDeliveredPts = nextPacket.pts;
#endif

      // Update the current frame and stream index
      setCurrentFrame(nextPacket.frame);
      m_currentStreamIndex = nextPacket.streamIndex;

      // Update the global stream index from the local index
      if (m_currentStreamIndex < m_streamIndices.size())
      {
        mStreamIndex = m_streamIndices[m_currentStreamIndex];
      }

      // Increment frame counter
      m_frameCounter++;

      return VideoResult<void>();
    }
    catch (std::exception&e)
    {
      SPDLOG_ERROR("Exception caught in next(): {}", e.what());
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
      auto seek_result = avio_seek(container.m_formatContext->pb, keyframe.pos, SEEK_SET);

      if (seek_result < 0)
      {
        SPDLOG_WARN("Byte position seeking failed, falling back to traditional seeking");
        return traditionalSeek(timestamp, flags);
      }

      // Flush buffers for all codec contexts
      flushAllCodecs();

      // Set the flag indicating we've just performed a seek
      m_wasSeekOperation = true;
      m_isAtEnd = false;

      // Clear packet queue after seeking
      std::priority_queue<PacketInfo, std::vector<PacketInfo>, PacketInfoComparator> emptyQueue;
      m_packetQueue.swap(emptyQueue);

#ifndef NDEBUG
      // Reset timestamp validation after seek
      m_lastDeliveredPts = -1;
#endif

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
          // If next() fails immediately after seeking, it might be due to a corrupt frame or
          // invalid data at the seek position. Try seeking to a slightly different position.
          SPDLOG_DEBUG("Failed to get first frame after byte-position seeking, trying alternate approach");
          return traditionalSeek(timestamp, flags);
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

      SPDLOG_DEBUG("Get frame for seek to {}. Seekable: {}", timestamp.count(), container.m_formatContext->pb->seekable);

      // Read the first frame at the new position
      auto nextResult = next();

      // If next() fails immediately after seeking, it might be due to a corrupt frame or
      // invalid data at the seek position. Try seeking to a slightly different position.
      if (!nextResult.isSuccess())
      {
        SPDLOG_DEBUG("Failed to get frame after byte-position seeking, trying traditional seeking");
        return traditionalSeek(timestamp, flags);
      }

      return VideoResult<void>();
    }

    // If byte position seeking isn't supported, fall back to timestamp-based seeking
    SPDLOG_DEBUG("Format doesn't support byte position seeking, falling back to timestamp-based seeking");
    return traditionalSeek(MediaTime(keyframe.pts), flags);
  }

  VideoResult<void> FfmpegMultiStreamIterator::traditionalSeek(MediaTime timestamp, SeekFlags flags)
  {
    auto& container = ffmpegContainer();

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

    // Track if we had a successful seek
    bool seekSuccess = false;
    int result = -1;

    // First try with avformat_seek_file for better precision
    result = avformat_seek_file(
      container.m_formatContext,
      -1,                // stream index, -1 for auto selection
      min_ts,            // minimum timestamp
      timestamp_tb,      // target timestamp
      max_ts,            // maximum timestamp
      avFlags            // flags
    );

    if (result >= 0) {
      seekSuccess = true;
    } else {
      SPDLOG_DEBUG("avformat_seek_file failed, falling back to av_seek_frame: {}",
                   toString(FfmpegMediaContainer::convertFfmpegError(result))
      );

      // Try av_seek_frame with -1 for auto stream selection
      result = av_seek_frame(container.m_formatContext, -1, timestamp_tb, avFlags);

      if (result >= 0) {
        seekSuccess = true;
      } else {
        SPDLOG_DEBUG("av_seek_frame with auto stream selection failed: {}",
                    toString(FfmpegMediaContainer::convertFfmpegError(result))
        );

        // Try seeking on each stream individually
        for (size_t i = 0; i < m_streamIndices.size() && !seekSuccess; ++i) {
          int streamIndex = static_cast<int>(m_streamIndices[i]);
          auto* stream = m_streams[i];

          // Convert timestamp to stream time base
          int64_t stream_ts = av_rescale_q(timestamp.count(),
                                           AVRational{1, AV_TIME_BASE},
                                           stream->time_base);

          result = av_seek_frame(container.m_formatContext, streamIndex, stream_ts, avFlags);
          if (result >= 0) {
            SPDLOG_DEBUG("Successfully seeked using stream {}", streamIndex);
            seekSuccess = true;
            break;
          }
        }

        // If still not successful, try one more approach with different flags
        if (!seekSuccess) {
          // Try with different flags - add AVSEEK_FLAG_BACKWARD to find the nearest keyframe
          int fallbackFlags = avFlags | AVSEEK_FLAG_BACKWARD;

          result = av_seek_frame(container.m_formatContext, -1, timestamp_tb, fallbackFlags);
          if (result >= 0) {
            SPDLOG_DEBUG("Successfully seeked with fallback flags");
            seekSuccess = true;
          } else {
            // One final attempt: try seeking to the beginning as a last resort
            if (timestamp.count() <= 0 || timestamp.count() < AV_TIME_BASE) // If very close to beginning
            {
              SPDLOG_DEBUG("Seeking to beginning as fallback");
              result = av_seek_frame(container.m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);

              if (result >= 0) {
                seekSuccess = true;
              }
            }
          }
        }
      }
    }

    // If all seek attempts failed, return the error
    if (!seekSuccess) {
      SPDLOG_ERROR("All seeking attempts failed for timestamp: {} - error code: {}",
                   timestamp.count(),
                   toString(FfmpegMediaContainer::convertFfmpegError(result))
      );
      return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
    }

    // Flush buffers for all codec contexts regardless of seek result
    flushAllCodecs();

    // Set the flag indicating we've just performed a seek
    m_wasSeekOperation = true;
    m_isAtEnd = false;

    // Clear packet queue after seeking
    std::priority_queue<PacketInfo, std::vector<PacketInfo>, PacketInfoComparator> emptyQueue;
    m_packetQueue.swap(emptyQueue);

#ifndef NDEBUG
    // Reset timestamp validation after seek
    m_lastDeliveredPts = -1;
#endif

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
    auto nextResult = next();

    // If reading the next frame fails, try a different approach
    if (!nextResult.isSuccess() && nextResult.error() != VideoErrorCode::EndOfStream) {
      SPDLOG_DEBUG("Failed to get frame after traditional seeking, trying one more approach");

      // Try seeking to a slightly different timestamp (1 second earlier)
      if (timestamp.count() > AV_TIME_BASE) {
        MediaTime earlierTime(timestamp.count() - AV_TIME_BASE);
        result = av_seek_frame(container.m_formatContext, -1,
                              av_rescale_q(earlierTime.count(), AVRational{1, AV_TIME_BASE}, AVRational{1, AV_TIME_BASE}),
                              AVSEEK_FLAG_BACKWARD);

        if (result >= 0) {
          // Flush codecs again
          flushAllCodecs();

          // Try to get a frame at this position
          nextResult = next();

          // If we got a frame, we consider the seek successful even if it's not at the exact time
          if (nextResult.isSuccess()) {
            SPDLOG_DEBUG("Successfully got frame at earlier position");
            return VideoResult<void>();
          }
        }
      }

      // If all recovery attempts failed, return error
      SPDLOG_ERROR("All seeking and recovery attempts failed");
      return nextResult;
    }

    return VideoResult<void>();
  }

  VideoResult<void> FfmpegMultiStreamIterator::seekToIndex(int64_t index)
  {
    // Try timestamp-based seeking if we know the frame rate (much faster than frame-by-frame)
    for (size_t i = 0; i < m_streams.size(); ++i)
    {
      auto* stream = m_streams[i];
      if (stream && stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0)
      {
        // Calculate estimated timestamp from frame index and frame rate
        double fps = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
        int64_t estimatedTime = static_cast<int64_t>((static_cast<double>(index) / fps) * static_cast<double>(AV_TIME_BASE));

        SPDLOG_DEBUG("Seeking to index {} using estimated timestamp {} us (fps: {})",
                     index, estimatedTime, fps);

        auto result = seek(MediaTime(estimatedTime), SeekFlags::Precise);
        if (result.isSuccess())
        {
          // May not be exactly at index, but close enough for most use cases
          return result;
        }

        SPDLOG_DEBUG("Timestamp-based seek failed, falling back to frame-by-frame");
        break;
      }
    }

    // Fallback to frame-by-frame if no frame rate available or timestamp seek failed
    SPDLOG_DEBUG("Using frame-by-frame seeking to index {}", index);

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
    auto localIndex = static_cast<std::size_t>(id & ((1LL << m_streamBits) - 1));
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
    for (int i = 0; i < MAX_FRAME_SEARCH && !iteratorCopy->isAtEnd(); i++)
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

    // NOTE: Backward frame search not implemented since previous() is not supported.
    // If frame not found going forward, we return NotFound rather than attempting
    // backward search. This is a known limitation.
    SPDLOG_WARN("Could not find frame with ID: {} (backward search not implemented)", id);
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
    flushAllCodecs();

    // Reset state
    m_isAtEnd = false;
    m_frameCounter = 0;

    // Reset frame ID counters
    for (auto&id : m_nextFrameIds)
    {
      id = 0;
    }

#ifndef NDEBUG
    // Reset timestamp validation after reset
    m_lastDeliveredPts = -1;
#endif

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
            if (!codecContext) {
              // No codec context for this stream
              return typeid(void);
            }
            // Determine the pixel format to convert to based on FFmpeg's format
            switch (codecContext->pix_fmt)
            {
              case AV_PIX_FMT_RGB24:
                return typeid(RGBPlanarImage<uint8_t>);
              case AV_PIX_FMT_RGBA:
                return typeid(RGBAPlanarImage<uint8_t>);
              case AV_PIX_FMT_YUV420P:
              case AV_PIX_FMT_YUVJ420P:
                return typeid(YUV420Image<uint8_t>);
              case AV_PIX_FMT_YUV420P10LE:
                return typeid(YUV420Image<uint16_t>);
              case AV_PIX_FMT_YUV422P:
              case AV_PIX_FMT_YUVJ422P:
                return typeid(YUV422Image<uint8_t>);
              case AV_PIX_FMT_YUV444P:
              case AV_PIX_FMT_YUVJ444P:
                return typeid(YUV444Image<uint8_t>);
              case AV_PIX_FMT_YUYV422:
                return typeid(Array<PixelYUYV8,2>);
              case AV_PIX_FMT_UYVY422:
                return typeid(Array<PixelUYVY8,2>);
              case AV_PIX_FMT_GRAY8:
                return typeid(Array<PixelI8,2>);
              default:
                SPDLOG_ERROR("Unsupported pixel format {} ", static_cast<int>(codecContext->pix_fmt));
                RavlAlwaysAssertMsg(false, "Unsupported pixel format");
                // Default to RGB for other formats
                return typeid(RGBPlanarImage<uint8_t>);
            }
          }
        case StreamType::Audio:
          {
            if (!codecContext) {
              // No codec context for this stream
              return typeid(void);
            }
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


  VideoResult<std::vector<std::shared_ptr<Frame>>> FfmpegMultiStreamIterator::decodePacket(AVPacket* packet, std::size_t localIndex)
  {
    if (localIndex >= m_codecContexts.size() || localIndex >= m_frames.size())
    {
      return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::InvalidArgument);
    }

    auto* codecContext = m_codecContexts[localIndex];
    auto* frame = m_frames[localIndex];

    if (!packet)
    {
      return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::InvalidOperation);
    }

#ifdef WITH_GPMF
    // Handle DATA streams (e.g., GoPro GPMF metadata)
    // Check if this is a DATA stream first, regardless of codec context
    if (localIndex < m_streams.size())
    {
      auto* stream = m_streams[localIndex];
      if (stream && stream->codecpar->codec_type == AVMEDIA_TYPE_DATA)
      {
        // Check if this is a GPMF metadata stream (codec_tag should be 'gpmd')
        // In FFmpeg, codec_tag is stored as a FourCC: 'gpmd' = 0x646d7067
        constexpr uint32_t GPMD_TAG = (static_cast<uint32_t>('g') << 0) | (static_cast<uint32_t>('p') << 8) |
                                       (static_cast<uint32_t>('m') << 16) | (static_cast<uint32_t>('d') << 24);

        if (stream->codecpar->codec_tag != GPMD_TAG)
        {
          // Not a GPMF stream, skip it (might be timecode or other data)
          SPDLOG_INFO("Skipping non-GPMF DATA stream at localIndex={}, codec_tag=0x{:08x} '{}'",
                       localIndex, stream->codecpar->codec_tag,fourCC2str(stream->codecpar->codec_tag));
          return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::NeedMoreData);
        }

        SPDLOG_INFO("Processing GPMF stream at localIndex={}, packet size={} bytes",
                     localIndex, packet->size);

        // Ensure parser is initialized
        if (!m_gpmfParser)
        {
          SPDLOG_ERROR("GPMF parser not initialized");
          return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::InvalidOperation);
        }

        // Calculate timestamp
        MediaTime timestamp(0);
        if (packet->pts != AV_NOPTS_VALUE)
        {
          int64_t pts_us = av_rescale_q(packet->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
          timestamp = MediaTime(pts_us);
        }

        // Generate stream ID based on PTS
        int64_t pts = 0;
        if (packet->pts != AV_NOPTS_VALUE)
        {
          pts = av_rescale_q(packet->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
        }
        else
        {
          pts = m_nextFrameIds[localIndex]++;
        }
        StreamItemId streamId = (pts << m_streamBits) | (static_cast<int64_t>(localIndex) & ((1LL << m_streamBits) - 1));

        // Parse GPMF data - this may return multiple frames (GPS, gyro, accel)
        auto frames = m_gpmfParser->parse(packet->data, static_cast<size_t>(packet->size), streamId, timestamp);
        SPDLOG_INFO("Got {} frames from buffer {} bytes ",frames.size(), packet->size);

        if (frames.empty())
        {
          // No frames parsed, need more data
          return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::NeedMoreData);
        }

        // Return all frames directly
        return VideoResult<std::vector<std::shared_ptr<Frame>>>(frames);
      }
    }
#endif

    if (!codecContext || !frame)
    {
      return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::InvalidOperation);
    }

    // Send the packet to the decoder
    int result = avcodec_send_packet(codecContext, packet);

    if (result < 0)
    {
      SPDLOG_WARN("Error sending packet to decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
      return VideoResult<std::vector<std::shared_ptr<Frame>>>(FfmpegMediaContainer::convertFfmpegError(result));
    }

    // Receive a frame from the decoder
    result = avcodec_receive_frame(codecContext, frame);

    if (result < 0)
    {
      // If the decoder needs more data, that's not an error
      if (result == AVERROR(EAGAIN))
      {
        return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::NeedMoreData);
      }
      SPDLOG_WARN("Error receiving frame from decoder: {}", toString(FfmpegMediaContainer::convertFfmpegError(result)));
      return VideoResult<std::vector<std::shared_ptr<Frame>>>(FfmpegMediaContainer::convertFfmpegError(result));
    }

    // Generate a unique frame ID based on PTS and stream index
    // Note: generateUniqueFrameId() handles counter increment internally for frames without PTS
    StreamItemId id = generateUniqueFrameId(frame, localIndex);

    // Convert the FFmpeg frame to our Frame type
    auto decodedFrame = convertFrameToFrame(frame, localIndex, id);

    if (!decodedFrame)
    {
      SPDLOG_WARN("Failed to convert frame to Frame");
      return VideoResult<std::vector<std::shared_ptr<Frame>>>(VideoErrorCode::DecodingError);
    }

    // For video/audio frames, return a single-element vector
    return VideoResult<std::vector<std::shared_ptr<Frame>>>({decodedFrame});
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

    // Check for overflow - ensure PTS fits in the available bits
    const int64_t maxPts = (INT64_MAX >> m_streamBits);
    if (pts > maxPts)
    {
      SPDLOG_WARN("PTS {} exceeds maximum {} for frame ID encoding with {} stream bits - clamping to max",
                  pts, maxPts, m_streamBits);
      pts = maxPts;  // Clamp to maximum representable value
    }

    // Create a unique ID by combining the pts and stream index
    // Format: [pts value in higher bits] | [stream index in lower bits]
    StreamItemId id = (pts << m_streamBits) | (static_cast<int64_t>(localIndex) & ((1LL << m_streamBits) - 1));

    SPDLOG_DEBUG("Generated frame ID: {} from PTS: {}, stream index: {}, using {} bits for stream index",
                 id, pts, localIndex, m_streamBits);

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
          if (!codecContext) {
            SPDLOG_ERROR("No codec context for video stream");
            return nullptr;
          }
          // Determine the pixel format to convert to based on FFmpeg's format
          switch (codecContext->pix_fmt)
          {
            case AV_PIX_FMT_RGB24:
              return createVideoFrame<RGBPlanarImage<uint8_t>>(frame, localIndex, id);
            case AV_PIX_FMT_RGBA:
              return createVideoFrame<RGBAPlanarImage<uint8_t>>(frame, localIndex, id);
            case AV_PIX_FMT_YUV420P:
            case AV_PIX_FMT_YUVJ420P:  // JPEG-range YUV420 (deprecated, treat as YUV420P)
              return createVideoFrame<YUV420Image<uint8_t>>(frame, localIndex, id);
            case AV_PIX_FMT_YUV420P10LE:
              return createVideoFrame<YUV420Image<uint16_t>>(frame, localIndex, id);
            case AV_PIX_FMT_YUV422P:
            case AV_PIX_FMT_YUVJ422P:  // JPEG-range YUV422 (deprecated, treat as YUV422P)
              return createVideoFrame<YUV422Image<uint8_t>>(frame, localIndex, id);
            case AV_PIX_FMT_YUV444P:
            case AV_PIX_FMT_YUVJ444P:  // JPEG-range YUV444 (deprecated, treat as YUV444P)
              return createVideoFrame<YUV444Image<uint8_t>>(frame, localIndex, id);
            case AV_PIX_FMT_YUYV422:
              return createVideoFrame<Array<PixelYUYV8,2>>(frame, localIndex, id);
            case AV_PIX_FMT_UYVY422:
              return createVideoFrame<Array<PixelUYVY8,2>>(frame, localIndex, id);
            case AV_PIX_FMT_GRAY8:
              return createVideoFrame<Array<PixelI8,2>>(frame, localIndex, id);
            default: {
              std::array<char,128> buff {};
              const char *strOfType = av_get_pix_fmt_string (buff.data(),buff.size(), codecContext->pix_fmt);
              if(strOfType == nullptr) {
                strOfType = "*Unknown*";
              }

              SPDLOG_ERROR("Unsupported pixel format: {} '{}', {} ", static_cast<int>(codecContext->pix_fmt),av_get_pix_fmt_name(codecContext->pix_fmt),strOfType);
              throw std::runtime_error("Unsupported pixel format");
            }
          }
        }
      case StreamType::Audio:
        {
          if (!codecContext) {
            SPDLOG_ERROR("No codec context for audio stream");
            return nullptr;
          }
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

    // Clone frame if needed for device captures to prevent buffer pool exhaustion
    // We clone all frames from device captures because:
    // 1. Frames may be held by user code during multi-threaded processing
    // 2. We can't track when user code releases frames
    // 3. Device buffer pools are very small (3-8 buffers)
    AVFrame* frameToUse = frame;
    AVFrame* clonedFrame = nullptr;
    if (m_needsFrameClone)
    {
      clonedFrame = av_frame_clone(frame);
      if (!clonedFrame)
      {
        SPDLOG_ERROR("Failed to clone frame for device input");
        return nullptr;
      }
      frameToUse = clonedFrame;
      SPDLOG_TRACE("Cloned video frame for device capture");
    }

    // Get the timestamp in our MediaTime format
    MediaTime timestamp(0);

    // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
    int64_t nopts = AV_NOPTS_VALUE;
    if (frameToUse->pts != nopts)
    {
      int64_t pts_us = av_rescale_q(frameToUse->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
      timestamp = MediaTime(pts_us);
    }

    // Create the image data from the frame
    ImageT frameData;
    bool success = makeImage(frameData, frameToUse);

    // Note: If we cloned the frame, DON'T free it here - makeImage() creates shared_ptr
    // references that will manage the frame's lifetime. The frame will be freed when
    // the last reference (in the ImageT) is destroyed.

    if (!success)
    {
      SPDLOG_ERROR("Failed to create image from frame");
      // On error, free the cloned frame if we created one
      if (clonedFrame)
      {
        av_frame_free(&clonedFrame);
      }
      return nullptr;
    }

    // Create a new video frame
    auto videoFrame = std::make_shared<VideoFrame<ImageT>>(frameData, id, timestamp);

    // Set keyframe flag
    videoFrame->setKeyFrame(frameToUse->pict_type == AV_PICTURE_TYPE_I);

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

    // Clone frame if needed for device captures to prevent buffer pool exhaustion
    // We clone all frames from device captures because frames may be held by user code
    AVFrame* frameToUse = frame;
    AVFrame* clonedFrame = nullptr;
    if (m_needsFrameClone)
    {
      clonedFrame = av_frame_clone(frame);
      if (!clonedFrame)
      {
        SPDLOG_ERROR("Failed to clone frame for device input");
        return nullptr;
      }
      frameToUse = clonedFrame;
      SPDLOG_TRACE("Cloned audio frame for device capture");
    }

    // Get number of channels from codec context
    int channels = codecContext->ch_layout.nb_channels;
    if (channels <= 0)
    {
      if (clonedFrame) av_frame_free(&clonedFrame);
      return nullptr;
    }

    // Get the timestamp in our MediaTime format
    MediaTime timestamp(0);

    // Use int64_t comparison instead of direct AV_NOPTS_VALUE to avoid old-style cast warning
    int64_t nopts = AV_NOPTS_VALUE;
    if (frameToUse->pts != nopts)
    {
      int64_t pts_us = av_rescale_q(frameToUse->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
      timestamp = MediaTime(pts_us);
    }

    // Create a 2D array for the audio data (samples x channels)
    Array<SampleT, 2> audioData({static_cast<size_t>(frameToUse->nb_samples), static_cast<size_t>(channels)});

    // Copy audio data from frame to audioData
    if (!copyAudioSamples(frameToUse, audioData, channels, codecContext->sample_fmt))
    {
      SPDLOG_ERROR("Failed to copy audio samples for frame");
      if (clonedFrame) av_frame_free(&clonedFrame);
      return nullptr;
    }

    // Note: Audio data is copied into audioData, so we CAN safely free the cloned frame
    // (unlike video frames where makeImage() creates references to the frame data)
    if (clonedFrame)
    {
      av_frame_free(&clonedFrame);
    }

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
    IndexRange<2> range({{0, height-1}, {0, width-1}});

    // Make a new handle to the frame
    AVFrame* newFrame = av_frame_alloc();
    if (!newFrame)
    {
      SPDLOG_ERROR("Failed to allocate new frame");
      return false;
    }
    if (av_frame_ref(newFrame, frame) != 0) {
      SPDLOG_ERROR("Failed to reference frame");
      av_frame_free(&newFrame);
      return false;
    }

    // Create a shared_ptr for the frame with custom deleter
    // Use AVFrame* as the managed type for clarity
    std::shared_ptr<AVFrame> frameHandle(newFrame, [](AVFrame* f) {
      av_frame_free(&f);
    });

    // Set up each plane in the PlanarImage
    int planeIndex = 0;
    img.forEachPlane([frameHandle, range, &planeIndex]<typename PlaneArgT>(PlaneArgT& plane)
      {
        using PlaneT = std::decay_t<PlaneArgT>;
        auto localRange = PlaneT::scale_type::calculateRange(range);
        using PixelTypeT = typename PlaneT::value_type;

        AVFrame* avFrame = frameHandle.get();
        assert(avFrame->data[planeIndex] != nullptr);

        PixelTypeT* pixelData = reinterpret_cast<PixelTypeT*>(avFrame->data[planeIndex]);
        int stride = avFrame->linesize[planeIndex] / static_cast<int>(sizeof(PixelTypeT));
        RavlAlwaysAssert(avFrame->linesize[planeIndex] % static_cast<int>(sizeof(PixelTypeT)) == 0);

        // Use aliasing constructor: shares ownership with frameHandle but stores pixelData pointer
        std::shared_ptr<PixelTypeT[]> planeHandle(frameHandle, pixelData);

        plane.data() = Array<PixelTypeT, 2>(pixelData,
                                             localRange,
                                             {stride, 1},
                                             planeHandle
        );
        planeIndex++;
      }
    );

    return true;
  }

  template<typename PixelT>
  bool FfmpegMultiStreamIterator::makeImage(Array<PixelT,2>&img,const AVFrame* frame) const
  {
    int width = frame->width;
    int height = frame->height;
    IndexRange<2> range;
    if constexpr (pixelHasChannel<PixelT, ImageChannel::Luminance2>::value) {
      // Two pixels packed into 1.
      range = IndexRange<2>({{0, height-1}, {0, width/2-1}});
    } else {
      range = IndexRange<2>({{0, height-1}, {0, width-1}});
    }

    // Make a new handle to the frame
    AVFrame* newFrame = av_frame_alloc();
    if (!newFrame)
    {
      SPDLOG_ERROR("Failed to allocate new frame");
      return false;
    }
    if (av_frame_ref(newFrame, frame) != 0) {
      SPDLOG_ERROR("Failed to reference frame");
      av_frame_free(&newFrame);
      return false;
    }

    // Create a shared_ptr for the frame with custom deleter
    std::shared_ptr<AVFrame> frameHandle(newFrame, [](AVFrame* f) {
      av_frame_free(&f);
    });

    auto* pixelPtr = reinterpret_cast<PixelT*>(newFrame->data[0]);
    int stride = newFrame->linesize[0] / static_cast<int>(sizeof(PixelT));
    RavlAssert((newFrame->linesize[0] % static_cast<int>(sizeof(PixelT))) == 0);

    SPDLOG_DEBUG("Setting up packed pixel plane ({}) with range {} Data:{} LineSize:{}",
                 typeName(typeid(PixelT)), range,
                 static_cast<void*>(newFrame->data[0]), newFrame->linesize[0]);

    // Use aliasing constructor: shares ownership with frameHandle but stores pixelPtr
    std::shared_ptr<PixelT[]> pixelHandle(frameHandle, pixelPtr);

    img = Ravl2::Array<PixelT, 2>(pixelPtr,
                                   range,
                                   {stride, 1},
                                   pixelHandle
    );

    return true;
  }

  FfmpegMediaContainer& FfmpegMultiStreamIterator::ffmpegContainer() const
  {
    // Cast the container to FfmpegMediaContainer
    return *m_ffmpegContainer;
  }

  void FfmpegMultiStreamIterator::flushAllCodecs()
  {
    // Flush buffers for all codec contexts (skip nullptr for DATA streams)
    for (auto* codecContext : m_codecContexts)
    {
      if (codecContext)
      {
        avcodec_flush_buffers(codecContext);
      }
    }
  }

  VideoResult<void> FfmpegMultiStreamIterator::fillPacketQueue()
  {
    // Clear any existing items in the queue if we just did a seek operation
    if (m_wasSeekOperation)
    {
      // Empty the queue after a seek operation
      std::priority_queue<PacketInfo, std::vector<PacketInfo>, PacketInfoComparator> emptyQueue;
      m_packetQueue.swap(emptyQueue);

      // Reset the seek flag after handling it
      m_wasSeekOperation = false;
    }

    // If we're at the end of all streams, don't try to read more
    // (buffered GPMF frames were already drained when we hit EOF)
    if (m_isAtEnd)
    {
      return VideoResult<void>(VideoErrorCode::EndOfStream);
    }

    // Keep reading packets until we have enough in the queue
    while (m_packetQueue.size() < m_minQueueSize)
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

          // Return success if we still have frames in the queue, otherwise EndOfStream
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
          auto* stream = m_streams[localIndex];

          // Add all frames to the queue (GPMF packets may produce multiple frames)
          for (const auto& frame : frameResult.value())
          {
            // Get the frame's presentation timestamp
            int64_t pts = 0;

            // Priority: frame PTS > packet PTS > packet DTS
            // Frame PTS is set correctly by the decoder and handles B-frames, multi-frame packets, etc.
            if (frame)
            {
              // Use frame timestamp (decoder sets this correctly for each frame)
              // Note: timestamp can legitimately be 0 for the first frame
              pts = frame->timestamp().count();
            }
            else if (m_packet->pts != AV_NOPTS_VALUE)
            {
              // Fallback to packet PTS if frame has no timestamp
              pts = av_rescale_q(m_packet->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
            }
            else if (m_packet->dts != AV_NOPTS_VALUE)
            {
              // Last resort: use DTS (can be incorrect for B-frames, but better than nothing)
              pts = av_rescale_q(m_packet->dts, stream->time_base, AVRational{1, AV_TIME_BASE});
            }
            else
            {
              // Generate synthetic timestamp if nothing available
              SPDLOG_WARN("No timestamp available for frame in stream {}, using synthetic", localIndex);
              pts = m_nextFrameIds[localIndex]++;
            }

            // Add the frame to the queue
            PacketInfo packetInfo{
              frame, // The decoded frame
              localIndex, // Local stream index
              pts // Presentation timestamp
            };
            m_packetQueue.push(packetInfo);
          }
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
    // Initialise with invalid values
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

    // If the keyframe index is empty, return an invalid keyframe
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
          if (keyframesFound > MAX_KEYFRAME_INDEX)
          {
            SPDLOG_WARN("Keyframe index reached limit ({}), stopping early",MAX_KEYFRAME_INDEX);
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
    flushAllCodecs();

    SPDLOG_DEBUG("Keyframe index built with {} keyframes across {} streams",
                 keyframesFound,
                 m_keyframeIndex.size()
    );

    // Mark as built
    m_keyframeIndexBuilt = true;

    return VideoResult<void>();
  }

  namespace
  {
    [[maybe_unused]] bool reg1 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::RGBPlanarImage<uint8_t>>),"Ravl2::Video::VideoFrame<Ravl2::RGBPlanarImage<uint8_t>>");
    [[maybe_unused]] bool reg2 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::RGBAPlanarImage<uint8_t>>), "Ravl2::Video::VideoFrame<Ravl2::RGBAPlanarImage<uint8_t>>");
    [[maybe_unused]] bool reg3 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::YUV420Image<uint8_t>>), "Ravl2::Video::VideoFrame<Ravl2::YUV420Image<uint8_t>>");
    [[maybe_unused]] bool reg4 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::YUV420Image<uint16_t>>), "Ravl2::Video::VideoFrame<Ravl2::YUV420Image<uint16_t>>");
    [[maybe_unused]] bool reg5 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::YUV422Image<uint8_t>>), "Ravl2::Video::VideoFrame<Ravl2::YUV422Image<uint8_t>>");
    [[maybe_unused]] bool reg6 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::YUV444Image<uint8_t>>), "Ravl2::Video::VideoFrame<Ravl2::YUV444Image<uint8_t>>");
    [[maybe_unused]] bool reg7 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::Array<PixelYUYV8,2>>), "Ravl2::Video::VideoFrame<Ravl2::Array<Ravl2::PixelYUYV8,2>>");
    [[maybe_unused]] bool reg8 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::Array<PixelUYVY8,2>>), "Ravl2::Video::VideoFrame<Ravl2::Array<Ravl2::PixelUYVY8,2>>");
    [[maybe_unused]] bool reg9 = registerTypeName(typeid(Ravl2::Video::VideoFrame<Ravl2::Array<PixelI8,2>>), "Ravl2::Video::VideoFrame<Ravl2::Array<Ravl2::PixelI8,2>>");
  }
} // namespace Ravl2::Video
