//
// Created on September 6, 2025
//

#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include "Ravl2/Video/FfmpegMultiStreamIterator.hh"
#include <iostream>

#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#endif


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

      // Register all device input/output handlers
      avdevice_register_all();

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

  VideoResult<std::shared_ptr<MediaContainer>> FfmpegMediaContainer::openDevice(const DeviceParameters& params)
  {
    SPDLOG_DEBUG("openDevice called");

    // Create a new instance of FfmpegMediaContainer
    auto container = std::make_shared<FfmpegMediaContainer>();

    SPDLOG_DEBUG("Container created");

    // Determine the device path and input format based on platform
    // Device path formats:
    //   Linux: "/dev/videoN" (e.g., "/dev/video0")
    //   macOS: "N" where N is device index (e.g., "0" for first camera)
    //   Windows: "video=Device Name" (e.g., "video=Integrated Camera")
    std::string devicePath = params.devicePath;
    const AVInputFormat* inputFormat = nullptr;

#ifdef __linux__
    // Linux: Use V4L2
    if (devicePath.empty())
    {
      devicePath = "/dev/video0";
    }

    SPDLOG_DEBUG("Device path (Linux): {}", devicePath);

    // Find the v4l2 input format (try both names)
    inputFormat = av_find_input_format("v4l2");
    if (!inputFormat)
    {
      inputFormat = av_find_input_format("video4linux2");
    }
    if (!inputFormat)
    {
      SPDLOG_ERROR("V4L2 input format not found. Make sure FFmpeg was compiled with V4L2 support.");
      return VideoResult<std::shared_ptr<MediaContainer>>(VideoErrorCode::UnsupportedFormat);
    }
#elif defined(__APPLE__)
    // macOS: Use AVFoundation
    if (devicePath.empty())
    {
      devicePath = "0"; // Default to first camera
    }

    SPDLOG_DEBUG("Device path (macOS): {}", devicePath);

    inputFormat = av_find_input_format("avfoundation");
    if (!inputFormat)
    {
      SPDLOG_ERROR("AVFoundation input format not found. Make sure FFmpeg was compiled with AVFoundation support.");
      return VideoResult<std::shared_ptr<MediaContainer>>(VideoErrorCode::UnsupportedFormat);
    }
#elif defined(_WIN32)
    // Windows: Use DirectShow
    if (devicePath.empty())
    {
      devicePath = "video=Integrated Camera"; // Common default
    }

    SPDLOG_DEBUG("Device path (Windows): {}", devicePath);

    inputFormat = av_find_input_format("dshow");
    if (!inputFormat)
    {
      SPDLOG_ERROR("DirectShow input format not found. Make sure FFmpeg was compiled with DirectShow support.");
      return VideoResult<std::shared_ptr<MediaContainer>>(VideoErrorCode::UnsupportedFormat);
    }
#else
    SPDLOG_ERROR("Video capture not supported on this platform");
    return VideoResult<std::shared_ptr<MediaContainer>>(VideoErrorCode::NotImplemented);
#endif

    // Set up device options
    AVDictionary* options = nullptr;

    // Set resolution if specified
    if (params.width > 0 && params.height > 0)
    {
      std::string videoSize = std::to_string(params.width) + "x" + std::to_string(params.height);
      av_dict_set(&options, "video_size", videoSize.c_str(), 0);
    }

    // Set frame rate if specified
    if (params.frameRate > 0.0f)
    {
      // Round to nearest integer for better compatibility with device constraints
      // AVFoundation and other capture APIs typically support exact integer frame rates
      int roundedFrameRate = static_cast<int>(params.frameRate + 0.5f);
      std::string frameRate = std::to_string(roundedFrameRate);
      av_dict_set(&options, "framerate", frameRate.c_str(), 0);

      SPDLOG_DEBUG("Setting frame rate: {} (rounded from {})", frameRate, params.frameRate);
    }
#ifdef __APPLE__
    else
    {
      // On macOS, explicitly set a common default frame rate to avoid issues
      // Most webcams support 30 fps as a safe default
      av_dict_set(&options, "framerate", "30", 0);
      SPDLOG_DEBUG("Using default frame rate of 30 fps for AVFoundation");
    }
#endif

    // Set pixel format if specified
    // Note: On macOS, AVFoundation will automatically select a compatible format if not specified
    if (!params.pixelFormat.empty())
    {
      av_dict_set(&options, "pixel_format", params.pixelFormat.c_str(), 0);
    }

#ifdef __APPLE__
    // macOS-specific options for better device initialization
    // Increase probesize to properly detect stream parameters
    av_dict_set(&options, "probesize", "10M", 0);
    // Allow more time for stream analysis
    av_dict_set(&options, "analyzeduration", "2000000", 0);
#endif

    // Open the device
    AVFormatContext* formatContext = nullptr;
    int result = avformat_open_input(&formatContext, devicePath.c_str(), inputFormat, &options);

    // Free the option dictionary
    av_dict_free(&options);

    if (result < 0)
    {
      char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(result, errorBuffer, AV_ERROR_MAX_STRING_SIZE);
      SPDLOG_ERROR("FFmpeg error: Could not open capture device: {} - {}", devicePath, errorBuffer);
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

    // Initialise codec contexts for each stream
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

  VideoResult<std::vector<DeviceInfo>> FfmpegMediaContainer::enumerateDevices()
  {
    std::vector<DeviceInfo> devices;

#ifdef __linux__
    // On Linux, enumerate V4L2 devices
    // Try to open video devices from /dev/video0 to /dev/video31
    for (int i = 0; i < 32; ++i)
    {
      std::string devicePath = "/dev/video" + std::to_string(i);

      // Try to open the device
      int fd = ::open(devicePath.c_str(), O_RDONLY);
      if (fd < 0)
      {
        continue;
      }

      // Query device capabilities
      struct v4l2_capability cap;
      if (::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0)
      {
        // Check if this is a video capture device
        if (cap.device_caps & V4L2_CAP_VIDEO_CAPTURE || cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        {
          DeviceInfo info;
          info.path = devicePath;
          info.name = reinterpret_cast<const char*>(cap.card);
          info.driver = reinterpret_cast<const char*>(cap.driver);
          info.busInfo = reinterpret_cast<const char*>(cap.bus_info);

          // Query supported formats
          struct v4l2_fmtdesc fmtDesc;
          fmtDesc.index = 0;
          fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

          while (::ioctl(fd, VIDIOC_ENUM_FMT, &fmtDesc) == 0)
          {
            info.supportedFormats.push_back(reinterpret_cast<const char*>(fmtDesc.description));
            fmtDesc.index++;
          }

          devices.push_back(info);
        }
      }

      ::close(fd);
    }
#elif defined(__APPLE__)
    // On macOS, enumerate AVFoundation devices using FFmpeg's avdevice API
    const AVInputFormat* inputFormat = av_find_input_format("avfoundation");
    if (!inputFormat)
    {
      SPDLOG_ERROR("AVFoundation input format not found");
      return VideoResult<std::vector<DeviceInfo>>(VideoErrorCode::UnsupportedFormat);
    }

    // Use FFmpeg's device enumeration
    AVDeviceInfoList* deviceList = nullptr;
    AVDictionary* options = nullptr;

    int result = avdevice_list_input_sources(inputFormat, nullptr, options, &deviceList);
    if (result >= 0 && deviceList)
    {
      for (int i = 0; i < deviceList->nb_devices; ++i)
      {
        AVDeviceInfo* devInfo = deviceList->devices[i];

        DeviceInfo info;
        info.path = std::to_string(i);
        info.name = devInfo->device_description ? devInfo->device_description : devInfo->device_name;
        info.driver = "AVFoundation";
        info.busInfo = devInfo->device_name;

        // List common formats supported by AVFoundation
        info.supportedFormats.push_back("YUV 4:2:2");
        info.supportedFormats.push_back("BGRA");

        devices.push_back(info);
      }

      avdevice_free_list_devices(&deviceList);
    }
    else
    {
      SPDLOG_WARN("Failed to enumerate AVFoundation devices");
    }
#elif defined(_WIN32)
    // On Windows, enumerate DirectShow devices using FFmpeg's avdevice API
    const AVInputFormat* inputFormat = av_find_input_format("dshow");
    if (!inputFormat)
    {
      SPDLOG_ERROR("DirectShow input format not found");
      return VideoResult<std::vector<DeviceInfo>>(VideoErrorCode::UnsupportedFormat);
    }

    // Use FFmpeg's device enumeration
    AVDeviceInfoList* deviceList = nullptr;
    AVDictionary* options = nullptr;

    int result = avdevice_list_input_sources(inputFormat, nullptr, options, &deviceList);
    if (result >= 0 && deviceList)
    {
      for (int i = 0; i < deviceList->nb_devices; ++i)
      {
        AVDeviceInfo* devInfo = deviceList->devices[i];

        DeviceInfo info;
        info.path = std::string("video=") + devInfo->device_name;
        info.name = devInfo->device_description ? devInfo->device_description : devInfo->device_name;
        info.driver = "DirectShow";
        info.busInfo = "";

        devices.push_back(info);
      }

      avdevice_free_list_devices(&deviceList);
    }
    else
    {
      SPDLOG_WARN("Failed to enumerate DirectShow devices");
    }
#else
    SPDLOG_WARN("Device enumeration is not supported on this platform");
    return VideoResult<std::vector<DeviceInfo>>(VideoErrorCode::NotImplemented);
#endif

    return VideoResult<std::vector<DeviceInfo>>(devices);
  }

  bool FfmpegMediaContainer::isOpen() const
  {
    return m_formatContext != nullptr;
  }

  VideoResult<void> FfmpegMediaContainer::close()
  {
    std::unique_lock lock(m_mutex);

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
    std::shared_lock lock(m_mutex);
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
    std::shared_lock lock(m_mutex);

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
    std::shared_lock lock(m_mutex);

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
    std::shared_lock lock(m_mutex);

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
    std::shared_lock lock(m_mutex);

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
    std::shared_lock lock(m_mutex);

    if (!m_formatContext || streamIndex >= m_formatContext->nb_streams)
    {
      return VideoResult<std::shared_ptr<StreamIterator>>(VideoErrorCode::InvalidOperation);
    }

    // Create a new iterator for the specified stream
    try
    {
      std::vector<std::size_t> streamsIndexList = { streamIndex };
      auto iterator = std::make_shared<FfmpegMultiStreamIterator>(
        std::static_pointer_cast<FfmpegMediaContainer>(shared_from_this()),
        streamsIndexList
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
    std::shared_lock lock(m_mutex);
    return m_metadata;
  }

  std::string FfmpegMediaContainer::metadata(const std::string&key) const
  {
    std::shared_lock lock(m_mutex);

    auto it = m_metadata.find(key);
    if (it != m_metadata.end())
    {
      return it->second;
    }

    return "";
  }

  bool FfmpegMediaContainer::hasMetadata(const std::string&key) const
  {
    std::shared_lock lock(m_mutex);
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
