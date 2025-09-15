//
// Created by charles galambos on 12/09/2025.
//

#include <spdlog/spdlog.h>
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/StreamInput.hh"            // For StreamInputCall
#include "Ravl2/Video/FfmpegMediaContainer.hh" // FFmpeg media container implementation
#include "Ravl2/Video/StreamIterator.hh"       // Generic stream iterator API
#include "Ravl2/Video/VideoFrame.hh"           // VideoFrame template
#include "Ravl2/Video/VideoTypes.hh"           // VideoErrorCode / MediaTime
#include "Ravl2/Video/VideoIO.hh"              // initIO()
#include "Ravl2/Pixel/PixelPlane.hh"           // Planar image types (YUV/RGB)
#include "Ravl2/Pixel/Colour.hh"               // Colour conversions & registration

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
      if (iterator->dataType() != typeid(ImageT))
        return std::nullopt; // Not this underlying type.

      // Find conversion chain only if caller requested a different target type.
      std::optional<ConversionChain> convChain;
      if (ctx.m_targetType != typeid(ImageT))
      {
        convChain = typeConverterMap().find(ctx.m_targetType, typeid(ImageT));
        if (!convChain)
        {
          if (ctx.m_verbose)
            SPDLOG_INFO("FFmpeg: No conversion from {} to requested {}", typeName(typeid(ImageT)), typeName(ctx.m_targetType));
          return std::nullopt; // Can't satisfy requested type with this base.
        }
      }

      // StreamInputCall simply yields successive frames as ImageT objects.
      auto stream = std::make_shared<StreamInputCall<ImageT>>(
        [iterator](std::streampos &pos) mutable -> std::optional<ImageT>
        {
          (void)pos; // Seeking not supported in this simple streaming adapter.
          if (!iterator->currentFrame())
            return std::nullopt;

            // Extract image (copy lightweight wrapper / shared planes).
          auto vf = std::dynamic_pointer_cast<VideoFrame<ImageT>>(iterator->currentFrame());
          if (!vf)
            return std::nullopt;
          ImageT img = vf->image();

          // Advance for next request. Failure other than EndOfStream is logged and stream ends.
          auto nextRes = iterator->next();
          if (!nextRes.isSuccess() && nextRes.error() != VideoErrorCode::EndOfStream)
            SPDLOG_DEBUG("FFmpeg: iterator->next() error {}", static_cast<int>(nextRes.error()));
          return img;
        }
      );

      if (ctx.m_verbose)
      {
        SPDLOG_INFO("FFmpeg: Plan created underlying {} -> target {} (loss {})",
                    typeName(typeid(ImageT)), typeName(ctx.m_targetType),
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
      if (auto p = makePlanForType<YUV420Image<uint8_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<YUV422Image<uint8_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<YUV444Image<uint8_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<RGBPlanarImage<uint8_t>>(ctx, container, iterator)) return p;
      if (auto p = makePlanForType<RGBAPlanarImage<uint8_t>>(ctx, container, iterator)) return p;

      // Fallback: not a type we currently adapt explicitly.
      SPDLOG_DEBUG("FFmpeg: Unhandled base frame type '{}'", typeName(iterator->dataType()));
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
        if (ctx.m_verbose) {
          SPDLOG_INFO("FFmpeg: probing '{}' target {}", ctx.m_filename, typeName(ctx.m_targetType));
        }

        // Attempt open.
        auto openRes = FfmpegMediaContainer::openFile(ctx.m_filename);
        if (!openRes.isSuccess())
        {
          if (ctx.m_verbose)
            SPDLOG_DEBUG("FFmpeg: openFile failed code {}", static_cast<int>(openRes.error()));
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
          if (ctx.m_verbose)
            SPDLOG_INFO("FFmpeg: no video stream in '{}'", ctx.m_filename);
          return std::nullopt; // Not our case.
        }

        auto iterRes = container->createIterator(vIndex);
        if (!iterRes.isSuccess())
        {
          if (ctx.m_verbose)
            SPDLOG_INFO("FFmpeg: createIterator failed code {}", static_cast<int>(iterRes.error()));
          return std::nullopt;
        }
        auto iterator = iterRes.value();

        // Build plan around iterator & initial frame.
        auto plan = buildFfmpegVideoPlan(ctx, container, iterator);
        if (!plan)
        {
          if (ctx.m_verbose)
            SPDLOG_INFO("FFmpeg: unable to build plan for '{}' (frame type {})", ctx.m_filename, typeName(iterator->dataType()));
          return std::nullopt;
        }
        if (ctx.m_verbose)
          SPDLOG_INFO("FFmpeg: plan ready for '{}'", ctx.m_filename);
        return plan;
      }
    ));
  } // namespace
}