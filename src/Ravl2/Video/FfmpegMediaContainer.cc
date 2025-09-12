//
// Created on September 6, 2025
//

#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include "Ravl2/Video/FfmpegStreamIterator.hh"
#include <iostream>

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace Ravl2::Video
{
  // Static member initialisation
  bool FfmpegMediaContainer::s_ffmpegInitialized = false;

  FfmpegMediaContainer::FfmpegMediaContainer()
    : m_formatContext(nullptr)
  {
    // Initialize FFmpeg libraries
    initializeFfmpeg();
  }

  FfmpegMediaContainer::~FfmpegMediaContainer()
  {
    // Ensure we clean up resources even if close() wasn't called explicitly
    close();
  }

  void FfmpegMediaContainer::initializeFfmpeg()
  {
    // Initialize FFmpeg libraries once
    if (!s_ffmpegInitialized)
    {
      // Register all codecs, demuxers and protocols
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
      av_register_all();
#endif

      avformat_network_init();
      s_ffmpegInitialized = true;
    }
  }

  VideoResult<std::shared_ptr<MediaContainer>> FfmpegMediaContainer::openFile(const std::string&filePath)
  {
    // Create a new instance of FfmpegMediaContainer
    auto container = std::make_shared<FfmpegMediaContainer>();

    // Open the input file
    AVFormatContext* formatContext = nullptr;
    int result = avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr);

    if (result < 0)
    {
      char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(result, errorBuffer, AV_ERROR_MAX_STRING_SIZE);
      SPDLOG_ERROR("FFmpeg error: Could not open input file: {} - {}", filePath, errorBuffer);
      return VideoResult<std::shared_ptr<MediaContainer>>(convertFfmpegError(result));
    }

    // Store the format context
    container->m_formatContext = formatContext;

    // Read stream information
    result = avformat_find_stream_info(formatContext, nullptr);
    if (result < 0)
    {
      container->close();
      return VideoResult<std::shared_ptr<MediaContainer>>(convertFfmpegError(result));
    }

    // Initialize codec contexts for each stream
    container->m_codecContexts.resize(formatContext->nb_streams, nullptr);
    container->m_streamTypes.resize(formatContext->nb_streams, StreamType::Unknown);

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i)
    {
      AVStream* stream = formatContext->streams[i];

      // Find the decoder for this stream
      const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
      if (!codec)
      {
        container->m_streamTypes[i] = StreamType::Unknown;
        continue;
      }

      // Create a new codec context
      AVCodecContext* codecContext = avcodec_alloc_context3(codec);
      if (!codecContext)
      {
        continue;
      }

      // Copy the codec parameters to the codec context
      if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
      {
        avcodec_free_context(&codecContext);
        continue;
      }

      // Open the codec
      if (avcodec_open2(codecContext, codec, nullptr) < 0)
      {
        avcodec_free_context(&codecContext);
        continue;
      }

      // Store the codec context
      container->m_codecContexts[i] = codecContext;

      // Map the FFmpeg stream type to our StreamType enum
      container->m_streamTypes[i] = mapFfmpegStreamType(stream->codecpar->codec_type);
    }

    // Extract metadata from the format context
    container->extractMetadata();

    return VideoResult<std::shared_ptr<MediaContainer>>(std::static_pointer_cast<MediaContainer>(container));
  }

  bool FfmpegMediaContainer::isOpen() const
  {
    return m_formatContext != nullptr;
  }

  VideoResult<void> FfmpegMediaContainer::close()
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Free all codec contexts
    for (auto codecContext : m_codecContexts)
    {
      if (codecContext)
      {
        avcodec_free_context(&codecContext);
      }
    }
    m_codecContexts.clear();

    // Close the format context
    if (m_formatContext)
    {
      avformat_close_input(&m_formatContext);
      m_formatContext = nullptr;
    }

    // Clear metadata
    m_metadata.clear();
    m_streamTypes.clear();

    return VideoResult<void>(VideoErrorCode::Success);
  }

  std::size_t FfmpegMediaContainer::streamCount() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_formatContext ? m_formatContext->nb_streams : 0;
  }

  StreamType FfmpegMediaContainer::streamType(std::size_t streamIndex) const
  {
    if (!m_formatContext || streamIndex >= m_streamTypes.size())
    {
      return StreamType::Unknown;
    }

    return m_streamTypes[streamIndex];
  }

  VideoResult<VideoProperties> FfmpegMediaContainer::videoProperties(std::size_t streamIndex) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_formatContext || streamIndex >= m_formatContext->nb_streams)
    {
      return VideoResult<VideoProperties>(VideoErrorCode::InvalidOperation);
    }

    if (m_streamTypes[streamIndex] != StreamType::Video)
    {
      return VideoResult<VideoProperties>(VideoErrorCode::InvalidOperation);
    }

    AVStream* stream = m_formatContext->streams[streamIndex];
    AVCodecContext* codecContext = m_codecContexts[streamIndex];

    if (!stream || !codecContext)
    {
      return VideoResult<VideoProperties>(VideoErrorCode::ResourceUnavailable);
    }

    VideoProperties props;
    props.width = codecContext->width;
    props.height = codecContext->height;

    // Calculate frame rate (can be variable)
    if (stream->avg_frame_rate.num && stream->avg_frame_rate.den)
    {
      props.frameRate = static_cast<float>(stream->avg_frame_rate.num) / static_cast<float>(stream->avg_frame_rate.den);
    }
    else if (stream->r_frame_rate.num && stream->r_frame_rate.den)
    {
      props.frameRate = static_cast<float>(stream->r_frame_rate.num) / static_cast<float>(stream->r_frame_rate.den);
    }
    else
    {
      props.frameRate = 0.0f;
    }

    props.isVariableFrameRate = (stream->avg_frame_rate.num != stream->r_frame_rate.num ||
      stream->avg_frame_rate.den != stream->r_frame_rate.den);

    // Set codec information
    props.codec = getCodecInfo(codecContext);

    // Duration
    if (stream->duration != AV_NOPTS_VALUE)
    {
      int64_t duration_us = av_rescale_q(stream->duration, stream->time_base, AVRational{1, AV_TIME_BASE});
      props.duration = MediaTime(duration_us);
    }
    else if (m_formatContext->duration != AV_NOPTS_VALUE)
    {
      props.duration = MediaTime(m_formatContext->duration);
    }
    else
    {
      props.duration = MediaTime(0);
    }

    // Estimate total frames if not available
    if (stream->nb_frames > 0)
    {
      props.totalFrames = stream->nb_frames;
    }
    else if (props.duration.count() > 0 && props.frameRate > 0)
    {
      props.totalFrames = static_cast<int64_t>(static_cast<double>(props.duration.count()) * static_cast<double>(props.
        frameRate) / 1000000.0);
    }
    else
    {
      props.totalFrames = 0;
    }

    // Pixel format
    props.pixelFormat = av_get_pix_fmt_name(codecContext->pix_fmt)
                          ? av_get_pix_fmt_name(codecContext->pix_fmt)
                          : "unknown";

    return VideoResult<VideoProperties>(props);
  }

  VideoResult<AudioProperties> FfmpegMediaContainer::audioProperties(std::size_t streamIndex) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_formatContext || streamIndex >= m_formatContext->nb_streams)
    {
      return VideoResult<AudioProperties>(VideoErrorCode::InvalidOperation);
    }

    if (m_streamTypes[streamIndex] != StreamType::Audio)
    {
      return VideoResult<AudioProperties>(VideoErrorCode::InvalidOperation);
    }

    AVStream* stream = m_formatContext->streams[streamIndex];
    AVCodecContext* codecContext = m_codecContexts[streamIndex];

    if (!stream || !codecContext)
    {
      return VideoResult<AudioProperties>(VideoErrorCode::ResourceUnavailable);
    }

    AudioProperties props;
    props.sampleRate = codecContext->sample_rate;
    props.channels = codecContext->ch_layout.nb_channels;

    // Get bits per sample
    switch (codecContext->sample_fmt)
    {
      case AV_SAMPLE_FMT_U8:
      case AV_SAMPLE_FMT_U8P:
        props.bitsPerSample = 8;
        break;
      case AV_SAMPLE_FMT_S16:
      case AV_SAMPLE_FMT_S16P:
        props.bitsPerSample = 16;
        break;
      case AV_SAMPLE_FMT_S32:
      case AV_SAMPLE_FMT_S32P:
      case AV_SAMPLE_FMT_FLT:
      case AV_SAMPLE_FMT_FLTP:
        props.bitsPerSample = 32;
        break;
      case AV_SAMPLE_FMT_DBL:
      case AV_SAMPLE_FMT_DBLP:
        props.bitsPerSample = 64;
        break;
      default:
        props.bitsPerSample = 0;
        break;
    }

    // Set codec information
    props.codec = getCodecInfo(codecContext);

    // Duration
    if (stream->duration != AV_NOPTS_VALUE)
    {
      int64_t duration_us = av_rescale_q(stream->duration, stream->time_base, AVRational{1, AV_TIME_BASE});
      props.duration = MediaTime(duration_us);
    }
    else if (m_formatContext->duration != AV_NOPTS_VALUE)
    {
      props.duration = MediaTime(m_formatContext->duration);
    }
    else
    {
      props.duration = MediaTime(0);
    }

    // Estimate total samples
    if (props.duration.count() > 0 && props.sampleRate > 0)
    {
      props.totalSamples = static_cast<int64_t>(static_cast<double>(props.duration.count() * props.sampleRate) /
        1000000.0);
    }
    else
    {
      props.totalSamples = 0;
    }

    return VideoResult<AudioProperties>(props);
  }

  VideoResult<DataProperties> FfmpegMediaContainer::dataProperties(std::size_t streamIndex) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_formatContext || streamIndex >= m_formatContext->nb_streams)
    {
      return VideoResult<DataProperties>(VideoErrorCode::InvalidOperation);
    }

    if (m_streamTypes[streamIndex] != StreamType::Data)
    {
      return VideoResult<DataProperties>(VideoErrorCode::InvalidOperation);
    }

    AVStream* stream = m_formatContext->streams[streamIndex];

    if (!stream)
    {
      return VideoResult<DataProperties>(VideoErrorCode::ResourceUnavailable);
    }

    DataProperties props;

    // Try to identify the metadata format
    props.format = "UNKNOWN";
    props.dataTypeName = "BINARY";

    // Duration
    if (stream->duration != AV_NOPTS_VALUE)
    {
      int64_t duration_us = av_rescale_q(stream->duration, stream->time_base, AVRational{1, AV_TIME_BASE});
      props.duration = MediaTime(duration_us);
    }
    else if (m_formatContext->duration != AV_NOPTS_VALUE)
    {
      props.duration = MediaTime(m_formatContext->duration);
    }
    else
    {
      props.duration = MediaTime(0);
    }

    // We can't easily determine the total number of metadata items or the sample rate
    props.totalItems = 0;
    props.sampleRate = 0.0f;

    return VideoResult<DataProperties>(props);
  }

  MediaTime FfmpegMediaContainer::duration() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_formatContext)
    {
      return MediaTime(0);
    }

    // Use the container duration if available
    if (m_formatContext->duration != AV_NOPTS_VALUE)
    {
      return MediaTime(m_formatContext->duration);
    }

    // Otherwise, find the longest stream
    MediaTime maxDuration(0);

    for (unsigned int i = 0; i < m_formatContext->nb_streams; ++i)
    {
      AVStream* stream = m_formatContext->streams[i];

      if (stream->duration != AV_NOPTS_VALUE)
      {
        int64_t duration_us = av_rescale_q(stream->duration, stream->time_base, AVRational{1, AV_TIME_BASE});
        maxDuration = std::max(maxDuration, MediaTime(duration_us));
      }
    }

    return maxDuration;
  }

  VideoResult<std::shared_ptr<StreamIterator>> FfmpegMediaContainer::createIterator(std::size_t streamIndex)
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_formatContext || streamIndex >= m_formatContext->nb_streams)
    {
      return VideoResult<std::shared_ptr<StreamIterator>>(VideoErrorCode::InvalidOperation);
    }

    // Create a new iterator for the specified stream
    try
    {
      auto iterator = std::make_shared<FfmpegStreamIterator>(
        std::static_pointer_cast<FfmpegMediaContainer>(shared_from_this()),
        streamIndex
      );

      return VideoResult<std::shared_ptr<StreamIterator>>(
        std::static_pointer_cast<StreamIterator>(iterator)
      );
    }
    catch (const std::exception&e)
    {
      SPDLOG_ERROR("Exception creating stream iterator: {}", e.what());
      return VideoResult<std::shared_ptr<StreamIterator>>(VideoErrorCode::ResourceAllocationError);
    }
  }

  std::map<std::string, std::string> FfmpegMediaContainer::metadata() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metadata;
  }

  std::string FfmpegMediaContainer::metadata(const std::string&key) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_metadata.find(key);
    if (it != m_metadata.end())
    {
      return it->second;
    }

    return "";
  }

  bool FfmpegMediaContainer::hasMetadata(const std::string&key) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metadata.find(key) != m_metadata.end();
  }

  void FfmpegMediaContainer::extractMetadata()
  {
    if (!m_formatContext)
    {
      return;
    }

    // Extract metadata from the format context
    AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_get(m_formatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    {
      m_metadata[tag->key] = tag->value;
    }

    // Extract metadata from each stream
    for (unsigned int i = 0; i < m_formatContext->nb_streams; ++i)
    {
      AVStream* stream = m_formatContext->streams[i];

      if (!stream)
      {
        continue;
      }

      tag = nullptr;
      while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
      {
        // Prefix stream metadata with stream index to avoid collisions
        std::string key = "stream_" + std::to_string(i) + "_" + tag->key;
        m_metadata[key] = tag->value;
      }
    }
  }

  StreamType FfmpegMediaContainer::mapFfmpegStreamType(int ffmpegStreamType)
  {
    switch (ffmpegStreamType)
    {
      case AVMEDIA_TYPE_VIDEO:
        return StreamType::Video;
      case AVMEDIA_TYPE_AUDIO:
        return StreamType::Audio;
      case AVMEDIA_TYPE_SUBTITLE:
        return StreamType::Subtitle;
      case AVMEDIA_TYPE_DATA:
        return StreamType::Data;
      default:
        return StreamType::Unknown;
    }
  }

  CodecInfo FfmpegMediaContainer::getCodecInfo(const AVCodecContext* codecContext)
  {
    CodecInfo info;

    if (!codecContext || !codecContext->codec)
    {
      info.name = "unknown";
      info.longName = "Unknown Codec";
      info.isLossless = false;
      return info;
    }

    info.name = codecContext->codec->name;
    info.longName = codecContext->codec->long_name;

    // Determine if the codec is lossless (simplified check)
    // A more comprehensive check would involve specific codec properties
    info.isLossless = (
      strstr(codecContext->codec->name, "pcm") != nullptr ||
      strstr(codecContext->codec->name, "png") != nullptr ||
      strstr(codecContext->codec->name, "huffyuv") != nullptr ||
      strstr(codecContext->codec->name, "ffv1") != nullptr
    );

    return info;
  }

  VideoErrorCode FfmpegMediaContainer::convertFfmpegError(int ffmpegError)
  {
    // Use pragma to suppress old-style cast warnings from FFmpeg macros
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

    switch (ffmpegError)
    {
      case AVERROR_EOF:
        return VideoErrorCode::EndOfStream;
      case AVERROR(ENOMEM):
        return VideoErrorCode::ResourceAllocationError;
      case AVERROR(EINVAL):
        return VideoErrorCode::InvalidOperation;
      case AVERROR_DECODER_NOT_FOUND:
      case AVERROR_DEMUXER_NOT_FOUND:
      case AVERROR_MUXER_NOT_FOUND:
      case AVERROR_ENCODER_NOT_FOUND:
      case AVERROR_PROTOCOL_NOT_FOUND:
        return VideoErrorCode::UnsupportedFormat;
      case AVERROR_STREAM_NOT_FOUND:
        return VideoErrorCode::ResourceUnavailable;
      case AVERROR(EIO):
        return VideoErrorCode::CorruptedData;
      default:
        break;
    }
#pragma GCC diagnostic pop
    return VideoErrorCode::DecodingError;
  }
} // namespace Ravl2::Video
