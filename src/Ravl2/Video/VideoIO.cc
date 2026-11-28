//
// Created by charles galambos on 12/09/2025.
//

#include <spdlog/spdlog.h>
#include <limits>
#include <stdexcept>
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/StreamInput.hh"            // For StreamInputCall
#include "Ravl2/Video/FfmpegMediaContainer.hh" // FFmpeg media container implementation
#include "Ravl2/Video/StreamIterator.hh"       // Generic stream iterator API
#include "Ravl2/Video/VideoTypes.hh"           // VideoErrorCode / MediaTime
#include "Ravl2/Video/VideoIO.hh"              // initIO()
#include "Ravl2/Pixel/PixelPlane.hh"           // Planar image types (YUV/RGB)
#include "Ravl2/Pixel/Colour.hh"               // Colour conversions & registration
#if defined(HAVE_FFMPEG)
#include "Ravl2/Video/FfmpegMediaWriter.hh"
#endif

namespace Ravl2::Video
{
  void initIO()
  {
    Ravl2::initColourConversion();
    Ravl2::initPlaneConversion();
  }

  namespace
  {
    // Helper: Build a StreamInputPlan for a concrete underlying frame image type.
    // Mirroring the OpenCV loader pattern we attempt the exact underlying type first,
    // then optionally attach a conversion chain to the requested target.
    template <typename ImageT>
    std::optional<StreamInputPlan> makePlanForType(const ProbeInputContext &ctx,
                                                   const std::shared_ptr<MediaContainer> &/*container*/, // reserved for future use
                                                   const std::shared_ptr<StreamIterator> &iterator)
    {
      if (iterator->dataType() != typeid(ImageT)) {
        return std::nullopt; // Not this underlying type.
      }
      SPDLOG_INFO("Got type match {} ",Ravl2::typeName(typeid(ImageT)));

      // Find conversion chain only if the caller requested a different target type.
      std::optional<ConversionChain> convChain;
      if (ctx.mTargetType != typeid(ImageT))
      {
        convChain = typeConverterMap().find(ctx.mTargetType, typeid(ImageT));
        if (!convChain)
        {
          if (ctx.mVerbose) {
            SPDLOG_INFO("FFmpeg: No conversion from {} to requested {}", typeName(typeid(ImageT)), typeName(ctx.mTargetType));
          }
          return std::nullopt; // Can't satisfy requested type with this base.
        }
      }

      // StreamInputCall simply yields successive frames as ImageT objects.
      auto stream = std::make_shared<StreamInputCall<ImageT>>(
        [iterator, ended = false](std::streampos &pos) mutable -> std::optional<ImageT>
        {
          (void)pos; // Seeking not supported in this simple streaming adapter.
          // At end of stream the iterator keeps its last frame, so track the end here.
          if (ended || !iterator->currentFrame()) {
            return std::nullopt;
          }

            // Extract image (copy lightweight wrapper / shared planes).
          auto vf = std::dynamic_pointer_cast<FrameData<ImageT>>(iterator->currentFrame());
          if (!vf) {
            return std::nullopt;
          }
          ImageT img = vf->data();

          // Advance for next request. Failure other than EndOfStream is logged and stream ends.
          auto nextRes = iterator->next();
          if (!nextRes.isSuccess()) {
            ended = true;
            if (nextRes.error() != VideoErrorCode::EndOfStream) {
              SPDLOG_WARN("FFmpeg: iterator->next() error {}, ending stream", static_cast<int>(nextRes.error()));
            }
          }
          return img;
        }
      );

      if (ctx.mVerbose)
      {
        SPDLOG_INFO("FFmpeg: Plan created underlying {} -> target {} (loss {})",
                    typeName(typeid(ImageT)), typeName(ctx.mTargetType),
                    convChain ? convChain->conversionLoss() : 1.0f);
      }

      if (convChain)
        return StreamInputPlan{stream, *convChain, convChain->conversionLoss()};
      return StreamInputPlan{stream, {}, 1.0f};
    }

    // Try a list of known planar / packed image representations.
    std::optional<StreamInputPlan> buildFfmpegVideoPlan(const ProbeInputContext &ctx,
                                                        const std::shared_ptr<MediaContainer> &container,
                                                        const std::shared_ptr<StreamIterator> &iterator)
    {
      if(ctx.mVerbose) {
        SPDLOG_INFO("FFmpeg: Building video plan for iterator type {} ",Ravl2::typeName(iterator->dataType()));
      }
      // Every type the FFmpeg iterator can deliver (see planVideo() in FfmpegMultiStreamIterator.cc).
      if (auto p = makePlanForType<YUV420Image<uint8_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<YUV420Image<uint16_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<YUV422Image<uint8_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<YUV444Image<uint8_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<Array<PixelRGB8,2>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<Array<PixelRGBA8,2>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<Array<PixelYUYV8,2>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<Array<PixelUYVY8,2>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<Array<PixelI8,2>>(ctx, container, iterator)) return p;

      // Fallback: not a type we currently adapt explicitly.
      if(ctx.mVerbose) {
        SPDLOG_WARN("FFmpeg: Unhandled base frame type '{}'", typeName(iterator->dataType()));
      }
      return std::nullopt;
    }

    // Register FFmpeg-based video input loader (similar style to OpenCV/ImageIO.cc video handlers).
    // Priority kept modest (-2) so that specialised handlers can override if needed.
    [[maybe_unused]] bool g_regFfmpegVideo = inputFormatMap().add(std::make_shared<InputFormatCall>(
      "FFmpeg",
      "mp4,mov,m4v,avi,mkv,webm,flv,ts,mpg,mpeg",
      "file",
      -2,
      [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan>
      {
        if (ctx.mVerbose) {
          SPDLOG_INFO("FFmpeg: probing '{}' target {}", ctx.mFilename, typeName(ctx.mTargetType));
        }

        // Attempt open.
        auto openRes = FfmpegMediaContainer::openFile(ctx.mFilename);
        if (!openRes.isSuccess())
        {
          if (ctx.mVerbose) {
            SPDLOG_DEBUG("FFmpeg: openFile failed code {}", static_cast<int>(openRes.error()));
          }
          return std::nullopt; // Allow other formats to try.
        }
        auto container = std::static_pointer_cast<MediaContainer>(openRes.value());

        // Find the first video stream.
        std::size_t vIndex = container->streamCount();
        for (std::size_t i = 0; i < container->streamCount(); ++i)
        {
          if (container->streamType(i) == StreamType::Video) { vIndex = i; break; }
        }
        if (vIndex == container->streamCount())
        {
          if (ctx.mVerbose) {
            SPDLOG_INFO("FFmpeg: no video stream in '{}'", ctx.mFilename);
          }
          return std::nullopt; // Not our case.
        }

        auto iterRes = container->createIterator(vIndex);
        if (!iterRes.isSuccess())
        {
          if (ctx.mVerbose) {
            SPDLOG_INFO("FFmpeg: createIterator failed code {}", static_cast<int>(iterRes.error()));
          }
          return std::nullopt;
        }
        auto iterator = iterRes.value();

        // Build plan around iterator & initial frame.
        auto plan = buildFfmpegVideoPlan(ctx, container, iterator);
        if (!plan)
        {
          if (ctx.mVerbose)
            SPDLOG_INFO("FFmpeg: unable to build plan for '{}' (frame type {})", ctx.mFilename, typeName(iterator->dataType()));
          return std::nullopt;
        }
        if (ctx.mVerbose)
          SPDLOG_INFO("FFmpeg: plan ready for '{}'", ctx.mFilename);
        return plan;
      }
    ));


        // Register FFmpeg-based video input loader (similar style to OpenCV/ImageIO.cc video handlers).
    // Priority kept modest (-2) so that specialised handlers can override if needed.
    [[maybe_unused]] bool g_regFfmpegCapture = inputFormatMap().add(std::make_shared<InputFormatCall>(
      "FFmpegCapture",
      "",
      "camera",
      -2,
      [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan>
      {
        if (ctx.mVerbose) {
          SPDLOG_INFO("FFmpeg: probing '{}' target {}", ctx.mFilename, typeName(ctx.mTargetType));
        }

        Ravl2::Video::DeviceParameters params;
        params.devicePath = ctx.mFilename;

        SPDLOG_INFO("Opening : {}   Verbose={} ", ctx.mFilename,ctx.mVerbose);
        // Attempt open.
        auto openRes = FfmpegMediaContainer::openDevice(params);
        if (!openRes.isSuccess())
        {
          if (ctx.mVerbose)
            SPDLOG_DEBUG("FFmpeg: openFile failed code {}", static_cast<int>(openRes.error()));
          return std::nullopt; // Allow other formats to try.
        }
        auto container = std::static_pointer_cast<MediaContainer>(openRes.value());

        // Find the first video stream.
        std::size_t vIndex = container->streamCount();
        for (std::size_t i = 0; i < container->streamCount(); ++i)
        {
          if (container->streamType(i) == StreamType::Video)
            { vIndex = i; break; }
        }
        if (vIndex == container->streamCount())
        {
          if (ctx.mVerbose) {
            SPDLOG_INFO("FFmpeg: no video stream in '{}'", ctx.mFilename);
          }
          return std::nullopt; // Not our case.
        }

        auto iterRes = container->createIterator(vIndex);
        if (!iterRes.isSuccess())
        {
          if (ctx.mVerbose) {
            SPDLOG_INFO("FFmpeg: createIterator failed code {}", static_cast<int>(iterRes.error()));
          }
          return std::nullopt;
        }
        auto iterator = iterRes.value();

        // Build plan around iterator & initial frame.
        auto plan = buildFfmpegVideoPlan(ctx, container, iterator);
        if (!plan)
        {
          if (ctx.mVerbose) {
            SPDLOG_INFO("FFmpeg: unable to build plan for '{}' (frame type {})", ctx.mFilename, typeName(iterator->dataType()));
          }
          return std::nullopt;
        }
        if (ctx.mVerbose)
          SPDLOG_INFO("FFmpeg: plan ready for '{}'", ctx.mFilename);
        return plan;
      }
    ));

#if defined(HAVE_FFMPEG)
    template <typename ObjectT, typename WriteFn>
    std::shared_ptr<StreamOutput<ObjectT>> makeFfmpegOutput(const ProbeOutputContext &ctx, WriteFn write)
    {
      auto writerResult = FfmpegMediaWriter::open(ctx.mFilename, ctx.mFormatHint);
      if(!writerResult.isSuccess())
        return {};
      auto writer = writerResult.value();
      return std::make_shared<StreamOutputCall<ObjectT>>(
        [writer, write = std::move(write)](const ObjectT &object, std::streampos pos) -> std::streampos {
          if(!write(*writer, object))
            throw std::runtime_error("FFmpeg media writer failed while encoding an object");
          return pos;
        },
        0, streamPosEnd(), [writer]() { return writer->close(); });
    }

    [[maybe_unused]] bool g_regFfmpegOutput = outputFormatMap().add(std::make_shared<OutputFormatCall>(
      "FFmpeg", "mp4,mov,m4v,mkv,avi,webm", "file", -2,
      [](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan> {
        if(ctx.mSourceType == typeid(std::shared_ptr<Frame>)) {
          auto stream = makeFfmpegOutput<std::shared_ptr<Frame>>(ctx,
            [](FfmpegMediaWriter &writer, const std::shared_ptr<Frame> &frame) {
              return frame && writer.writeFrame(*frame);
            });
          if(stream)
            return StreamOutputPlan {stream, {}, 1.0f};
          return std::nullopt;
        }
        if(ctx.mSourceType == typeid(YUV420Image<uint8_t>)) {
          auto stream = makeFfmpegOutput<YUV420Image<uint8_t>>(ctx,
            [](FfmpegMediaWriter &writer, const YUV420Image<uint8_t> &image) { return writer.writeVideo(image); });
          if(stream)
            return StreamOutputPlan {stream, {}, 1.0f};
          return std::nullopt;
        }
        if(ctx.mSourceType == typeid(Array<PixelRGB8, 2>)) {
          auto stream = makeFfmpegOutput<Array<PixelRGB8, 2>>(ctx,
            [](FfmpegMediaWriter &writer, const Array<PixelRGB8, 2> &image) { return writer.writeVideo(image); });
          if(stream)
            return StreamOutputPlan {stream, {}, 1.0f};
          return std::nullopt;
        }
        if(ctx.mSourceType == typeid(Array<PixelBGR8, 2>)) {
          auto stream = makeFfmpegOutput<Array<PixelBGR8, 2>>(ctx,
            [](FfmpegMediaWriter &writer, const Array<PixelBGR8, 2> &image) { return writer.writeVideo(image); });
          if(stream)
            return StreamOutputPlan {stream, {}, 1.0f};
          return std::nullopt;
        }

        auto conversion = typeConverterMap().find(typeid(Array<PixelRGB8, 2>), ctx.mSourceType);
        if(!conversion)
          return std::nullopt;
        auto stream = makeFfmpegOutput<Array<PixelRGB8, 2>>(ctx,
          [](FfmpegMediaWriter &writer, const Array<PixelRGB8, 2> &image) { return writer.writeVideo(image); });
        if(!stream)
          return std::nullopt;
        return StreamOutputPlan {stream, *conversion, conversion->conversionLoss()};
      }
    ));
#endif

  } // namespace
}