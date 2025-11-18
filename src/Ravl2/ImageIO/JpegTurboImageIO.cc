//
// JPEG Turbo Image IO registration and minimal implementation
//
// This translation unit registers a high-priority JPEG input format and provides
// a lightweight implementation that:
//  - Probes using libjpeg only enough to decide viable native output types
//  - Scores candidates against the requested target type via the TypeConverter
//    map and picks the highest preserved-bits path (best conversionLoss)
//  - Decodes lazily on first read using a captured RAII decode context
//
// See ImageIO/JpegLoaderPlan.md for the broader plan (including planar YUV paths).
//

#include "Ravl2/ImageIO/JpegTurboImageIO.hh"

#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/Logging.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/PixelPlane.hh"

#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <algorithm>
#include <cstring>

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

    //! RAII context that keeps the file open and a prepared jpeg_decompress_struct.
    //! Header is read in the constructor so we don't need to reopen/parse again later.
    //!
    //! Thread-safety: one context is intended for single-use by a single plan/stream
    //! instance. The 'consumed' flag is used to guard against multiple decodes.
    struct JpegDecodeContext {
      FilePtr file;                // Open file handle (stdin source for libjpeg)
      jpeg_decompress_struct cinfo{}; // Decompress struct
      jpeg_error_mgr jerr{};          // Error manager
      bool created{false};
      bool headerOk{false};
      bool consumed{false};          // Ensure single-use
      // Cached sampling info
      int hsamp[3]{1,1,1};
      int vsamp[3]{1,1,1};
      int maxHs{1};
      int maxVs{1};

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
        if (headerOk) {
          maxHs = 1; maxVs = 1;
          for (int i = 0; i < static_cast<int>(cinfo.num_components) && i < 3; ++i) {
            hsamp[i] = cinfo.comp_info[i].h_samp_factor;
            vsamp[i] = cinfo.comp_info[i].v_samp_factor;
            if (hsamp[i] > maxHs) maxHs = hsamp[i];
            if (vsamp[i] > maxVs) maxVs = vsamp[i];
          }
        }
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
        } else if constexpr (std::is_same_v<ViaT, YUV444Image<uint8_t>> ||
                             std::is_same_v<ViaT, YUV422Image<uint8_t>> ||
                             std::is_same_v<ViaT, YUV420Image<uint8_t>>) {
          // Planar raw output
          cinfo.raw_data_out = TRUE;
          cinfo.out_color_space = JCS_YCbCr;
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
          // Whilst we should do this check, the class only has pixel values so it
          // as the size assert proves, and we don't change their type so we can overwrite them without worry.
          //static_assert(std::is_trivially_copyable_v<PixelRGB8>, "PixelRGB8 must be trivially copyable");
          for (JDIMENSION y = 0; y < height; ++y) {
            JSAMPROW row = reinterpret_cast<JSAMPROW>(&out[{int(y), 0}]);
            JSAMPARRAY rows = &row;
            jpeg_read_scanlines(&cinfo, rows, 1);
          }
          jpeg_finish_decompress(&cinfo);
          sharedCtx->consumed = true;
          pos = 1;
          return out;
        } else if constexpr (std::is_same_v<ViaT, YUV444Image<uint8_t>> ||
                             std::is_same_v<ViaT, YUV422Image<uint8_t>> ||
                             std::is_same_v<ViaT, YUV420Image<uint8_t>>) {
          // Raw planar decode path
          // Build planar image with master range = height x width
          // Build explicit min/max indices to avoid constructor ambiguity
          Index<2> minIdx{0, 0};
          Index<2> maxIdx{static_cast<int>(height - 1), static_cast<int>(width - 1)};
          ViaT out(IndexRange<2>(minIdx, maxIdx));

          // Compute iMCU-based row counts per component
          const int max_v = sharedCtx->maxVs; // typically 2 for 420, 2 for 422 (vertical 1), 1 for 444
          const JDIMENSION y_lines_per_iMCU = static_cast<JDIMENSION>(max_v * DCTSIZE);
          // Set up JSAMPARRAY arrays for each component with required height per iMCU
          JSAMPARRAY ybuf = (*cinfo.mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE,
                                                       cinfo.output_width, y_lines_per_iMCU);
          // For chroma, rows per iMCU depend on v_samp_factor of Cb/Cr
          const JDIMENSION cb_vs = static_cast<JDIMENSION>((sharedCtx->cinfo.num_components > 1) ? sharedCtx->cinfo.comp_info[1].v_samp_factor : 1);
          const JDIMENSION cb_hs = static_cast<JDIMENSION>((sharedCtx->cinfo.num_components > 1) ? sharedCtx->cinfo.comp_info[1].h_samp_factor : 1);
          const JDIMENSION maxHs = static_cast<JDIMENSION>(sharedCtx->maxHs);
          const JDIMENSION maxVs = static_cast<JDIMENSION>(sharedCtx->maxVs);
          const JDIMENSION cb_width = static_cast<JDIMENSION>((cinfo.output_width * cb_hs + maxHs - 1) / maxHs);
          const JDIMENSION cb_lines_per_iMCU = static_cast<JDIMENSION>(cb_vs * DCTSIZE);
          JSAMPARRAY cbbuf = (*cinfo.mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE,
                                                        cb_width, cb_lines_per_iMCU);
          JSAMPARRAY crbuf = (*cinfo.mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE,
                                                        cb_width, cb_lines_per_iMCU);

          JDIMENSION yPos = 0;
          while (yPos < height) {
            JSAMPARRAY bufs[3] = { ybuf, cbbuf, crbuf };
            JDIMENSION nread = jpeg_read_raw_data(&cinfo, bufs, y_lines_per_iMCU);
            if (nread == 0) break;

            // Copy into destination planes
            // Y plane: nread lines starting at yPos
            for (JDIMENSION r = 0; r < nread && (yPos + r) < height; ++r) {
              auto &yPlane = out.template planeByChannel<ImageChannel::Luminance>();
              uint8_t *dst = &yPlane.data()[{static_cast<int>(yPos + r), 0}];
              std::memcpy(dst, ybuf[r], static_cast<size_t>(width));
            }
            // Compute chroma row count corresponding to these luma lines
            const JDIMENSION chromaRows = static_cast<JDIMENSION>((nread * cb_vs + maxVs - 1) / maxVs);
            const JDIMENSION chromaHeight = static_cast<JDIMENSION>((height * cb_vs + maxVs - 1) / maxVs);
            const JDIMENSION yPosChroma = static_cast<JDIMENSION>((yPos * cb_vs) / maxVs);

            // U and V planes
            auto &uPlane = out.template planeByChannel<ImageChannel::ChrominanceU>();
            auto &vPlane = out.template planeByChannel<ImageChannel::ChrominanceV>();
            for (JDIMENSION r = 0; r < chromaRows && (yPosChroma + r) < chromaHeight; ++r) {
              uint8_t *udst = &uPlane.data()[{static_cast<int>(yPosChroma + r), 0}];
              uint8_t *vdst = &vPlane.data()[{static_cast<int>(yPosChroma + r), 0}];
              std::memcpy(udst, cbbuf[r], static_cast<size_t>(cb_width));
              std::memcpy(vdst, crbuf[r], static_cast<size_t>(cb_width));
            }

            yPos += nread;
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

    // Helper to score a candidate ViaT without building a stream
    template <typename ViaT>
    std::optional<float> scoreCandidate(const ProbeInputContext &ctx, float intrinsicLoss = 1.0f) {
      auto chainOpt = typeConverterMap().find(ctx.m_targetType, typeid(ViaT));
      if (!chainOpt.has_value()) return std::nullopt;
      return chainOpt->conversionLoss() * intrinsicLoss;
    }

#endif // RAVL2_HAVE_JPEG

    // Registration: priority 10 to win over OpenCV handler (-1)
    [[maybe_unused]] bool g_regJpegFmt = inputFormatMap().add(std::make_shared<InputFormatCall>(
      "JPEGTurbo", "jpg,jpeg", "file", 10,
      [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan>
      {
#ifdef RAVL2_HAVE_JPEG
        // Quick signature check using look-ahead data if available
        if (!ctx.m_data.empty()) {
          if (ctx.m_data.size() < 2 || ctx.m_data[0] != 0xFF || ctx.m_data[1] != 0xD8) {
            // Not a JPEG SOI; decline quietly unless verbose
            if (ctx.m_verbose) {
              SPDLOG_INFO("JPEGTurbo: look-ahead does not match JPEG magic for {}", ctx.m_filename);
            }
            return std::nullopt;
          }
        }

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

        // Detect subsampling category for color JPEG (only when native JPEG colorspace is YCbCr)
        enum class Subsampling { S444, S422, S420, Unknown };
        Subsampling subs = Subsampling::Unknown;
        const bool isYCbCr = (!isGray && decodeCtx->cinfo.jpeg_color_space == JCS_YCbCr && decodeCtx->cinfo.num_components >= 3);
        if (isYCbCr) {
          const int hs = decodeCtx->hsamp[1];
          const int vs = decodeCtx->vsamp[1];
          const int maxHs = decodeCtx->maxHs;
          const int maxVs = decodeCtx->maxVs;
          const int hRatio = (maxHs == 0) ? 1 : (maxHs / std::max(1, hs));
          const int vRatio = (maxVs == 0) ? 1 : (maxVs / std::max(1, vs));
          if (hRatio == 1 && vRatio == 1) subs = Subsampling::S444;
          else if (hRatio == 2 && vRatio == 1) subs = Subsampling::S422;
          else if ((hRatio == 2 && vRatio == 2) || (hRatio == 2 && vRatio == 2)) subs = Subsampling::S420;
          else subs = Subsampling::Unknown;
        }

        // Score candidates and pick the best conversion-preserving plan.
        std::optional<float> bestScore;
        enum class Choice { None, Gray, RGB, YUV444, YUV422, YUV420 } bestChoice = Choice::None;

        if (isGray) {
          if (auto s = scoreCandidate<Array<uint8_t, 2>>(ctx, 1.0f)) {
            bestScore = s;
            bestChoice = Choice::Gray;
          }
          if (auto s = scoreCandidate<Array<PixelRGB8, 2>>(ctx, 1.0f)) {
            if (!bestScore || *s > *bestScore) {
              bestScore = s;
              bestChoice = Choice::RGB;
            }
          }
        } else {
          // Consider planar candidates first matching subsampling with intrinsicLoss 1.0
          if (subs == Subsampling::S444) {
            if (auto s = scoreCandidate<YUV444Image<uint8_t>>(ctx, 1.0f)) { bestScore = s; bestChoice = Choice::YUV444; }
          } else if (subs == Subsampling::S422) {
            if (auto s = scoreCandidate<YUV422Image<uint8_t>>(ctx, 1.0f)) { bestScore = s; bestChoice = Choice::YUV422; }
          } else if (subs == Subsampling::S420) {
            if (auto s = scoreCandidate<YUV420Image<uint8_t>>(ctx, 1.0f)) { bestScore = s; bestChoice = Choice::YUV420; }
          }
          // Always consider RGB8; if subsampled, apply small intrinsic loss for upsampling
          const float rgbIntrinsic = (subs == Subsampling::S444) ? 1.0f : 0.96f;
          if (auto s = scoreCandidate<Array<PixelRGB8, 2>>(ctx, rgbIntrinsic)) {
            if (!bestScore || *s > *bestScore) { bestScore = s; bestChoice = Choice::RGB; }
          }
        }

        if (!bestScore) {
          return std::nullopt;
        }

        if (ctx.m_verbose) {
          const char* choiceName = "None";
          switch (bestChoice) {
            case Choice::Gray: choiceName = "Gray"; break;
            case Choice::RGB: choiceName = "RGB"; break;
            case Choice::YUV444: choiceName = "YUV444"; break;
            case Choice::YUV422: choiceName = "YUV422"; break;
            case Choice::YUV420: choiceName = "YUV420"; break;
            default: break;
          }
          SPDLOG_INFO("JPEGTurbo: selected {} path with score {}", choiceName, *bestScore);
        }

        switch (bestChoice) {
          case Choice::Gray: return buildPlanFor<Array<uint8_t, 2>>(ctx, decodeCtx);
          case Choice::RGB:  return buildPlanFor<Array<PixelRGB8, 2>>(ctx, decodeCtx);
          case Choice::YUV444: return buildPlanFor<YUV444Image<uint8_t>>(ctx, decodeCtx);
          case Choice::YUV422: return buildPlanFor<YUV422Image<uint8_t>>(ctx, decodeCtx);
          case Choice::YUV420: return buildPlanFor<YUV420Image<uint8_t>>(ctx, decodeCtx);
          default: break;
        }
        return std::nullopt; // Shouldn't happen
#else
        // No JPEG library available; behave as stub
        if (ctx.m_verbose) {
          SPDLOG_INFO("JPEGTurbo probe active for file: {} (no libjpeg found)", ctx.m_filename);
        }
        return std::nullopt;
#endif
      }));

    // Output (save) registration: priority 10 to win over OpenCV handler (-1)
    [[maybe_unused]] bool g_regJpegOut = outputFormatMap().add(std::make_shared<OutputFormatCall>(
      "JPEGTurbo", "jpg,jpeg", "file", 10,
      [](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan>
      {
#ifdef RAVL2_HAVE_JPEG
        // Helper to compute score for converting from source -> ViaT
        auto scoreFor = [&](const std::type_info &via) -> std::optional<ConversionChain> {
          auto chainOpt = typeConverterMap().find(via, ctx.m_sourceType);
          if (!chainOpt.has_value()) return std::nullopt;
          return chainOpt; // loss carried inside chain
        };

        enum class Choice { None, Gray, RGB } choice = Choice::None;
        std::optional<ConversionChain> bestChain;

        // Consider RGB first (typical)
        if (auto ch = scoreFor(typeid(Array<PixelRGB8, 2>))) {
          bestChain = ch;
          choice = Choice::RGB;
        }
        // Consider grayscale as alternative (if source is 1-channel etc.)
        if (auto ch = scoreFor(typeid(Array<uint8_t, 2>))) {
          if (!bestChain || ch->conversionLoss() > bestChain->conversionLoss()) {
            bestChain = ch;
            choice = Choice::Gray;
          }
        }

        if (!bestChain) {
          if (ctx.m_verbose) {
            SPDLOG_INFO("JPEGTurbo: no conversion chain from {} to JPEG via RGB8 or Y8", typeName(ctx.m_sourceType));
          }
          return std::nullopt;
        }

        // Quality hint (0-100), default 90
        int quality = 90;
        try {
          if (ctx.m_formatHint.contains("jpegQuality")) {
            quality = std::clamp(ctx.m_formatHint["jpegQuality"].get<int>(), 1, 100);
          } else if (ctx.m_formatHint.contains("quality")) {
            quality = std::clamp(ctx.m_formatHint["quality"].get<int>(), 1, 100);
          }
        } catch (...) {
          // Ignore malformed hint
        }

        if (ctx.m_verbose) {
          SPDLOG_INFO("JPEGTurbo: selected {} output path (quality={}) for {}",
                      (choice == Choice::RGB ? "RGB8" : "Y8"), quality, ctx.m_filename);
        }

        // Build the output stream for chosen ViaT
        if (choice == Choice::RGB) {
          using ViaT = Array<PixelRGB8, 2>;
          static_assert(sizeof(PixelRGB8) == 3, "PixelRGB8 must be 3 bytes");
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, quality](const ViaT &img, std::streampos pos) -> std::streampos {
            if (pos != 0) {
              throw std::runtime_error("JPEGTurbo output format does not support seeking.");
            }
            FilePtr file(std::fopen(filename.c_str(), "wb"));
            if (!file) {
              throw std::runtime_error("Failed to open file for writing JPEG");
            }

            jpeg_compress_struct cinfo{};
            jpeg_error_mgr jerr{};
            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_compress(&cinfo);
            jpeg_stdio_dest(&cinfo, file.get());

            const auto &rng = img.range();
            cinfo.image_width = static_cast<JDIMENSION>(rng.size(1));
            cinfo.image_height = static_cast<JDIMENSION>(rng.size(0));
            cinfo.input_components = 3;
#ifdef JCS_EXTENSIONS
            cinfo.in_color_space = JCS_EXT_RGB;
#else
            cinfo.in_color_space = JCS_RGB;
#endif
            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, quality, TRUE);

            jpeg_start_compress(&cinfo, TRUE);

            //const JDIMENSION width = cinfo.image_width;
            const JDIMENSION height = cinfo.image_height;
            for (JDIMENSION y = 0; y < height; ++y) {
              JSAMPROW row = reinterpret_cast<JSAMPROW>(const_cast<PixelRGB8*>(&img[{static_cast<int>(y), 0}]));
              JSAMPARRAY rows = &row;
              jpeg_write_scanlines(&cinfo, rows, 1);
            }

            jpeg_finish_compress(&cinfo);
            jpeg_destroy_compress(&cinfo);
            return 0;
          });

          const float intrinsicLoss = 1.0f;
          const float loss = bestChain->conversionLoss() * intrinsicLoss;
          return StreamOutputPlan{std::move(strm), *bestChain, loss};
        } else if (choice == Choice::Gray) {
          using ViaT = Array<uint8_t, 2>;
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, quality](const ViaT &img, std::streampos pos) -> std::streampos {
            if (pos != 0) {
              throw std::runtime_error("JPEGTurbo output format does not support seeking.");
            }
            FilePtr file(std::fopen(filename.c_str(), "wb"));
            if (!file) {
              throw std::runtime_error("Failed to open file for writing JPEG");
            }

            jpeg_compress_struct cinfo{};
            jpeg_error_mgr jerr{};
            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_compress(&cinfo);
            jpeg_stdio_dest(&cinfo, file.get());

            const auto &rng = img.range();
            cinfo.image_width = static_cast<JDIMENSION>(rng.size(1));
            cinfo.image_height = static_cast<JDIMENSION>(rng.size(0));
            cinfo.input_components = 1;
            cinfo.in_color_space = JCS_GRAYSCALE;
            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, quality, TRUE);

            jpeg_start_compress(&cinfo, TRUE);

            const JDIMENSION width = cinfo.image_width;
            const JDIMENSION height = cinfo.image_height;
            (void)width;
            for (JDIMENSION y = 0; y < height; ++y) {
              JSAMPROW row = reinterpret_cast<JSAMPROW>(const_cast<uint8_t*>(&img[{static_cast<int>(y), 0}]));
              JSAMPARRAY rows = &row;
              jpeg_write_scanlines(&cinfo, rows, 1);
            }

            jpeg_finish_compress(&cinfo);
            jpeg_destroy_compress(&cinfo);
            return 0;
          });

          const float intrinsicLoss = 1.0f;
          const float loss = bestChain->conversionLoss() * intrinsicLoss;
          return StreamOutputPlan{std::move(strm), *bestChain, loss};
        }

        return std::nullopt;
#else
        // No JPEG library; let other handlers (e.g., OpenCV) take it
        if (ctx.m_verbose) {
          SPDLOG_INFO("JPEGTurbo output probe active for file: {} (no libjpeg found)", ctx.m_filename);
        }
        return std::nullopt;
#endif
      }));
  }

  void initJpegTurboImageIO()
  {
    // Ensure plane converters are registered for planar YUV candidates used by JPEG IO
    initPlaneConversion();
  }
}
