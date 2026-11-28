//
// Owning smart pointers for FFmpeg objects, so they're released on every path.
//

#pragma once

#include <memory>

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace Ravl2::Video
{
  struct AVFrameDeleter {
    void operator()(AVFrame *frame) const noexcept { av_frame_free(&frame); }
  };
  struct AVPacketDeleter {
    void operator()(AVPacket *packet) const noexcept { av_packet_free(&packet); }
  };
  struct AVCodecContextDeleter {
    void operator()(AVCodecContext *context) const noexcept { avcodec_free_context(&context); }
  };
  struct SwsContextDeleter {
    void operator()(SwsContext *context) const noexcept { sws_freeContext(context); }
  };
  struct SwrContextDeleter {
    void operator()(SwrContext *context) const noexcept { swr_free(&context); }
  };
  struct AVAudioFifoDeleter {
    void operator()(AVAudioFifo *fifo) const noexcept { av_audio_fifo_free(fifo); }
  };

  using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
  using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
  using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
  using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
  using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>;
  using AVAudioFifoPtr = std::unique_ptr<AVAudioFifo, AVAudioFifoDeleter>;

  //! Make a same-size pixel format converter with explicit colour ranges (full = 0-255, limited = 16-235) and
  //! YUV matrices (sws_getCoefficients tables). The ranges are set before sws_init_context: for a same-format
  //! conversion swscale picks a plain copy that ignores ranges set afterwards with sws_setColorspaceDetails.
  //! @return null if FFmpeg has no such conversion.
  inline SwsContextPtr makeSwsConverter(int width, int height, AVPixelFormat source, AVPixelFormat target,
                                        bool sourceFullRange, bool targetFullRange,
                                        const int *sourceCoefficients, const int *targetCoefficients)
  {
    SwsContextPtr context(sws_alloc_context());
    SwsContext *ctx = context.get();
    if(ctx == nullptr
       || av_opt_set_int(ctx, "srcw", width, 0) < 0 || av_opt_set_int(ctx, "srch", height, 0) < 0
       || av_opt_set_int(ctx, "src_format", source, 0) < 0
       || av_opt_set_int(ctx, "dstw", width, 0) < 0 || av_opt_set_int(ctx, "dsth", height, 0) < 0
       || av_opt_set_int(ctx, "dst_format", target, 0) < 0
       || av_opt_set_int(ctx, "sws_flags", SWS_BICUBIC, 0) < 0
       || av_opt_set_int(ctx, "src_range", sourceFullRange ? 1 : 0, 0) < 0
       || av_opt_set_int(ctx, "dst_range", targetFullRange ? 1 : 0, 0) < 0
       || sws_init_context(ctx, nullptr, nullptr) < 0)
      return nullptr;
    // Matrices only; this fails harmlessly (keeping the ranges above) for conversions without YUV<->RGB.
    (void)sws_setColorspaceDetails(ctx, sourceCoefficients, sourceFullRange ? 1 : 0, targetCoefficients,
                                   targetFullRange ? 1 : 0, 0, 1 << 16, 1 << 16);
    return context;
  }
}// namespace Ravl2::Video
