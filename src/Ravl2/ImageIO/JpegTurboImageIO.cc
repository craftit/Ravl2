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

    template <typename ViaT>
    std::optional<StreamInputPlan> buildPlanFor(const ProbeInputContext &ctx)
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
      auto strm = std::make_shared<StreamInputCall<ViaT>>([filename = ctx.m_filename](std::streampos &pos) -> std::optional<ViaT>
      {
        if (pos != 0) {
          return std::nullopt;
        }

        FilePtr infile(std::fopen(filename.c_str(), "rb"));
        if (!infile) {
          SPDLOG_WARN("JPEGTurbo: failed to open file {}", filename);
          return std::nullopt;
        }

        jpeg_decompress_struct cinfo{};
        jpeg_error_mgr jerr{};
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile.get());

        if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
          jpeg_destroy_decompress(&cinfo);
          SPDLOG_WARN("JPEGTurbo: invalid JPEG header in {}", filename);
          return std::nullopt;
        }

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
          jpeg_destroy_decompress(&cinfo);
          return std::nullopt;
        }

        if (!jpeg_start_decompress(&cinfo)) {
          jpeg_destroy_decompress(&cinfo);
          SPDLOG_WARN("JPEGTurbo: failed to start decompression for {}", filename);
          return std::nullopt;
        }

        const JDIMENSION width = cinfo.output_width;
        const JDIMENSION height = cinfo.output_height;
        const int comps = cinfo.output_components; // 1 for gray, 3 for RGB
        const JDIMENSION rowStride = width * static_cast<JDIMENSION>(comps);

        JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, rowStride, 1);

        if constexpr (std::is_same_v<ViaT, Array<uint8_t, 2>>) {
          ViaT out({int(height), int(width)});
          JDIMENSION y = 0;
          while (cinfo.output_scanline < height) {
            jpeg_read_scanlines(&cinfo, buffer, 1);
            auto rowPtr = buffer[0];
            for (JDIMENSION x = 0; x < width; ++x) {
              out[{int(y), int(x)}] = rowPtr[x];
            }
            ++y;
          }
          jpeg_finish_decompress(&cinfo);
          jpeg_destroy_decompress(&cinfo);
          pos = 1;
          return out;
        } else if constexpr (std::is_same_v<ViaT, Array<PixelRGB8, 2>>) {
          ViaT out({int(height), int(width)});
          JDIMENSION y = 0;
          while (cinfo.output_scanline < height) {
            jpeg_read_scanlines(&cinfo, buffer, 1);
            const uint8_t* rowPtr = buffer[0];
            for (JDIMENSION x = 0; x < width; ++x) {
              const uint8_t r = rowPtr[x * 3 + 0];
              const uint8_t g = rowPtr[x * 3 + 1];
              const uint8_t b = rowPtr[x * 3 + 2];
              out[{int(y), int(x)}] = PixelRGB8{r, g, b};
            }
            ++y;
          }
          jpeg_finish_decompress(&cinfo);
          jpeg_destroy_decompress(&cinfo);
          pos = 1;
          return out;
        }

        // Not reached
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
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
        // Open and read header to decide between gray and color decode
        FilePtr infile(std::fopen(ctx.m_filename.c_str(), "rb"));
        if (!infile) {
          if (ctx.m_verbose) {
            SPDLOG_INFO("JPEGTurbo: cannot open {}", ctx.m_filename);
          }
          return std::nullopt;
        }

        jpeg_decompress_struct cinfo{};
        jpeg_error_mgr jerr{};
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile.get());
        if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
          jpeg_destroy_decompress(&cinfo);
          if (ctx.m_verbose) {
            SPDLOG_INFO("JPEGTurbo: not a valid JPEG: {}", ctx.m_filename);
          }
          return std::nullopt;
        }

        const bool isGray = (cinfo.num_components == 1) || (cinfo.jpeg_color_space == JCS_GRAYSCALE);
        const unsigned w = cinfo.image_width;
        const unsigned h = cinfo.image_height;

        if (ctx.m_verbose) {
          SPDLOG_INFO("JPEGTurbo probe: {}x{}, comps={}, colorspace={} for {}", w, h, cinfo.num_components, int(cinfo.jpeg_color_space), ctx.m_filename);
        }

        // Try grayscale first if file is grayscale
        if (isGray) {
          if (auto plan = buildPlanFor<Array<uint8_t, 2>>(ctx)) {
            return plan;
          }
        }

        // Otherwise, use RGB packed as a minimal supported path
        if (auto plan = buildPlanFor<Array<PixelRGB8, 2>>(ctx)) {
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
