//
// Owning smart pointers for FFmpeg objects, so they're released on every path.
//

#pragma once

#include <memory>

extern "C" {
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
}// namespace Ravl2::Video
