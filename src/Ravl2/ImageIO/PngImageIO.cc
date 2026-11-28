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
#include <stdexcept>

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

    using RGB16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>;
    using RGBA16Pixel = Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>;

    //! libpng's errors longjmp by default, which would skip C++ destructors. Throw instead; callers catch.
    //! (libpng is built with unwind tables on the GCC/Clang targets we support.)
    [[noreturn]] void pngError(png_structp, png_const_charp msg)
    {
      throw std::runtime_error(std::string("libpng: ") + msg);
    }

    void pngWarning(png_structp, png_const_charp msg)
    {
      SPDLOG_WARN("libpng: {}", msg);
    }

    //! PNG layout of each pixel type we read and write directly.
    template <typename PixelT>
    struct PngPixelTraits;
    template <>
    struct PngPixelTraits<uint8_t> {
      static constexpr int bitDepth = 8, channels = 1, colorType = PNG_COLOR_TYPE_GRAY;
    };
    template <>
    struct PngPixelTraits<PixelRGB8> {
      static constexpr int bitDepth = 8, channels = 3, colorType = PNG_COLOR_TYPE_RGB;
    };
    template <>
    struct PngPixelTraits<PixelRGBA8> {
      static constexpr int bitDepth = 8, channels = 4, colorType = PNG_COLOR_TYPE_RGB_ALPHA;
    };
    template <>
    struct PngPixelTraits<uint16_t> {
      static constexpr int bitDepth = 16, channels = 1, colorType = PNG_COLOR_TYPE_GRAY;
    };
    template <>
    struct PngPixelTraits<RGB16Pixel> {
      static constexpr int bitDepth = 16, channels = 3, colorType = PNG_COLOR_TYPE_RGB;
    };
    template <>
    struct PngPixelTraits<RGBA16Pixel> {
      static constexpr int bitDepth = 16, channels = 4, colorType = PNG_COLOR_TYPE_RGB_ALPHA;
    };

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
        pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, pngError, pngWarning);
        if(!pngPtr) return;
        infoPtr = png_create_info_struct(pngPtr);
        if(!infoPtr) return;
        try {
          png_init_io(pngPtr, file.get());
          png_read_info(pngPtr, infoPtr);
          png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType, &interlace, nullptr, nullptr);
        } catch(const std::exception &e) {
          SPDLOG_WARN("PNGNative: cannot read header of '{}': {}", filename, e.what());
          return;
        }
        created = true;
        headerOk = true;
      }

      ~PngDecodeContext()
      {
        if(pngPtr) {
          // libpng requires destroy with both pointers
          png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        }
      }
      PngDecodeContext(const PngDecodeContext &) = delete;
      PngDecodeContext &operator=(const PngDecodeContext &) = delete;
    };

    //! Set up libpng transforms so any source PNG decodes to PixelT, then decode.
    template <typename PixelT>
    std::optional<Array<PixelT, 2>> decodePng(PngDecodeContext &dctx)
    {
      using Traits = PngPixelTraits<PixelT>;
      auto pngPtr = dctx.pngPtr;
      auto infoPtr = dctx.infoPtr;
      const int colorType = dctx.colorType;
      const int bitDepth = dctx.bitDepth;
      // Palette images have the colour bit set.
      const bool srcColour = (colorType & PNG_COLOR_MASK_COLOR) != 0;
      const bool hasTrns = png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS) != 0;
      const bool srcAlpha = (colorType & PNG_COLOR_MASK_ALPHA) != 0 || hasTrns;
      constexpr bool wantColour = Traits::channels >= 3;
      constexpr bool wantAlpha = Traits::channels == 4;

      // Expand to at least 8 bits per channel, palette to RGB, tRNS to alpha.
      if(colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(pngPtr);
      if(hasTrns) png_set_tRNS_to_alpha(pngPtr);
      if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) png_set_expand_gray_1_2_4_to_8(pngPtr);

      // Bit depth.
      if constexpr(Traits::bitDepth == 8) {
        if(bitDepth == 16) png_set_strip_16(pngPtr);
      } else {
#ifdef PNG_READ_EXPAND_16_SUPPORTED
        if(bitDepth < 16) png_set_expand_16(pngPtr);
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        png_set_swap(pngPtr);// PNG stores 16-bit samples big-endian
#endif
      }

      // Colour and alpha.
      if(wantColour && !srcColour) png_set_gray_to_rgb(pngPtr);
      if(!wantColour && srcColour) png_set_rgb_to_gray_fixed(pngPtr, 1, -1, -1);
      if(wantAlpha && !srcAlpha) png_set_add_alpha(pngPtr, Traits::bitDepth == 16 ? 0xFFFF : 0xFF, PNG_FILLER_AFTER);
      if(!wantAlpha && srcAlpha) png_set_strip_alpha(pngPtr);

      // libpng handles all interlace passes inside a single png_read_image().
      png_set_interlace_handling(pngPtr);
      png_read_update_info(pngPtr, infoPtr);

      const int outW = static_cast<int>(png_get_image_width(pngPtr, infoPtr));
      const int outH = static_cast<int>(png_get_image_height(pngPtr, infoPtr));
      const auto rowBytes = png_get_rowbytes(pngPtr, infoPtr);
      if(png_get_channels(pngPtr, infoPtr) != Traits::channels || png_get_bit_depth(pngPtr, infoPtr) != Traits::bitDepth
         || rowBytes != static_cast<std::size_t>(outW) * sizeof(PixelT)) {
        SPDLOG_WARN("PNGNative: cannot decode {}-bit colour type {} as {} ({} bytes per row, expected {})", bitDepth, colorType,
                    typeName(typeid(PixelT)), rowBytes, static_cast<std::size_t>(outW) * sizeof(PixelT));
        return std::nullopt;
      }

      Array<PixelT, 2> out({outH, outW});
      std::vector<png_bytep> rows(static_cast<size_t>(outH));
      for(int y = 0; y < outH; ++y) {
        rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(&out[{y, 0}]);
      }
      png_read_image(pngPtr, rows.data());
      png_read_end(pngPtr, nullptr);
      return out;
    }

    template <typename ViaT>
    std::optional<StreamInputPlan> buildPlanForInput(const ProbeInputContext &ctx, std::shared_ptr<PngDecodeContext> dctx)
    {
      auto chainOpt = typeConverterMap().find(ctx.mTargetType, typeid(ViaT));
      if(!chainOpt) return std::nullopt;

      auto strm = std::make_shared<StreamInputCall<ViaT>>([dctx](std::streampos &pos) -> std::optional<ViaT> {
        if(pos != 0) return std::nullopt;
        if(!dctx || !dctx->created || !dctx->headerOk || dctx->consumed) return std::nullopt;
        // Single use: libpng's read state can't be rewound.
        dctx->consumed = true;
        try {
          auto out = decodePng<typename ViaT::value_type>(*dctx);
          if(out) pos = 1;
          return out;
        } catch(const std::exception &e) {
          SPDLOG_WARN("PNGNative: decode failed: {}", e.what());
          return std::nullopt;
        }
      });

      auto chain = chainOpt.value();
      const float loss = chain.conversionLoss();
      return StreamInputPlan {strm, chain, loss};
    }

    //! Destroys a write struct however the encoder exits.
    struct PngWriteGuard {
      png_structp pngPtr = nullptr;
      png_infop infoPtr = nullptr;
      ~PngWriteGuard()
      {
        if(pngPtr) png_destroy_write_struct(&pngPtr, &infoPtr);
      }
    };

    //! Write an image as PNG. Throws std::runtime_error on failure.
    template <typename PixelT>
    void writePng(const std::string &filename, const Array<PixelT, 2> &img, int compression, bool interlace, int filter)
    {
      using Traits = PngPixelTraits<PixelT>;
      FilePtr file(std::fopen(filename.c_str(), "wb"));
      if(!file) throw std::runtime_error("Failed to open PNG for write");
      PngWriteGuard guard;
      guard.pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, pngError, pngWarning);
      if(!guard.pngPtr) throw std::runtime_error("png_create_write_struct failed");
      guard.infoPtr = png_create_info_struct(guard.pngPtr);
      if(!guard.infoPtr) throw std::runtime_error("png_create_info_struct failed");
      auto pngPtr = guard.pngPtr;
      auto infoPtr = guard.infoPtr;

      png_init_io(pngPtr, file.get());
      const auto &rng = img.range();
      png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(rng.size(1)), static_cast<png_uint_32>(rng.size(0)), Traits::bitDepth, Traits::colorType,
                   interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
      png_set_compression_level(pngPtr, compression);
      if(filter != PNG_ALL_FILTERS) png_set_filter(pngPtr, 0, filter);
      png_write_info(pngPtr, infoPtr);
      if constexpr(Traits::bitDepth == 16) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        // PNG stores 16-bit samples big-endian. png_set_swap() only takes effect once the struct's bit depth
        // is known, which for writing is set by png_write_info(), so it must come after it.
        png_set_swap(pngPtr);
#endif
      }
      std::vector<png_bytep> rows(static_cast<size_t>(rng.size(0)));
      for(int y = 0; y < rng.size(0); ++y) {
        rows[static_cast<size_t>(y)] = reinterpret_cast<png_bytep>(const_cast<PixelT *>(&img[{rng.min(0) + y, rng.min(1)}]));
      }
      png_write_image(pngPtr, rows.data());
      png_write_end(pngPtr, nullptr);
    }

    template <typename PixelT>
    StreamOutputPlan makePngOutputPlan(const std::string &filename, const ConversionChain &chain, int compression, bool interlace, int filter)
    {
      using ViaT = Array<PixelT, 2>;
      auto strm = std::make_unique<StreamOutputCall<ViaT>>([filename, compression, interlace, filter](const ViaT &img, std::streampos pos) -> std::streampos {
        if(pos != 0) throw std::runtime_error("PNG output does not support seeking.");
        writePng(filename, img, compression, interlace, filter);
        return 0;
      });
      return StreamOutputPlan {std::move(strm), chain, chain.conversionLoss()};
    }
#endif// RAVL2_HAVE_PNG

    // Input registration (priority 10) for .png files
    [[maybe_unused]] bool g_regPngIn = inputFormatMap().add(std::make_shared<InputFormatCall>(
      "PNGNative", "png", "file", 10,
      [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan> {
#ifdef RAVL2_HAVE_PNG
        // Quick signature check via look-ahead if present
        if(!ctx.mData.empty()) {
          static const unsigned char kPngSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
          if(ctx.mData.size() < 8 || !std::equal(ctx.mData.begin(), ctx.mData.begin() + 8, kPngSig)) {
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
          auto ch = typeConverterMap().find(ctx.mTargetType, via);
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
          if(auto ch = typeConverterMap().find(via, ctx.mSourceType)) {
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

        if(ctx.mVerbose) {
          SPDLOG_INFO("PNGNative: selected output via kind {} for {}", int(choice), ctx.mFilename);
        }

        // Parse hints
        int compression = 6;
        bool interlace = false;
        int filter = PNG_ALL_FILTERS;// libpng will choose
        try {
          if(ctx.mFormatHint.contains("pngCompressionLevel")) compression = std::clamp(ctx.mFormatHint["pngCompressionLevel"].get<int>(), 0, 9);
          if(ctx.mFormatHint.contains("pngInterlace")) interlace = ctx.mFormatHint["pngInterlace"].get<bool>();
          if(ctx.mFormatHint.contains("pngFilter")) {
            const auto f = ctx.mFormatHint["pngFilter"].get<std::string>();
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

        switch(choice) {
          case Choice::RGB8: return makePngOutputPlan<PixelRGB8>(ctx.mFilename, *bestChain, compression, interlace, filter);
          case Choice::RGBA8: return makePngOutputPlan<PixelRGBA8>(ctx.mFilename, *bestChain, compression, interlace, filter);
          case Choice::Gray8: return makePngOutputPlan<uint8_t>(ctx.mFilename, *bestChain, compression, interlace, filter);
          case Choice::Gray16: return makePngOutputPlan<uint16_t>(ctx.mFilename, *bestChain, compression, interlace, filter);
          case Choice::RGB16: return makePngOutputPlan<RGB16Pixel>(ctx.mFilename, *bestChain, compression, interlace, filter);
          case Choice::RGBA16: return makePngOutputPlan<RGBA16Pixel>(ctx.mFilename, *bestChain, compression, interlace, filter);
          case Choice::None: break;
        }
        return std::nullopt;
#else
        if(ctx.mVerbose) {
          SPDLOG_INFO("PNGNative output probe active for {} (no libpng found)", ctx.mFilename);
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
