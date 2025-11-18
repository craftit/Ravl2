//
// JPEG Turbo Image IO registration (stub)
// This file currently registers a high-priority JPEG input format placeholder.
// Subsequent commits will add libjpeg-based probing and decode paths per JpegLoaderPlan.md.
//

#include "Ravl2/ImageIO/JpegTurboImageIO.hh"

#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/Logging.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"

#include <cstdio>
#include <memory>
#include <optional>
#include <string>

#ifdef RAVL2_HAVE_JPEG
#  include <jpeglib.h>
#endif

namespace Ravl2
{
  namespace {

#ifdef RAVL2_HAVE_JPEG
    struct FileCloser {
      void operator()(FILE* f) const noexcept { if (f) std::fclose(f); }
    };

    using FilePtr = std::unique_ptr<FILE, FileCloser>;

    //! RAII context that keeps the file open and a prepared jpeg_decompress_struct
    //! Header is read in the constructor so we don't need to reopen/parse again later.
    struct JpegDecodeContext {
      FilePtr file;                // Open file handle (stdin source for libjpeg)
      jpeg_decompress_struct cinfo{}; // Decompress struct
      jpeg_error_mgr jerr{};          // Error manager
      bool created{false};
      bool headerOk{false};
      bool consumed{false};          // Ensure single-use

      explicit JpegDecodeContext(const std::string &filename)
      {
        file.reset(std::fopen(filename.c_str(), "rb"));
        if (!file) {
          return;
        }
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        created = true;
        jpeg_stdio_src(&cinfo, file.get());
        headerOk = (jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK);
      }

      ~JpegDecodeContext()
      {
        if (created) {
          jpeg_destroy_decompress(&cinfo);
        }
      }
      // Non-copyable
      JpegDecodeContext(const JpegDecodeContext&) = delete;
      JpegDecodeContext& operator=(const JpegDecodeContext&) = delete;
    };

    template <typename ViaT>
    std::optional<StreamInputPlan> buildPlanFor(const ProbeInputContext &ctx, std::shared_ptr<JpegDecodeContext> sharedCtx)
    {
      // Find conversion chain from ViaT -> target
      auto chainOpt = typeConverterMap().find(ctx.m_targetType, typeid(ViaT));
      if (!chainOpt.has_value()) {
        if (ctx.m_verbose) {
          SPDLOG_INFO("JPEGTurbo: no conversion path from {} to {}", typeName(typeid(ViaT)), typeName(ctx.m_targetType));
        }
        return std::nullopt;
      }

      // Create a decoder stream that emits ViaT once
      auto strm = std::make_shared<StreamInputCall<ViaT>>([sharedCtx, verbose = ctx.m_verbose](std::streampos &pos) -> std::optional<ViaT>
      {
        if (pos != 0) {
          return std::nullopt;
        }
        if (!sharedCtx || !sharedCtx->created || !sharedCtx->headerOk || sharedCtx->consumed) {
          if (verbose) {
            SPDLOG_INFO("JPEGTurbo: decode context invalid or already consumed");
          }
          return std::nullopt;
        }
        auto &cinfo = sharedCtx->cinfo;

        // Configure output colorspace based on ViaT
        if constexpr (std::is_same_v<ViaT, Array<uint8_t, 2>>) {
          cinfo.out_color_space = JCS_GRAYSCALE;
        } else if constexpr (std::is_same_v<ViaT, Array<PixelRGB8, 2>>) {
#ifdef JCS_EXTENSIONS
          cinfo.out_color_space = JCS_EXT_RGB;
#else
          cinfo.out_color_space = JCS_RGB;
#endif
        } else {
          // Unsupported ViaT in this minimal implementation
          return std::nullopt;
        }

        if (!jpeg_start_decompress(&cinfo)) {
          SPDLOG_WARN("JPEGTurbo: failed to start decompression");
          return std::nullopt;
        }

        const JDIMENSION width = cinfo.output_width;
        const JDIMENSION height = cinfo.output_height;

        if constexpr (std::is_same_v<ViaT, Array<uint8_t, 2>>) {
          ViaT out({int(height), int(width)});
          for (JDIMENSION y = 0; y < height; ++y) {
            // Directly decode into destination row buffer
            JSAMPROW row = reinterpret_cast<JSAMPROW>(&out[{int(y), 0}]);
            JSAMPARRAY rows = &row;
            jpeg_read_scanlines(&cinfo, rows, 1);
          }
          jpeg_finish_decompress(&cinfo);
          sharedCtx->consumed = true;
          pos = 1;
          return out;
        } else if constexpr (std::is_same_v<ViaT, Array<PixelRGB8, 2>>) {
          ViaT out({static_cast<int>(height), static_cast<int>(width)});
          static_assert(sizeof(PixelRGB8) == 3, "PixelRGB8 must be 3 bytes");
          for (JDIMENSION y = 0; y < height; ++y) {
            JSAMPROW row = reinterpret_cast<JSAMPROW>(&out[{int(y), 0}]);
            JSAMPARRAY rows = &row;
            jpeg_read_scanlines(&cinfo, rows, 1);
          }
          jpeg_finish_decompress(&cinfo);
          sharedCtx->consumed = true;
          pos = 1;
          return out;
        }

        // Not reached
        jpeg_finish_decompress(&cinfo);
        sharedCtx->consumed = true;
        return std::nullopt;
      });

      const float intrinsicLoss = 1.0f; // no implicit conversion loss for basic gray/RGB paths
      auto chain = chainOpt.value();
      const float loss = chain.conversionLoss() * intrinsicLoss;
      return StreamInputPlan{strm, chain, loss};
    }

#endif // RAVL2_HAVE_JPEG

    // Registration: priority 10 to win over OpenCV handler (-1)
    [[maybe_unused]] bool g_regJpegFmt = inputFormatMap().add(std::make_shared<InputFormatCall>(
      "JPEGTurbo", "jpg,jpeg", "file", 10,
      [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan>
      {
#ifdef RAVL2_HAVE_JPEG
        // Create a persistent decode context so we don't reopen or re-read header twice
        auto decodeCtx = std::make_shared<JpegDecodeContext>(ctx.m_filename);
        if (!decodeCtx->file || !decodeCtx->created || !decodeCtx->headerOk) {
          if (ctx.m_verbose) {
            SPDLOG_INFO("JPEGTurbo: cannot open or parse JPEG: {}", ctx.m_filename);
          }
          return std::nullopt;
        }

        const bool isGray = (decodeCtx->cinfo.num_components == 1) || (decodeCtx->cinfo.jpeg_color_space == JCS_GRAYSCALE);
        const unsigned w = decodeCtx->cinfo.image_width;
        const unsigned h = decodeCtx->cinfo.image_height;

        if (ctx.m_verbose) {
          SPDLOG_INFO("JPEGTurbo probe: {}x{}, comps={}, colorspace={} for {}", w, h, decodeCtx->cinfo.num_components, int(decodeCtx->cinfo.jpeg_color_space), ctx.m_filename);
        }

        // Try grayscale first if file is grayscale
        if (isGray) {
          if (auto plan = buildPlanFor<Array<uint8_t, 2>>(ctx, decodeCtx)) {
            return plan;
          }
        }

        // Otherwise, use RGB packed as a minimal supported path
        if (auto plan = buildPlanFor<Array<PixelRGB8, 2>>(ctx, decodeCtx)) {
          return plan;
        }

        // If neither path is convertible to target, decline
        return std::nullopt;
#else
        // No JPEG library available; behave as stub
        if (ctx.m_verbose) {
          SPDLOG_INFO("JPEGTurbo probe active for file: {} (no libjpeg found)", ctx.m_filename);
        }
        return std::nullopt;
#endif
      }));
  }

  void initJpegTurboImageIO()
  {
    // Intentionally empty; exists to force TU linkage when called by clients.
  }
}
