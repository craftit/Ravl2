//
// Compressed media output using FFmpeg.
//

#include "Ravl2/Video/FfmpegMediaWriter.hh"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace Ravl2::Video
{
  namespace
  {
    constexpr AVRational kVideoTimeBase {1, 90000};
    constexpr AVRational kMicrosecondTimeBase {1, 1000000};

    std::string ffmpegError(int error)
    {
      std::array<char, AV_ERROR_MAX_STRING_SIZE> buffer {};
      av_strerror(error, buffer.data(), buffer.size());
      return buffer.data();
    }

    // avcodec_get_supported_config() replaced AVCodec::pix_fmts / sample_fmts in FFmpeg 7.1
    // (libavcodec 61.13.100); the old fields are deprecated there. Use the new API when available.
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 13, 100)
#define RAVL2_HAVE_AVCODEC_GET_SUPPORTED_CONFIG 1
#endif

    //! Formats the encoder accepts; empty means unknown (the encoder accepts anything).
    std::span<const AVPixelFormat> supportedPixelFormats(const AVCodec *codec, const AVCodecContext *context)
    {
#ifdef RAVL2_HAVE_AVCODEC_GET_SUPPORTED_CONFIG
      const void *supported = nullptr;
      int count = 0;
      if(avcodec_get_supported_config(context, codec, AV_CODEC_CONFIG_PIX_FORMAT, 0, &supported, &count) < 0 || !supported || count <= 0)
        return {};
      return {static_cast<const AVPixelFormat *>(supported), static_cast<std::size_t>(count)};
#else
      (void)context;
      const AVPixelFormat *formats = codec->pix_fmts;
      if(!formats)
        return {};
      std::size_t count = 0;
      while(formats[count] != AV_PIX_FMT_NONE)
        ++count;
      return {formats, count};
#endif
    }

    //! Sample formats the encoder accepts; empty means unknown (the encoder accepts anything).
    std::span<const AVSampleFormat> supportedSampleFormats(const AVCodec *codec, const AVCodecContext *context)
    {
#ifdef RAVL2_HAVE_AVCODEC_GET_SUPPORTED_CONFIG
      const void *supported = nullptr;
      int count = 0;
      if(avcodec_get_supported_config(context, codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, &supported, &count) < 0 || !supported || count <= 0)
        return {};
      return {static_cast<const AVSampleFormat *>(supported), static_cast<std::size_t>(count)};
#else
      (void)context;
      const AVSampleFormat *formats = codec->sample_fmts;
      if(!formats)
        return {};
      std::size_t count = 0;
      while(formats[count] != AV_SAMPLE_FMT_NONE)
        ++count;
      return {formats, count};
#endif
    }

    bool isPixelFormatSupported(const AVCodec *codec, const AVCodecContext *context, AVPixelFormat pixelFormat)
    {
      const auto formats = supportedPixelFormats(codec, context);
      return formats.empty() || std::ranges::find(formats, pixelFormat) != formats.end();
    }

    AVPixelFormat firstSupportedPixelFormat(const AVCodec *codec, const AVCodecContext *context)
    {
      const auto formats = supportedPixelFormats(codec, context);
      return formats.empty() ? AV_PIX_FMT_YUV420P : formats.front();
    }

    AVSampleFormat firstSupportedSampleFormat(const AVCodec *codec, const AVCodecContext *context)
    {
      const auto formats = supportedSampleFormats(codec, context);
      return formats.empty() ? AV_SAMPLE_FMT_FLTP : formats.front();
    }

    AVCodecID defaultVideoCodec(const AVOutputFormat *format, const char *path)
    {
      AVCodecID codecId = av_guess_codec(format, nullptr, path, nullptr, AVMEDIA_TYPE_VIDEO);
      if(codecId != AV_CODEC_ID_NONE)
        return codecId;
      if(format->video_codec != AV_CODEC_ID_NONE)
        return format->video_codec;
      return AV_CODEC_ID_H264;
    }

    const AVCodec *findVideoEncoder(const AVOutputFormat *format, const char *path,
                                    const nlohmann::json &options, std::string &codecName, bool &hardware)
    {
      const std::string explicitCodec = options.value("codec", std::string {});
      if(!explicitCodec.empty()) {
        const AVCodec *codec = avcodec_find_encoder_by_name(explicitCodec.c_str());
        if(codec) {
          codecName = codec->name;
          return codec;
        }
        SPDLOG_ERROR("FFmpeg video encoder '{}' was not found", explicitCodec);
        return nullptr;
      }

      const AVCodecID codecId = defaultVideoCodec(format, path);
      const std::string acceleration = options.value("hwAccel", std::string("none"));
      if(acceleration == "auto") {
        std::string family;
        switch(codecId) {
          case AV_CODEC_ID_H264: family = "h264"; break;
          case AV_CODEC_ID_HEVC: family = "hevc"; break;
          case AV_CODEC_ID_AV1: family = "av1"; break;
          default: break;
        }
        if(!family.empty()) {
          for(std::string_view suffix : {"_videotoolbox", "_nvenc", "_qsv", "_amf"}) {
            const std::string candidate = family + std::string(suffix);
            if(const AVCodec *codec = avcodec_find_encoder_by_name(candidate.c_str())) {
              codecName = codec->name;
              hardware = true;
              return codec;
            }
          }
        }
      } else if(acceleration != "none") {
        SPDLOG_WARN("Unknown FFmpeg hardware acceleration mode '{}'; using software encoding", acceleration);
      }

      const AVCodec *codec = nullptr;
      if(codecId == AV_CODEC_ID_H264)
        codec = avcodec_find_encoder_by_name("libx264");
      if(!codec)
        codec = avcodec_find_encoder(codecId);
      if(!codec && format->video_codec != AV_CODEC_ID_NONE)
        codec = avcodec_find_encoder(format->video_codec);
      if(!codec && codecId != AV_CODEC_ID_H264)
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
      if(!codec)
        codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
      if(codec)
        codecName = codec->name;
      return codec;
    }

    bool isAudioEnabled(const nlohmann::json &options)
    {
      if(!options.is_object() || !options.contains("audio"))
        return false;
      const auto &audio = options.at("audio");
      return audio.is_object() || (audio.is_boolean() && audio.get<bool>());
    }

    template <typename ArrayT>
    std::vector<uint8_t> packedPixels(const ArrayT &image, int &width, int &height)
    {
      const auto range = image.range();
      const auto dimensions = range.size();
      height = dimensions[0];
      width = dimensions[1];
      // Padded: sws_scale's SIMD paths read past the last pixel.
      std::vector<uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3 + AV_INPUT_BUFFER_PADDING_SIZE);
      std::size_t offset = 0;
      for(int y = range.min(0); y <= range.max(0); ++y) {
        for(int x = range.min(1); x <= range.max(1); ++x) {
          const auto &pixel = image(y, x);
          pixels[offset++] = pixel[0];
          pixels[offset++] = pixel[1];
          pixels[offset++] = pixel[2];
        }
      }
      return pixels;
    }

    std::vector<uint8_t> planePixels(const Array<uint8_t, 2> &plane)
    {
      const auto range = plane.range();
      const auto dimensions = range.size();
      // Padded: sws_scale's SIMD paths read past the last pixel.
      std::vector<uint8_t> pixels(static_cast<std::size_t>(dimensions[0]) * static_cast<std::size_t>(dimensions[1]) + AV_INPUT_BUFFER_PADDING_SIZE);
      std::size_t offset = 0;
      for(int y = range.min(0); y <= range.max(0); ++y) {
        for(int x = range.min(1); x <= range.max(1); ++x)
          pixels[offset++] = plane(y, x);
      }
      return pixels;
    }
  }

  struct FfmpegMediaWriter::Impl
  {
    std::string path;
    nlohmann::json options;
    AVFormatContext *format = nullptr;
    const AVCodec *videoEncoder = nullptr;
    AVCodecContext *videoCodec = nullptr;
    AVStream *videoStream = nullptr;
    AVFrame *videoFrame = nullptr;
    SwsContext *videoScaler = nullptr;
    const AVCodec *audioEncoder = nullptr;
    AVCodecContext *audioCodec = nullptr;
    AVStream *audioStream = nullptr;
    SwrContext *audioResampler = nullptr;
    AVAudioFifo *audioFifo = nullptr;
    std::vector<AVPacket *> pendingPackets;
    std::string videoCodecName;
    std::string pixelFormatName = "yuv420p";
    std::string preset = "medium";
    std::string audioCodecName = "aac";
    int videoWidth = 0;
    int videoHeight = 0;
    int audioRate = 48000;
    int audioChannels = 2;
    int audioBitrate = 128000;
    int gopSize = 0;
    int64_t videoFrameCount = 0;
    int64_t lastVideoPts = std::numeric_limits<int64_t>::min();
    int64_t audioPts = 0;
    MediaTime mediaStartTime {};
    double fps = 30.0;
    int crf = 23;
    int64_t videoBitrate = 0;
    bool hardware = false;
    bool audioExpected = false;
    bool videoInitialized = false;
    bool audioInitialized = false;
    bool hasAudioStart = false;
    bool hasMediaStart = false;
    bool headerWritten = false;
    bool closed = false;

    ~Impl()
    {
      for(auto *packet : pendingPackets)
        av_packet_free(&packet);
      av_audio_fifo_free(audioFifo);
      swr_free(&audioResampler);
      sws_freeContext(videoScaler);
      av_frame_free(&videoFrame);
      avcodec_free_context(&videoCodec);
      avcodec_free_context(&audioCodec);
      if(format) {
        if(format->pb && !(format->oformat->flags & AVFMT_NOFILE))
          avio_closep(&format->pb);
        avformat_free_context(format);
      }
    }
  };

  namespace
  {
    bool writeMuxPacket(FfmpegMediaWriter::Impl &impl, AVPacket *packet)
    {
      if(!impl.headerWritten)
        return false;
      AVStream *stream = impl.format->streams[packet->stream_index];
      const AVRational sourceTimeBase = packet->time_base;
      av_packet_rescale_ts(packet, sourceTimeBase, stream->time_base);
      packet->time_base = stream->time_base;
      const int result = av_interleaved_write_frame(impl.format, packet);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg failed writing a packet to '{}': {}", impl.path, ffmpegError(result));
        return false;
      }
      return true;
    }

    bool emitPacket(FfmpegMediaWriter::Impl &impl, AVPacket *packet, AVCodecContext *codec, AVStream *stream)
    {
      packet->stream_index = stream->index;
      packet->time_base = codec->time_base;
      if(stream == impl.videoStream && packet->duration <= 0)
        packet->duration = av_rescale_q(1, av_inv_q(codec->framerate), codec->time_base);
      if(!impl.headerWritten) {
        AVPacket *queued = av_packet_clone(packet);
        if(!queued) {
          SPDLOG_ERROR("FFmpeg could not queue a packet for '{}'", impl.path);
          return false;
        }
        impl.pendingPackets.push_back(queued);
        return true;
      }
      return writeMuxPacket(impl, packet);
    }

    bool receivePackets(FfmpegMediaWriter::Impl &impl, AVCodecContext *codec, AVStream *stream)
    {
      AVPacket *packet = av_packet_alloc();
      if(!packet) {
        SPDLOG_ERROR("FFmpeg could not allocate an output packet");
        return false;
      }
      bool success = true;
      for(;;) {
        const int result = avcodec_receive_packet(codec, packet);
        if(result == AVERROR(EAGAIN) || result == AVERROR_EOF)
          break;
        if(result < 0) {
          SPDLOG_ERROR("FFmpeg encoder error for '{}': {}", impl.path, ffmpegError(result));
          success = false;
          break;
        }
        if(!emitPacket(impl, packet, codec, stream))
          success = false;
        av_packet_unref(packet);
        if(!success)
          break;
      }
      av_packet_free(&packet);
      return success;
    }

    bool encodeFrame(FfmpegMediaWriter::Impl &impl, AVCodecContext *codec, AVStream *stream, AVFrame *frame)
    {
      const int result = avcodec_send_frame(codec, frame);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not submit a frame for '{}': {}", impl.path, ffmpegError(result));
        return false;
      }
      return receivePackets(impl, codec, stream);
    }

    bool writeHeaderIfReady(FfmpegMediaWriter::Impl &impl)
    {
      if(impl.headerWritten || !impl.videoInitialized || (impl.audioExpected && !impl.audioInitialized))
        return true;
      const int result = avformat_write_header(impl.format, nullptr);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not write the header for '{}': {}", impl.path, ffmpegError(result));
        return false;
      }
      impl.headerWritten = true;
      // Free every queued packet whether or not it was written, so ~Impl can't free any of them twice.
      bool ok = true;
      for(auto *&packet : impl.pendingPackets) {
        if(ok && !writeMuxPacket(impl, packet))
          ok = false;
        av_packet_free(&packet);
      }
      impl.pendingPackets.clear();
      return ok;
    }

    bool initializeVideo(FfmpegMediaWriter::Impl &impl, int width, int height)
    {
      if(impl.videoInitialized)
        return width == impl.videoWidth && height == impl.videoHeight;
      if(width <= 0 || height <= 0) {
        SPDLOG_ERROR("FFmpeg cannot encode invalid image dimensions {}x{}", width, height);
        return false;
      }
      impl.videoWidth = width;
      impl.videoHeight = height;
      // A previous attempt may have failed part way; don't leak what it allocated.
      avcodec_free_context(&impl.videoCodec);
      av_frame_free(&impl.videoFrame);
      impl.videoCodec = avcodec_alloc_context3(impl.videoEncoder);
      if(!impl.videoCodec) {
        SPDLOG_ERROR("FFmpeg could not allocate the video encoder context");
        return false;
      }
      impl.videoCodec->width = width;
      impl.videoCodec->height = height;
      impl.videoCodec->framerate = av_d2q(impl.fps, 100000);
      impl.videoCodec->time_base = impl.videoEncoder->id == AV_CODEC_ID_MPEG4
                                     ? av_inv_q(impl.videoCodec->framerate)
                                     : kVideoTimeBase;
      impl.videoCodec->gop_size = impl.gopSize > 0 ? impl.gopSize : static_cast<int>(std::round(impl.fps * 2.0));
      impl.videoCodec->bit_rate = impl.videoBitrate;
      const AVPixelFormat requested = av_get_pix_fmt(impl.pixelFormatName.c_str());
      if(requested == AV_PIX_FMT_NONE) {
        SPDLOG_ERROR("Unknown FFmpeg pixel format '{}'", impl.pixelFormatName);
        return false;
      }
      impl.videoCodec->pix_fmt = isPixelFormatSupported(impl.videoEncoder, impl.videoCodec, requested)
                                   ? requested
                                   : firstSupportedPixelFormat(impl.videoEncoder, impl.videoCodec);
      if(impl.videoCodec->pix_fmt == AV_PIX_FMT_YUV420P && ((width & 1) != 0 || (height & 1) != 0)) {
        SPDLOG_ERROR("FFmpeg yuv420p encoding requires even dimensions; got {}x{}", width, height);
        return false;
      }
      if(impl.videoCodec->pix_fmt == AV_PIX_FMT_YUV422P && (width & 1) != 0) {
        SPDLOG_ERROR("FFmpeg yuv422p encoding requires an even width; got {}", width);
        return false;
      }
      if(impl.format->oformat->flags & AVFMT_GLOBALHEADER)
        impl.videoCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
      if(impl.videoBitrate == 0 && impl.hardware)
        impl.videoCodec->bit_rate = static_cast<int64_t>(width) * height * 4;
      if(impl.videoCodec->priv_data) {
        av_opt_set(impl.videoCodec->priv_data, "preset", impl.preset.c_str(), AV_OPT_SEARCH_CHILDREN);
        const std::string crf = std::to_string(impl.crf);
        av_opt_set(impl.videoCodec->priv_data, "crf", crf.c_str(), AV_OPT_SEARCH_CHILDREN);
      }
      int result = avcodec_open2(impl.videoCodec, impl.videoEncoder, nullptr);
      if(result < 0 && impl.hardware && impl.options.value("hwAccel", std::string("none")) == "auto") {
        SPDLOG_WARN("Hardware encoder '{}' failed to open; falling back to software: {}", impl.videoCodecName, ffmpegError(result));
        avcodec_free_context(&impl.videoCodec);
        impl.videoEncoder = avcodec_find_encoder(defaultVideoCodec(impl.format->oformat, impl.path.c_str()));
        if(!impl.videoEncoder)
          impl.videoEncoder = avcodec_find_encoder(AV_CODEC_ID_H264);
        impl.hardware = false;
        if(!impl.videoEncoder)
          return false;
        impl.videoCodec = avcodec_alloc_context3(impl.videoEncoder);
        if(!impl.videoCodec)
          return false;
        impl.videoCodec->width = width;
        impl.videoCodec->height = height;
        impl.videoCodec->framerate = av_d2q(impl.fps, 100000);
        impl.videoCodec->time_base = impl.videoEncoder->id == AV_CODEC_ID_MPEG4
                                       ? av_inv_q(impl.videoCodec->framerate)
                                       : kVideoTimeBase;
        impl.videoCodec->gop_size = impl.gopSize > 0 ? impl.gopSize : static_cast<int>(std::round(impl.fps * 2.0));
        impl.videoCodec->bit_rate = impl.videoBitrate;
        impl.videoCodec->pix_fmt = isPixelFormatSupported(impl.videoEncoder, impl.videoCodec, requested)
                                     ? requested
                                     : firstSupportedPixelFormat(impl.videoEncoder, impl.videoCodec);
        if(impl.format->oformat->flags & AVFMT_GLOBALHEADER)
          impl.videoCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        result = avcodec_open2(impl.videoCodec, impl.videoEncoder, nullptr);
      }
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not open video encoder '{}': {}", impl.videoEncoder->name, ffmpegError(result));
        return false;
      }
      impl.videoCodecName = impl.videoEncoder->name;
      impl.videoStream->time_base = impl.videoCodec->time_base;
      result = avcodec_parameters_from_context(impl.videoStream->codecpar, impl.videoCodec);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not copy video stream parameters: {}", ffmpegError(result));
        return false;
      }
      impl.videoFrame = av_frame_alloc();
      if(!impl.videoFrame)
        return false;
      impl.videoFrame->format = impl.videoCodec->pix_fmt;
      impl.videoFrame->width = width;
      impl.videoFrame->height = height;
      result = av_frame_get_buffer(impl.videoFrame, 32);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not allocate the video frame: {}", ffmpegError(result));
        return false;
      }
      impl.videoInitialized = true;
      return writeHeaderIfReady(impl);
    }

    bool initializeAudioEncoder(FfmpegMediaWriter::Impl &impl, int channels)
    {
      if(impl.audioInitialized) {
        if(channels != impl.audioChannels) {
          SPDLOG_ERROR("Audio chunk has {} channels, but the encoder was set up for {}", channels, impl.audioChannels);
          return false;
        }
        return true;
      }
      if(impl.headerWritten) {
        SPDLOG_ERROR("Cannot add an audio stream after the FFmpeg container header was written");
        return false;
      }
      impl.audioChannels = channels;
      const auto audioOptions = impl.options.value("audio", nlohmann::json::object());
      if(audioOptions.is_object()) {
        impl.audioRate = audioOptions.value("sampleRate", impl.audioRate);
        impl.audioChannels = audioOptions.value("channels", channels);
        impl.audioBitrate = audioOptions.value("bitrate", impl.audioBitrate);
        impl.audioCodecName = audioOptions.value("codec", impl.audioCodecName);
      }
      if(impl.audioChannels <= 0 || impl.audioChannels > 64 || impl.audioRate <= 0) {
        SPDLOG_ERROR("Invalid FFmpeg audio settings: {} channels at {} Hz", impl.audioChannels, impl.audioRate);
        return false;
      }
      if(impl.audioChannels != channels) {
        SPDLOG_ERROR("Audio chunk has {} channels, but the writer is configured for {}", channels, impl.audioChannels);
        return false;
      }
      if(!impl.audioCodecName.empty())
        impl.audioEncoder = avcodec_find_encoder_by_name(impl.audioCodecName.c_str());
      if(!impl.audioEncoder && impl.format->oformat->audio_codec != AV_CODEC_ID_NONE)
        impl.audioEncoder = avcodec_find_encoder(impl.format->oformat->audio_codec);
      if(!impl.audioEncoder)
        impl.audioEncoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
      if(!impl.audioEncoder) {
        SPDLOG_ERROR("No FFmpeg audio encoder is available for '{}'", impl.path);
        return false;
      }
      // A previous attempt may have failed part way; don't leak what it allocated.
      avcodec_free_context(&impl.audioCodec);
      swr_free(&impl.audioResampler);
      if(impl.audioFifo) {
        av_audio_fifo_free(impl.audioFifo);
        impl.audioFifo = nullptr;
      }
      impl.audioCodec = avcodec_alloc_context3(impl.audioEncoder);
      if(!impl.audioCodec)
        return false;
      impl.audioCodec->sample_rate = impl.audioRate;
      impl.audioCodec->time_base = AVRational {1, impl.audioRate};
      impl.audioCodec->bit_rate = impl.audioBitrate;
      impl.audioCodec->sample_fmt = firstSupportedSampleFormat(impl.audioEncoder, impl.audioCodec);
      av_channel_layout_default(&impl.audioCodec->ch_layout, impl.audioChannels);
      if(impl.format->oformat->flags & AVFMT_GLOBALHEADER)
        impl.audioCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
      int result = avcodec_open2(impl.audioCodec, impl.audioEncoder, nullptr);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not open audio encoder '{}': {}", impl.audioEncoder->name, ffmpegError(result));
        return false;
      }
      // Only one audio stream, even if an earlier attempt got this far.
      if(!impl.audioStream)
        impl.audioStream = avformat_new_stream(impl.format, nullptr);
      if(!impl.audioStream) {
        SPDLOG_ERROR("FFmpeg could not create an audio stream");
        return false;
      }
      impl.audioStream->time_base = impl.audioCodec->time_base;
      result = avcodec_parameters_from_context(impl.audioStream->codecpar, impl.audioCodec);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not copy audio stream parameters: {}", ffmpegError(result));
        return false;
      }
      AVChannelLayout inputLayout {};
      av_channel_layout_default(&inputLayout, impl.audioChannels);
      const AVSampleFormat inputFormat = AV_SAMPLE_FMT_FLT;
      result = swr_alloc_set_opts2(&impl.audioResampler, &impl.audioCodec->ch_layout, impl.audioCodec->sample_fmt,
                                   impl.audioCodec->sample_rate, &inputLayout, inputFormat, impl.audioRate, 0, nullptr);
      av_channel_layout_uninit(&inputLayout);
      if(result < 0 || !impl.audioResampler) {
        SPDLOG_ERROR("FFmpeg could not create the audio resampler: {}", ffmpegError(result));
        return false;
      }
      result = swr_init(impl.audioResampler);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not initialize the audio resampler: {}", ffmpegError(result));
        return false;
      }
      impl.audioFifo = av_audio_fifo_alloc(impl.audioCodec->sample_fmt, impl.audioChannels,
                                           std::max(impl.audioCodec->frame_size, 1));
      if(!impl.audioFifo) {
        SPDLOG_ERROR("FFmpeg could not allocate an audio FIFO");
        return false;
      }
      impl.audioInitialized = true;
      impl.audioExpected = true;
      return writeHeaderIfReady(impl);
    }

    int64_t videoPts(FfmpegMediaWriter::Impl &impl, std::optional<MediaTime> timestamp)
    {
      int64_t pts = 0;
      if(timestamp) {
        if(!impl.hasMediaStart) {
          impl.mediaStartTime = *timestamp;
          impl.hasMediaStart = true;
        }
        const auto relative = *timestamp - impl.mediaStartTime;
        pts = av_rescale_q(relative.count(), kMicrosecondTimeBase, impl.videoCodec->time_base);
      } else {
        const AVRational frameTimeBase = av_inv_q(impl.videoCodec->framerate);
        pts = av_rescale_q(impl.videoFrameCount, frameTimeBase, impl.videoCodec->time_base);
      }
      if(impl.lastVideoPts != std::numeric_limits<int64_t>::min() && pts <= impl.lastVideoPts)
        pts = impl.lastVideoPts + 1;
      impl.lastVideoPts = pts;
      ++impl.videoFrameCount;
      return pts;
    }

    bool encodeAudioSamples(FfmpegMediaWriter::Impl &impl, const uint8_t **input, int inputSamples,
                            std::optional<MediaTime> timestamp)
    {
      if(!impl.audioInitialized) {
        SPDLOG_ERROR("Audio encoder is not initialized");
        return false;
      }
      if(timestamp && !impl.hasAudioStart) {
        if(!impl.hasMediaStart) {
          impl.mediaStartTime = *timestamp;
          impl.hasMediaStart = true;
        }
        const auto relative = *timestamp - impl.mediaStartTime;
        impl.audioPts = av_rescale_q(relative.count(), kMicrosecondTimeBase, impl.audioCodec->time_base);
        impl.hasAudioStart = true;
      }
      const int outputSamples = swr_get_out_samples(impl.audioResampler, inputSamples);
      AVFrame *converted = av_frame_alloc();
      if(!converted)
        return false;
      converted->format = impl.audioCodec->sample_fmt;
      converted->sample_rate = impl.audioCodec->sample_rate;
      converted->nb_samples = std::max(outputSamples, 1);
      av_channel_layout_copy(&converted->ch_layout, &impl.audioCodec->ch_layout);
      int result = av_frame_get_buffer(converted, 0);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not allocate converted audio samples: {}", ffmpegError(result));
        av_frame_free(&converted);
        return false;
      }
      result = swr_convert(impl.audioResampler, converted->extended_data, converted->nb_samples, input, inputSamples);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg audio resampling failed: {}", ffmpegError(result));
        av_frame_free(&converted);
        return false;
      }
      if(result > 0 && av_audio_fifo_realloc(impl.audioFifo, av_audio_fifo_size(impl.audioFifo) + result) >= 0) {
        const int written = av_audio_fifo_write(impl.audioFifo, reinterpret_cast<void **>(converted->extended_data), result);
        if(written != result) {
          SPDLOG_ERROR("FFmpeg wrote only {} of {} converted audio samples", written, result);
          av_frame_free(&converted);
          return false;
        }
      }
      av_frame_free(&converted);

      const int frameSize = impl.audioCodec->frame_size > 0 ? impl.audioCodec->frame_size : 1;
      while(av_audio_fifo_size(impl.audioFifo) >= frameSize) {
        AVFrame *frame = av_frame_alloc();
        if(!frame)
          return false;
        frame->format = impl.audioCodec->sample_fmt;
        frame->sample_rate = impl.audioCodec->sample_rate;
        frame->nb_samples = frameSize;
        av_channel_layout_copy(&frame->ch_layout, &impl.audioCodec->ch_layout);
        result = av_frame_get_buffer(frame, 0);
        if(result < 0) {
          av_frame_free(&frame);
          return false;
        }
        const int read = av_audio_fifo_read(impl.audioFifo, reinterpret_cast<void **>(frame->extended_data), frameSize);
        if(read != frameSize) {
          av_frame_free(&frame);
          return false;
        }
        frame->pts = impl.audioPts;
        impl.audioPts += frameSize;
        const bool ok = encodeFrame(impl, impl.audioCodec, impl.audioStream, frame);
        av_frame_free(&frame);
        if(!ok)
          return false;
      }
      return true;
    }
  }

  FfmpegMediaWriter::FfmpegMediaWriter(std::unique_ptr<Impl> impl)
      : mImpl(std::move(impl))
  {}

  FfmpegMediaWriter::~FfmpegMediaWriter()
  {
    static_cast<void>(close());
  }

  VideoResult<std::shared_ptr<FfmpegMediaWriter>> FfmpegMediaWriter::open(const std::string &path,
                                                                          const nlohmann::json &options)
  {
    auto impl = std::make_unique<Impl>();
    impl->path = path;
    impl->options = options.is_object() ? options : nlohmann::json::object();
    try {
      impl->fps = impl->options.value("fps", 30.0);
      impl->crf = impl->options.value("crf", 23);
      impl->videoBitrate = impl->options.value("bitrate", int64_t {0});
      impl->preset = impl->options.value("preset", std::string("medium"));
      impl->pixelFormatName = impl->options.value("pixelFormat", std::string("yuv420p"));
      impl->gopSize = impl->options.value("gop", 0);
      impl->audioExpected = isAudioEnabled(impl->options);
      if(!std::isfinite(impl->fps) || impl->fps <= 0.0 || impl->fps > 1000.0) {
        SPDLOG_ERROR("Invalid video frame rate {}", impl->fps);
        return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::InvalidArgument);
      }
      const std::string acceleration = impl->options.value("hwAccel", std::string("none"));
      if(acceleration != "none" && acceleration != "auto") {
        SPDLOG_ERROR("Invalid hwAccel mode '{}'; expected 'none' or 'auto'", acceleration);
        return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::InvalidArgument);
      }
    } catch(const std::exception &error) {
      SPDLOG_ERROR("Invalid FFmpeg writer options: {}", error.what());
      return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::InvalidArgument);
    }

    int result = avformat_alloc_output_context2(&impl->format, nullptr, nullptr, path.c_str());
    if(result < 0 || !impl->format) {
      SPDLOG_ERROR("FFmpeg could not select an output format for '{}': {}", path, ffmpegError(result));
      return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::UnsupportedFormat);
    }
    impl->videoEncoder = findVideoEncoder(impl->format->oformat, path.c_str(), impl->options,
                                          impl->videoCodecName, impl->hardware);
    if(!impl->videoEncoder) {
      SPDLOG_ERROR("No FFmpeg video encoder is available for '{}'", path);
      return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::ResourceUnavailable);
    }
    impl->videoStream = avformat_new_stream(impl->format, nullptr);
    if(!impl->videoStream) {
      SPDLOG_ERROR("FFmpeg could not create a video stream for '{}'", path);
      return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::ResourceAllocationError);
    }
    if(!(impl->format->oformat->flags & AVFMT_NOFILE)) {
      result = avio_open(&impl->format->pb, path.c_str(), AVIO_FLAG_WRITE);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not open '{}' for writing: {}", path, ffmpegError(result));
        return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::ResourceUnavailable);
      }
    }
    auto writer = std::shared_ptr<FfmpegMediaWriter>(new FfmpegMediaWriter(std::move(impl)));
    if(writer->mImpl->audioExpected) {
      const auto audioSettings = writer->mImpl->options.value("audio", nlohmann::json::object());
      const int channels = audioSettings.is_object() ? audioSettings.value("channels", 2) : 2;
      if(!writer->initializeAudio(channels))
        return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(VideoErrorCode::UnsupportedFormat);
    }
    return VideoResult<std::shared_ptr<FfmpegMediaWriter>>(std::move(writer));
  }

  bool FfmpegMediaWriter::initializeAudio(int channels)
  {
    return mImpl && initializeAudioEncoder(*mImpl, channels);
  }

  bool FfmpegMediaWriter::writeVideo(const Array<PixelRGB8, 2> &image, std::optional<MediaTime> timestamp)
  {
    int width = 0;
    int height = 0;
    auto pixels = packedPixels(image, width, height);
    return writePacked(pixels.data(), width, height, width * 3, AV_PIX_FMT_RGB24, timestamp);
  }

  bool FfmpegMediaWriter::writeVideo(const Array<PixelBGR8, 2> &image, std::optional<MediaTime> timestamp)
  {
    int width = 0;
    int height = 0;
    auto pixels = packedPixels(image, width, height);
    return writePacked(pixels.data(), width, height, width * 3, AV_PIX_FMT_BGR24, timestamp);
  }

  bool FfmpegMediaWriter::writePacked(const uint8_t *data, int width, int height, int lineSize, int pixelFormat,
                                      std::optional<MediaTime> timestamp)
  {
    if(!mImpl || mImpl->closed || !data)
      return false;
    if(!initializeVideo(*mImpl, width, height))
      return false;
    if(av_frame_make_writable(mImpl->videoFrame) < 0)
      return false;
    mImpl->videoScaler = sws_getCachedContext(mImpl->videoScaler, width, height, static_cast<AVPixelFormat>(pixelFormat),
                                              width, height, mImpl->videoCodec->pix_fmt, SWS_BICUBIC,
                                              nullptr, nullptr, nullptr);
    if(!mImpl->videoScaler) {
      SPDLOG_ERROR("FFmpeg could not create a pixel format converter");
      return false;
    }
    const uint8_t *sourceData[4] {data, nullptr, nullptr, nullptr};
    const int sourceStride[4] {lineSize, 0, 0, 0};
    const int rows = sws_scale(mImpl->videoScaler, sourceData, sourceStride, 0, height,
                               mImpl->videoFrame->data, mImpl->videoFrame->linesize);
    if(rows != height) {
      SPDLOG_ERROR("FFmpeg converted {} of {} video rows", rows, height);
      return false;
    }
    mImpl->videoFrame->pts = videoPts(*mImpl, timestamp);
    return encodeFrame(*mImpl, mImpl->videoCodec, mImpl->videoStream, mImpl->videoFrame);
  }

  bool FfmpegMediaWriter::writePlanar420(const YUV420Image<uint8_t> &image, std::optional<MediaTime> timestamp)
  {
    if(!mImpl || mImpl->closed)
      return false;
    const auto fullRange = image.range();
    const auto dimensions = fullRange.size();
    const int height = dimensions[0];
    const int width = dimensions[1];
    if((width & 1) != 0 || (height & 1) != 0) {
      SPDLOG_ERROR("YUV420 video dimensions must be even; got {}x{}", width, height);
      return false;
    }
    auto y = planePixels(image.plane<0>().data());
    auto u = planePixels(image.plane<1>().data());
    auto v = planePixels(image.plane<2>().data());
    if(!initializeVideo(*mImpl, width, height) || av_frame_make_writable(mImpl->videoFrame) < 0)
      return false;
    mImpl->videoScaler = sws_getCachedContext(mImpl->videoScaler, width, height, AV_PIX_FMT_YUV420P,
                                              width, height, mImpl->videoCodec->pix_fmt, SWS_BICUBIC,
                                              nullptr, nullptr, nullptr);
    if(!mImpl->videoScaler)
      return false;
    const uint8_t *sourceData[4] {y.data(), u.data(), v.data(), nullptr};
    const int sourceStride[4] {width, width / 2, width / 2, 0};
    const int rows = sws_scale(mImpl->videoScaler, sourceData, sourceStride, 0, height,
                               mImpl->videoFrame->data, mImpl->videoFrame->linesize);
    if(rows != height)
      return false;
    mImpl->videoFrame->pts = videoPts(*mImpl, timestamp);
    return encodeFrame(*mImpl, mImpl->videoCodec, mImpl->videoStream, mImpl->videoFrame);
  }

  bool FfmpegMediaWriter::writeVideo(const YUV420Image<uint8_t> &image, std::optional<MediaTime> timestamp)
  {
    return writePlanar420(image, timestamp);
  }

  bool FfmpegMediaWriter::writeAudioFloat(const Array<float, 2> &audio, std::optional<MediaTime> timestamp)
  {
    const auto range = audio.range();
    const auto dimensions = range.size();
    const int samples = dimensions[0];
    const int channels = dimensions[1];
    if(samples <= 0 || channels <= 0)
      return false;
    // Checks the channel count against the encoder on every chunk, not just the first.
    if(!initializeAudio(channels))
      return false;
    std::vector<float> packed(static_cast<std::size_t>(samples) * static_cast<std::size_t>(channels));
    std::size_t offset = 0;
    for(int sample = range.min(0); sample <= range.max(0); ++sample) {
      for(int channel = range.min(1); channel <= range.max(1); ++channel)
        packed[offset++] = audio(sample, channel);
    }
    const uint8_t *input[1] {reinterpret_cast<const uint8_t *>(packed.data())};
    return encodeAudioSamples(*mImpl, input, samples, timestamp);
  }

  bool FfmpegMediaWriter::writeAudioInt16(const Array<int16_t, 2> &audio, std::optional<MediaTime> timestamp)
  {
    const auto range = audio.range();
    const auto dimensions = range.size();
    const int samples = dimensions[0];
    const int channels = dimensions[1];
    if(samples <= 0 || channels <= 0)
      return false;
    // Checks the channel count against the encoder on every chunk, not just the first.
    if(!initializeAudio(channels))
      return false;
    std::vector<float> packed(static_cast<std::size_t>(samples) * static_cast<std::size_t>(channels));
    std::size_t offset = 0;
    for(int sample = range.min(0); sample <= range.max(0); ++sample) {
      for(int channel = range.min(1); channel <= range.max(1); ++channel)
        packed[offset++] = static_cast<float>(audio(sample, channel)) / 32768.0f;
    }
    const uint8_t *input[1] {reinterpret_cast<const uint8_t *>(packed.data())};
    return encodeAudioSamples(*mImpl, input, samples, timestamp);
  }

  bool FfmpegMediaWriter::writeFrame(const Frame &frame)
  {
    if(!frame.isValid())
      return false;
    const auto timestamp = std::optional<MediaTime>(frame.timestamp());
    const std::any data = frame.frameData();
    if(frame.dataType() == typeid(Array<PixelRGB8, 2>))
      return writeVideo(std::any_cast<const Array<PixelRGB8, 2> &>(data), timestamp);
    if(frame.dataType() == typeid(Array<PixelBGR8, 2>))
      return writeVideo(std::any_cast<const Array<PixelBGR8, 2> &>(data), timestamp);
    if(frame.dataType() == typeid(YUV420Image<uint8_t>))
      return writeVideo(std::any_cast<const YUV420Image<uint8_t> &>(data), timestamp);
    if(frame.dataType() == typeid(AudioChunk<float>))
      return writeAudio(std::any_cast<const AudioChunk<float> &>(data), timestamp);
    if(frame.dataType() == typeid(AudioChunk<int16_t>))
      return writeAudio(std::any_cast<const AudioChunk<int16_t> &>(data), timestamp);
    if(auto converted = frame.castOpt<Array<PixelRGB8, 2>>())
      return writeVideo(*converted, timestamp);
    SPDLOG_ERROR("FFmpeg writer does not support frame data type {}", typeName(frame.dataType()));
    return false;
  }

  bool FfmpegMediaWriter::close() noexcept
  {
    if(!mImpl || mImpl->closed)
      return true;
    bool success = true;
    if(mImpl->videoInitialized) {
      if(!encodeFrame(*mImpl, mImpl->videoCodec, mImpl->videoStream, nullptr))
        success = false;
    }
    if(mImpl->audioInitialized) {
      for(;;) {
        const int outSamples = swr_get_out_samples(mImpl->audioResampler, 0);
        if(outSamples <= 0)
          break;
        AVFrame *converted = av_frame_alloc();
        if(!converted) {
          success = false;
          break;
        }
        converted->format = mImpl->audioCodec->sample_fmt;
        converted->sample_rate = mImpl->audioCodec->sample_rate;
        converted->nb_samples = outSamples;
        av_channel_layout_copy(&converted->ch_layout, &mImpl->audioCodec->ch_layout);
        int result = av_frame_get_buffer(converted, 0);
        if(result >= 0)
          result = swr_convert(mImpl->audioResampler, converted->extended_data, outSamples, nullptr, 0);
        if(result < 0) {
          SPDLOG_ERROR("FFmpeg could not flush the audio resampler: {}", ffmpegError(result));
          success = false;
          av_frame_free(&converted);
          break;
        }
        if(result == 0) {
          av_frame_free(&converted);
          break;
        }
        if(av_audio_fifo_realloc(mImpl->audioFifo, av_audio_fifo_size(mImpl->audioFifo) + result) < 0
           || av_audio_fifo_write(mImpl->audioFifo, reinterpret_cast<void **>(converted->extended_data), result) != result)
          success = false;
        av_frame_free(&converted);
        if(!success)
          break;
      }
      const int frameSize = mImpl->audioCodec->frame_size > 0 ? mImpl->audioCodec->frame_size : av_audio_fifo_size(mImpl->audioFifo);
      const int remaining = av_audio_fifo_size(mImpl->audioFifo);
      if(remaining > 0) {
        AVFrame *frame = av_frame_alloc();
        if(!frame) {
          success = false;
        } else {
          frame->format = mImpl->audioCodec->sample_fmt;
          frame->sample_rate = mImpl->audioCodec->sample_rate;
          frame->nb_samples = std::max(frameSize, remaining);
          av_channel_layout_copy(&frame->ch_layout, &mImpl->audioCodec->ch_layout);
          int result = av_frame_get_buffer(frame, 0);
          if(result >= 0) {
            av_samples_set_silence(frame->extended_data, 0, frame->nb_samples, mImpl->audioChannels,
                                   mImpl->audioCodec->sample_fmt);
            const int read = av_audio_fifo_read(mImpl->audioFifo, reinterpret_cast<void **>(frame->extended_data), remaining);
            frame->pts = mImpl->audioPts;
            mImpl->audioPts += frame->nb_samples;
            if(read != remaining || !encodeFrame(*mImpl, mImpl->audioCodec, mImpl->audioStream, frame))
              success = false;
          } else {
            success = false;
          }
          av_frame_free(&frame);
        }
      }
      if(!encodeFrame(*mImpl, mImpl->audioCodec, mImpl->audioStream, nullptr))
        success = false;
    }
    if(mImpl->headerWritten) {
      const int result = av_write_trailer(mImpl->format);
      if(result < 0) {
        SPDLOG_ERROR("FFmpeg could not write the trailer for '{}': {}", mImpl->path, ffmpegError(result));
        success = false;
      }
    } else if(mImpl->videoInitialized) {
      if(!writeHeaderIfReady(*mImpl))
        success = false;
      if(mImpl->headerWritten && av_write_trailer(mImpl->format) < 0)
        success = false;
    } else {
      SPDLOG_WARN("FFmpeg writer for '{}' was closed before a video frame was written", mImpl->path);
      success = false;
    }
    mImpl->closed = true;
    return success;
  }

} // namespace Ravl2::Video