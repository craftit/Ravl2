//
// PNG Image IO registration and initial implementation (stub when libpng missing)
//

#include "Ravl2/ImageIO/PngImageIO.hh"

#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Logging.hh"

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

#ifdef RAVL2_HAVE_PNG
#include <png.h>
#endif

namespace Ravl2
{
  namespace
  {

    // Small RAII context for decode; single-use and FILE*-based for now
#ifdef RAVL2_HAVE_PNG
    struct FileCloser {
      void operator()(FILE *f) const noexcept
      {
        if(f) std::fclose(f);
      }
    };
    using FilePtr = std::unique_ptr<FILE, FileCloser>;

    struct PngDecodeContext {
      FilePtr file;
      png_structp pngPtr {nullptr};
      png_infop infoPtr {nullptr};
      bool created {false};
      bool headerOk {false};
      bool consumed {false};
      // IHDR
      png_uint_32 width {0};
      png_uint_32 height {0};
      int bitDepth {0};
      int colorType {0};
      int interlace {0};

      explicit PngDecodeContext(const std::string &filename)
      {
        file.reset(std::fopen(filename.c_str(), "rb"));
        if(!file) return;
        pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if(!pngPtr) return;
        infoPtr = png_create_info_struct(pngPtr);
        if(!infoPtr) return;
        if(setjmp(png_jmpbuf(pngPtr))) {
          return;// error
        }
        png_init_io(pngPtr, file.get());
        png_read_info(pngPtr, infoPtr);
        png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType, &interlace, nullptr, nullptr);
        created = true;
        headerOk = true;
      }

      ~PngDecodeContext()
      {
        if(pngPtr || infoPtr) {
          if(pngPtr) {
            // libpng requires destroy with both pointers
            png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
          }
        }
      }
      PngDecodeContext(const PngDecodeContext &) = delete;
      PngDecodeContext &operator=(const PngDecodeContext &) = delete;
    };

    template <typename ViaT>
    std::optional<StreamInputPlan> buildPlanForInput(const ProbeInputContext &ctx, std::shared_ptr<PngDecodeContext> dctx)
    {
      auto chainOpt = typeConverterMap().find(ctx.m_targetType, typeid(ViaT));
      if(!chainOpt) return std::nullopt;

      auto strm = std::make_shared<StreamInputCall<ViaT>>([dctx, verbose = ctx.mVerbose](std::streampos &pos) -> std::optional<ViaT> {
        if(pos != 0) return std::nullopt;
        if(!dctx || !dctx->created || !dctx->headerOk || dctx->consumed) return std::nullopt;

        auto pngPtr = dctx->pngPtr;
        auto infoPtr = dctx->infoPtr;
        if(setjmp(png_jmpbuf(pngPtr))) {
          if(verbose) SPDLOG_WARN("PNGNative: decode error");
          return std::nullopt;
        }

        // Configure transforms to match ViaT
        int colorType = dctx->colorType;
        int bitDepth = dctx->bitDepth;

        // Expand palette and tRNS to RGBA when needed; gray 1/2/4 to 8
        if(colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(pngPtr);
        if(png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(pngPtr);
        if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) png_set_expand_gray_1_2_4_to_8(pngPtr);

        // Decide target channel/bit depth based on ViaT
        constexpr bool isGray8 = std::is_same_v<ViaT, Array<uint8_t, 2>>;
        constexpr bool isGray16 = std::is_same_v<ViaT, Array<uint16_t, 2>>;
        constexpr bool isRGB8 = std::is_same_v<ViaT, Array<PixelRGB8, 2>>;
        using RGB16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
        using RGBA16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;
        constexpr bool isRGB16 = std::is_same_v<ViaT, Array<RGB16Pixel, 2>>;
        constexpr bool isRGBA8 = std::is_same_v<ViaT, Array<PixelRGBA8, 2>>;
        constexpr bool isRGBA16 = std::is_same_v<ViaT, Array<RGBA16Pixel, 2>>;

        if constexpr(isGray8) {
          // Strip 16 to 8 if needed
          if(bitDepth == 16) png_set_strip_16(pngPtr);
          // If input is RGB/RGBA, drop to gray via libpng
          if(colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGB_ALPHA || colorType == PNG_COLOR_TYPE_PALETTE)
            png_set_rgb_to_gray_fixed(pngPtr, 1, -1, -1);
        } else if constexpr(isGray16) {
          if(bitDepth < 16) {
            // Promote 8-bit gray to 16 by left-shift (libpng lacks direct); we'll just expand by scale
            // Simpler approach: read as 8 then up-convert via TypeConverter; here we keep 16-bit only when source is 16
            // So if not 16-bit, strip to 8 and we will fail below to keep logic simple
          }
        } else if constexpr(isRGB8) {
          if(bitDepth == 16) png_set_strip_16(pngPtr);
          // Expand GA to RGBA then strip alpha if present
          if(colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(pngPtr);
          }
          if(png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_RGB_ALPHA) {
            png_set_strip_alpha(pngPtr);
          }
        } else if constexpr(isRGB16) {
          if(bitDepth < 16) {
            // No up-bit here; keep as-is and rely on converters outside; but libpng will deliver 8-bit. We'll bail out to keep semantics strict.
          }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
          png_set_swap(pngPtr);
#endif
          if(colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(pngPtr);
          }
          if(png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_RGB_ALPHA) {
            png_set_strip_alpha(pngPtr);
          }
        } else if constexpr(isRGBA8) {
          if(bitDepth == 16) png_set_strip_16(pngPtr);
          if(colorType == PNG_COLOR_TYPE_GRAY) {
            png_set_gray_to_rgb(pngPtr);
          }
          // Ensure alpha: if none, add fully opaque
          if(!(png_get_color_type(pngPtr, infoPtr) & PNG_COLOR_MASK_ALPHA)) {
            png_set_add_alpha(pngPtr, 0xFF, PNG_FILLER_AFTER);
          }
        } else if constexpr(isRGBA16) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
          png_set_swap(pngPtr);
#endif
          if(colorType == PNG_COLOR_TYPE_GRAY) {
            png_set_gray_to_rgb(pngPtr);
          }
          if(!(png_get_color_type(pngPtr, infoPtr) & PNG_COLOR_MASK_ALPHA)) {
            png_set_add_alpha(pngPtr, 0xFFFF, PNG_FILLER_AFTER);
          }
        }

        // Interlace handling
        int passes = 1;
        if(dctx->interlace == PNG_INTERLACE_ADAM7) {
          passes = png_set_interlace_handling(pngPtr);
        }
        png_read_update_info(pngPtr, infoPtr);

        const int outW = static_cast<int>(png_get_image_width(pngPtr, infoPtr));
        const int outH = static_cast<int>(png_get_image_height(pngPtr, infoPtr));

        // Allocate destination and row pointers
        ViaT out({outH, outW});
        std::vector<png_bytep> rows(static_cast<size_t>(outH));
        for(int y = 0; y < outH; ++y) {
          rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(&out[{y, 0}]);
        }

        for(int p = 0; p < passes; ++p) {
          png_read_image(pngPtr, rows.data());
        }
        png_read_end(pngPtr, nullptr);

        dctx->consumed = true;
        pos = 1;
        return out;
      });

      auto chain = chainOpt.value();
      const float loss = chain.conversionLoss();
      return StreamInputPlan {strm, chain, loss};
    }
#endif// RAVL2_HAVE_PNG

    // Input registration (priority 10) for .png files
    [[maybe_unused]] bool g_regPngIn = inputFormatMap().add(std::make_shared<InputFormatCall>(
      "PNGNative", "png", "file", 10,
      [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan> {
#ifdef RAVL2_HAVE_PNG
        // Quick signature check via look-ahead if present
        if(!ctx.m_data.empty()) {
          static const unsigned char kPngSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
          if(ctx.m_data.size() < 8 || !std::equal(ctx.m_data.begin(), ctx.m_data.begin() + 8, kPngSig)) {
            if(ctx.mVerbose) {
              SPDLOG_INFO("PNGNative: signature mismatch for {}", ctx.mFilename);
            }
            return std::nullopt;
          }
        }
        // Open and parse header, keep context for plan
        auto dctx = std::make_shared<PngDecodeContext>(ctx.mFilename);
        if(!dctx->created || !dctx->headerOk) {
          if(ctx.mVerbose) {
            SPDLOG_INFO("PNGNative: cannot open or parse PNG: {}", ctx.mFilename);
          }
          return std::nullopt;
        }

        // Score candidates based on IHDR
        using RGB16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
        using RGBA16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;

        struct Candidate {
          enum class Kind
          {
            RGBA8,
            RGB8,
            Gray8,
            RGB16,
            RGBA16,
            Gray16
          } kind;
          float score;
        };
        std::optional<Candidate> best;

        auto scoreVia = [&](const std::type_info &via, Candidate::Kind k) {
          auto ch = typeConverterMap().find(ctx.m_targetType, via);
          if(!ch) return;
          float s = ch->conversionLoss();
          if(!best || s > best->score) best = Candidate {k, s};
        };

        // Prefer keeping bit depth where possible; enumerate common ViaTs
        if(dctx->bitDepth == 16) {
          scoreVia(typeid(Array<RGBA16Pixel, 2>), Candidate::Kind::RGBA16);
          scoreVia(typeid(Array<RGB16Pixel, 2>), Candidate::Kind::RGB16);
          scoreVia(typeid(Array<uint16_t, 2>), Candidate::Kind::Gray16);
        } else {
          scoreVia(typeid(Array<PixelRGBA8, 2>), Candidate::Kind::RGBA8);
          scoreVia(typeid(Array<PixelRGB8, 2>), Candidate::Kind::RGB8);
          scoreVia(typeid(Array<uint8_t, 2>), Candidate::Kind::Gray8);
        }

        if(!best) return std::nullopt;
        if(ctx.mVerbose) {
          SPDLOG_INFO("PNGNative probe: {}x{} depth={} colorType={} → selected kind {} for {}",
                      dctx->height, dctx->width, dctx->bitDepth, dctx->colorType, int(best->kind), ctx.mFilename);
        }

        switch(best->kind) {
          case Candidate::Kind::RGBA8: return buildPlanForInput<Array<PixelRGBA8, 2>>(ctx, dctx);
          case Candidate::Kind::RGB8: return buildPlanForInput<Array<PixelRGB8, 2>>(ctx, dctx);
          case Candidate::Kind::Gray8: return buildPlanForInput<Array<uint8_t, 2>>(ctx, dctx);
          case Candidate::Kind::RGB16: return buildPlanForInput<Array<RGB16Pixel, 2>>(ctx, dctx);
          case Candidate::Kind::RGBA16: return buildPlanForInput<Array<RGBA16Pixel, 2>>(ctx, dctx);
          case Candidate::Kind::Gray16: return buildPlanForInput<Array<uint16_t, 2>>(ctx, dctx);
        }
        return std::nullopt;
#else
        if(ctx.mVerbose) {
          SPDLOG_INFO("PNGNative probe active for {} (no libpng found)", ctx.mFilename);
        }
        return std::nullopt;
#endif
      }));

    // Output registration (priority 10) for .png files
    [[maybe_unused]] bool g_regPngOut = outputFormatMap().add(std::make_shared<OutputFormatCall>(
      "PNGNative", "png", "file", 10,
      [](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan> {
#ifdef RAVL2_HAVE_PNG
        using RGB16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
        using RGBA16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;

        enum class Choice
        {
          None,
          RGBA8,
          RGB8,
          Gray8,
          RGB16,
          RGBA16,
          Gray16
        } choice = Choice::None;
        std::optional<ConversionChain> bestChain;

        auto consider = [&](const std::type_info &via, Choice c) {
          if(auto ch = typeConverterMap().find(via, ctx.m_sourceType)) {
            if(!bestChain || ch->conversionLoss() > bestChain->conversionLoss()) {
              bestChain = ch;
              choice = c;
            }
          }
        };

        consider(typeid(Array<RGBA16Pixel, 2>), Choice::RGBA16);
        consider(typeid(Array<RGB16Pixel, 2>), Choice::RGB16);
        consider(typeid(Array<uint16_t, 2>), Choice::Gray16);
        consider(typeid(Array<PixelRGBA8, 2>), Choice::RGBA8);
        consider(typeid(Array<PixelRGB8, 2>), Choice::RGB8);
        consider(typeid(Array<uint8_t, 2>), Choice::Gray8);

        if(!bestChain) return std::nullopt;

        if(ctx.m_verbose) {
          SPDLOG_INFO("PNGNative: selected output via kind {} for {}", int(choice), ctx.m_filename);
        }

        // Parse hints
        int compression = 6;
        bool interlace = false;
        int filter = PNG_ALL_FILTERS;// libpng will choose
        try {
          if(ctx.m_formatHint.contains("pngCompressionLevel")) compression = std::clamp(ctx.m_formatHint["pngCompressionLevel"].get<int>(), 0, 9);
          if(ctx.m_formatHint.contains("pngInterlace")) interlace = ctx.m_formatHint["pngInterlace"].get<bool>();
          if(ctx.m_formatHint.contains("pngFilter")) {
            const auto f = ctx.m_formatHint["pngFilter"].get<std::string>();
            if(f == "none") filter = PNG_FILTER_NONE;
            else if(f == "sub")
              filter = PNG_FILTER_SUB;
            else if(f == "up")
              filter = PNG_FILTER_UP;
            else if(f == "avg")
              filter = PNG_FILTER_AVG;
            else if(f == "paeth")
              filter = PNG_FILTER_PAETH;
            else
              filter = PNG_ALL_FILTERS;
          }
        } catch(...) { /* ignore malformed hints */
        }

        // RGB8
        if(choice == Choice::RGB8) {
          using ViaT = Array<PixelRGB8, 2>;
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, compression, interlace, filter](const ViaT &img, std::streampos pos) -> std::streampos {
            if(pos != 0) throw std::runtime_error("PNG output does not support seeking.");
            FILE *f = std::fopen(filename.c_str(), "wb");
            if(!f) throw std::runtime_error("Failed to open PNG for write");
            png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(!pngPtr) {
              std::fclose(f);
              throw std::runtime_error("png_create_write_struct failed");
            }
            png_infop infoPtr = png_create_info_struct(pngPtr);
            if(!infoPtr) {
              png_destroy_write_struct(&pngPtr, nullptr);
              std::fclose(f);
              throw std::runtime_error("png_create_info_struct failed");
            }
            if(setjmp(png_jmpbuf(pngPtr))) {
              png_destroy_write_struct(&pngPtr, &infoPtr);
              std::fclose(f);
              throw std::runtime_error("libpng write error");
            }
            png_init_io(pngPtr, f);
            const auto &rng = img.range();
            png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(rng.size(1)), static_cast<png_uint_32>(rng.size(0)), 8, PNG_COLOR_TYPE_RGB,
                         interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_set_compression_level(pngPtr, compression);
            if(filter != PNG_ALL_FILTERS) png_set_filter(pngPtr, 0, filter);
            png_write_info(pngPtr, infoPtr);
            std::vector<png_bytep> rows(static_cast<size_t>(rng.size(0)));
            for(int y = 0; y < rng.size(0); ++y) rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(const_cast<PixelRGB8 *>(&img[{y, 0}]));
            png_write_image(pngPtr, rows.data());
            png_write_end(pngPtr, nullptr);
            png_destroy_write_struct(&pngPtr, &infoPtr);
            std::fclose(f);
            return 0;
          });
          const float loss = bestChain->conversionLoss();
          return StreamOutputPlan {std::move(strm), *bestChain, loss};
        }

        // RGBA8
        if(choice == Choice::RGBA8) {
          using ViaT = Array<PixelRGBA8, 2>;
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, compression, interlace, filter](const ViaT &img, std::streampos pos) -> std::streampos {
            if(pos != 0) throw std::runtime_error("PNG output does not support seeking.");
            FILE *f = std::fopen(filename.c_str(), "wb");
            if(!f) throw std::runtime_error("Failed to open PNG for write");
            png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(!pngPtr) {
              std::fclose(f);
              throw std::runtime_error("png_create_write_struct failed");
            }
            png_infop infoPtr = png_create_info_struct(pngPtr);
            if(!infoPtr) {
              png_destroy_write_struct(&pngPtr, nullptr);
              std::fclose(f);
              throw std::runtime_error("png_create_info_struct failed");
            }
            if(setjmp(png_jmpbuf(pngPtr))) {
              png_destroy_write_struct(&pngPtr, &infoPtr);
              std::fclose(f);
              throw std::runtime_error("libpng write error");
            }
            png_init_io(pngPtr, f);
            const auto &rng = img.range();
            png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(rng.size(1)), static_cast<png_uint_32>(rng.size(0)), 8, PNG_COLOR_TYPE_RGB_ALPHA,
                         interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_set_compression_level(pngPtr, compression);
            if(filter != PNG_ALL_FILTERS) png_set_filter(pngPtr, 0, filter);
            png_write_info(pngPtr, infoPtr);
            std::vector<png_bytep> rows(static_cast<size_t>(rng.size(0)));
            for(int y = 0; y < rng.size(0); ++y) rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(const_cast<PixelRGBA8 *>(&img[{y, 0}]));
            png_write_image(pngPtr, rows.data());
            png_write_end(pngPtr, nullptr);
            png_destroy_write_struct(&pngPtr, &infoPtr);
            std::fclose(f);
            return 0;
          });
          const float loss = bestChain->conversionLoss();
          return StreamOutputPlan {std::move(strm), *bestChain, loss};
        }

        // Gray8
        if(choice == Choice::Gray8) {
          using ViaT = Array<uint8_t, 2>;
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, compression, interlace, filter](const ViaT &img, std::streampos pos) -> std::streampos {
            if(pos != 0) throw std::runtime_error("PNG output does not support seeking.");
            FILE *f = std::fopen(filename.c_str(), "wb");
            if(!f) throw std::runtime_error("Failed to open PNG for write");
            png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(!pngPtr) {
              std::fclose(f);
              throw std::runtime_error("png_create_write_struct failed");
            }
            png_infop infoPtr = png_create_info_struct(pngPtr);
            if(!infoPtr) {
              png_destroy_write_struct(&pngPtr, nullptr);
              std::fclose(f);
              throw std::runtime_error("png_create_info_struct failed");
            }
            if(setjmp(png_jmpbuf(pngPtr))) {
              png_destroy_write_struct(&pngPtr, &infoPtr);
              std::fclose(f);
              throw std::runtime_error("libpng write error");
            }
            png_init_io(pngPtr, f);
            const auto &rng = img.range();
            png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(rng.size(1)), static_cast<png_uint_32>(rng.size(0)), 8, PNG_COLOR_TYPE_GRAY,
                         interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_set_compression_level(pngPtr, compression);
            if(filter != PNG_ALL_FILTERS) png_set_filter(pngPtr, 0, filter);
            png_write_info(pngPtr, infoPtr);
            std::vector<png_bytep> rows(static_cast<size_t>(rng.size(0)));
#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
            for(int y = 0; y < rng.size(0); ++y) rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(const_cast<uint8_t *>(&img[{y, 0}]));
#pragma GCC diagnostic pop
            png_write_image(pngPtr, rows.data());
            png_write_end(pngPtr, nullptr);
            png_destroy_write_struct(&pngPtr, &infoPtr);
            std::fclose(f);
            return 0;
          });
          const float loss = bestChain->conversionLoss();
          return StreamOutputPlan {std::move(strm), *bestChain, loss};
        }

        // Gray16
        if(choice == Choice::Gray16) {
          using ViaT = Array<uint16_t, 2>;
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, compression, interlace, filter](const ViaT &img, std::streampos pos) -> std::streampos {
            if(pos != 0) throw std::runtime_error("PNG output does not support seeking.");
            FILE *f = std::fopen(filename.c_str(), "wb");
            if(!f) throw std::runtime_error("Failed to open PNG for write");
            png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(!pngPtr) {
              std::fclose(f);
              throw std::runtime_error("png_create_write_struct failed");
            }
            png_infop infoPtr = png_create_info_struct(pngPtr);
            if(!infoPtr) {
              png_destroy_write_struct(&pngPtr, nullptr);
              std::fclose(f);
              throw std::runtime_error("png_create_info_struct failed");
            }
            if(setjmp(png_jmpbuf(pngPtr))) {
              png_destroy_write_struct(&pngPtr, &infoPtr);
              std::fclose(f);
              throw std::runtime_error("libpng write error");
            }
            png_init_io(pngPtr, f);
            const auto &rng = img.range();
            png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(rng.size(1)), static_cast<png_uint_32>(rng.size(0)), 16, PNG_COLOR_TYPE_GRAY,
                         interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            png_set_swap(pngPtr);
#endif
            png_set_compression_level(pngPtr, compression);
            if(filter != PNG_ALL_FILTERS) png_set_filter(pngPtr, 0, filter);
            png_write_info(pngPtr, infoPtr);
            std::vector<png_bytep> rows(static_cast<size_t>(rng.size(0)));
            for(int y = 0; y < rng.size(0); ++y) rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(const_cast<uint16_t *>(&img[{y, 0}]));
            png_write_image(pngPtr, rows.data());
            png_write_end(pngPtr, nullptr);
            png_destroy_write_struct(&pngPtr, &infoPtr);
            std::fclose(f);
            return 0;
          });
          const float loss = bestChain->conversionLoss();
          return StreamOutputPlan {std::move(strm), *bestChain, loss};
        }

        // RGB16
        if(choice == Choice::RGB16) {
          using ViaT = Array<RGB16Pixel, 2>;
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, compression, interlace, filter](const ViaT &img, std::streampos pos) -> std::streampos {
            if(pos != 0) throw std::runtime_error("PNG output does not support seeking.");
            FILE *f = std::fopen(filename.c_str(), "wb");
            if(!f) throw std::runtime_error("Failed to open PNG for write");
            png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(!pngPtr) {
              std::fclose(f);
              throw std::runtime_error("png_create_write_struct failed");
            }
            png_infop infoPtr = png_create_info_struct(pngPtr);
            if(!infoPtr) {
              png_destroy_write_struct(&pngPtr, nullptr);
              std::fclose(f);
              throw std::runtime_error("png_create_info_struct failed");
            }
            if(setjmp(png_jmpbuf(pngPtr))) {
              png_destroy_write_struct(&pngPtr, &infoPtr);
              std::fclose(f);
              throw std::runtime_error("libpng write error");
            }
            png_init_io(pngPtr, f);
            const auto &rng = img.range();
            png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(rng.size(1)), static_cast<png_uint_32>(rng.size(0)), 16, PNG_COLOR_TYPE_RGB,
                         interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            png_set_swap(pngPtr);
#endif
            png_set_compression_level(pngPtr, compression);
            if(filter != PNG_ALL_FILTERS) png_set_filter(pngPtr, 0, filter);
            png_write_info(pngPtr, infoPtr);
            std::vector<png_bytep> rows(static_cast<size_t>(rng.size(0)));
            for(int y = 0; y < rng.size(0); ++y) rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(const_cast<RGB16Pixel *>(&img[{y, 0}]));
            png_write_image(pngPtr, rows.data());
            png_write_end(pngPtr, nullptr);
            png_destroy_write_struct(&pngPtr, &infoPtr);
            std::fclose(f);
            return 0;
          });
          const float loss = bestChain->conversionLoss();
          return StreamOutputPlan {std::move(strm), *bestChain, loss};
        }

        // RGBA16
        if(choice == Choice::RGBA16) {
          using ViaT = Array<RGBA16Pixel, 2>;
          auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename = ctx.m_filename, compression, interlace, filter](const ViaT &img, std::streampos pos) -> std::streampos {
            if(pos != 0) throw std::runtime_error("PNG output does not support seeking.");
            FILE *f = std::fopen(filename.c_str(), "wb");
            if(!f) throw std::runtime_error("Failed to open PNG for write");
            png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if(!pngPtr) {
              std::fclose(f);
              throw std::runtime_error("png_create_write_struct failed");
            }
            png_infop infoPtr = png_create_info_struct(pngPtr);
            if(!infoPtr) {
              png_destroy_write_struct(&pngPtr, nullptr);
              std::fclose(f);
              throw std::runtime_error("png_create_info_struct failed");
            }
            if(setjmp(png_jmpbuf(pngPtr))) {
              png_destroy_write_struct(&pngPtr, &infoPtr);
              std::fclose(f);
              throw std::runtime_error("libpng write error");
            }
            png_init_io(pngPtr, f);
            const auto &rng = img.range();
            png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(rng.size(1)), static_cast<png_uint_32>(rng.size(0)), 16, PNG_COLOR_TYPE_RGB_ALPHA,
                         interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            png_set_swap(pngPtr);
#endif
            png_set_compression_level(pngPtr, compression);
            if(filter != PNG_ALL_FILTERS) png_set_filter(pngPtr, 0, filter);
            png_write_info(pngPtr, infoPtr);
            std::vector<png_bytep> rows(static_cast<size_t>(rng.size(0)));
            for(int y = 0; y < rng.size(0); ++y) rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(const_cast<RGBA16Pixel *>(&img[{y, 0}]));
            png_write_image(pngPtr, rows.data());
            png_write_end(pngPtr, nullptr);
            png_destroy_write_struct(&pngPtr, &infoPtr);
            std::fclose(f);
            return 0;
          });
          const float loss = bestChain->conversionLoss();
          return StreamOutputPlan {std::move(strm), *bestChain, loss};
        }

        return std::nullopt;
#else
        if(ctx.m_verbose) {
          SPDLOG_INFO("PNGNative output probe active for {} (no libpng found)", ctx.m_filename);
        }
        return std::nullopt;
#endif
      }));
  }// namespace

  void initPngImageIO()
  {
    // Nothing required yet; presence ensures TU linkage
  }
}// namespace Ravl2
