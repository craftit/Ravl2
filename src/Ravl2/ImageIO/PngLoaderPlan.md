PNG loader/saver design and implementation plan (libpng)

Overview
This document outlines how to add native PNG support based on libpng that decodes/encodes directly to Ravl2 containers, integrates with the IO probing/plan mechanism, and leverages the TypeConverter system to select the highest preserved‑bits path. Unlike JPEG, PNG is lossless, supports alpha, and offers multiple bit depths (1/2/4/8/16). We will focus on packed outputs at 8‑bit and 16‑bit depths to support both typical images and higher‑precision results (e.g., depth, scientific outputs). Palette support is not a priority but will be handled by lossless expansion when encountered.

Relevant infrastructure in Ravl2
- Input probing and planning
  - InputFormatCall and inputFormatMap(): register handler with (name, extensions, protocol, priority, probe lambda).
  - ProbeInputContext: immutable context with url, filename, protocol, extension, target C++ type, JSON hints, and optional look‑ahead bytes.
  - StreamInputPlan: stream callback that yields a native ViaT once (for still images) and a ConversionChain to the requested target; includes a preserved‑bits score (loss in [0,1], higher is better).

- Output probing and planning
  - OutputFormatCall and outputFormatMap(): register writer with (name, extensions, protocol, priority, probe lambda).
  - ProbeOutputContext: immutable context with url, source C++ type, and JSON hints.
  - StreamOutputPlan: stream callback that consumes a ViaT and writes, plus a ConversionChain from the user’s source type to ViaT.

- Type conversion and loss accounting
  - TypeConverter registry provides conversions between types and aggregates loss via ConversionChain::conversionLoss().
  - This PNG module selects a native ViaT that maximizes the final preserved‑bits score. Assume all required converters (Gray↔RGB(A), 8↔16, add/remove alpha, etc.) already exist.

Design goals for PNG
1) Decode/encode directly between libpng and Ravl2 containers with zero per‑pixel copies (row pointers alias destination rows).
2) Support multiple bit depths: primary focus on 8‑bit and 16‑bit for gray and RGB(A) paths.
3) Favor packed pixel outputs (PNG’s native layout); planar outputs are not used here (converters handle planar requests).
4) Provide accurate preserved‑bits scoring; expansions are lossless; down‑bit conversions are reflected via TypeConverter.
5) Clean error handling; minimal, informative logging gated by verbosity hints.

Native ViaT candidates (decode/encode)
- Grayscale
  - 8‑bit: Array<uint8_t, 2>
  - 16‑bit: Array<uint16_t, 2>
- Truecolor (no alpha)
  - 8‑bit: Array<PixelRGB8, 2>
  - 16‑bit: Array<Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue>, 2> (alias PixelRGB16 if available)
- Truecolor + alpha
  - 8‑bit: Array<PixelRGBA8, 2>
  - 16‑bit: Array<Pixel<uint16_t, ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::Alpha>, 2> (alias PixelRGBA16 if available)
- Gray + alpha (optional but straightforward)
  - 8‑bit: Array<Pixel<uint8_t, ImageChannel::Luminance, ImageChannel::Alpha>, 2>
  - 16‑bit: Array<Pixel<uint16_t, ImageChannel::Luminance, ImageChannel::Alpha>, 2>
- Palette (non‑priority): libpng will expand losslessly to RGB8 or RGBA8 (when tRNS present). We won’t optimize for palette; we will treat it as a path to RGB(A).

Input probing strategy
1) Signature check using ProbeInputContext::m_data (PNG magic: 89 50 4E 47 0D 0A 1A 0A). Decline quickly if no match.
2) Build RAII PngDecodeContext (single‑use): holds FILE*/istream access, png_structp, png_infop, IHDR (width, height, bitDepth, colorType, interlace), flags for PLTE/tRNS/alpha. Read signature/IHDR once and keep context for the plan.
3) Choose ViaT candidates based on IHDR:
   - GRAY: Gray8 or Gray16 depending on bitDepth.
   - GRAY_ALPHA: LA8/LA16.
   - RGB: RGB8/RGB16.
   - RGBA: RGBA8/RGBA16.
   - PALETTE: RGB8 (or RGBA8 if tRNS) via lossless expansion.
   - For bit‑packed gray (1/2/4), expand to 8‑bit (we won’t offer sub‑8‑bit ViaTs).
4) Score candidates against ctx.m_targetType via TypeConverter; pick best by maximizing convLoss × intrinsicLoss.
   - intrinsicLoss = 1.0 for exact mappings and for palette/tRNS expansions (lossless).
   - intrinsicLoss = bitDepth/8.0 for gray 1/2/4 expanded to 8‑bit (e.g., 0.5 for 4‑bit).
5) Build a StreamInputPlan that captures the shared_ptr<PngDecodeContext>, decodes once at pos==0, and returns std::nullopt thereafter.

Decode paths (libpng)
- Error handling with setjmp/longjmp via png_jmpbuf; return std::nullopt on failure; upper layer logs at boundary.
- Source selection:
  - If ctx.mStream is present, install custom read fn via png_set_read_fn to pull from std::istream.
  - Else, open FILE* and use png_init_io.
- Transform configuration (before png_read_update_info) to match chosen ViaT while preserving sample values:
  - Palette → RGB: png_set_palette_to_rgb
  - tRNS → alpha: png_set_tRNS_to_alpha (becomes RGBA)
  - Gray 1/2/4 → 8: png_set_expand_gray_1_2_4_to_8
  - If ViaT is 8‑bit and IHDR is 16‑bit: png_set_strip_16 (avoid if ViaT is 16‑bit)
  - If ViaT is 16‑bit on little‑endian hosts: png_set_swap
  - Gamma/color: no transforms by default (exact sample preservation). A future hint can enable sRGB→linear if desired.
- Allocate destination Array with exact range. Prepare row pointer array aliasing each destination row. Call png_read_image (or png_read_row loop), then png_read_end.
- Interlaced images (Adam7): enable via png_set_interlace_handling and let png_read_image deinterlace into the destination.

Output probing and encode paths
- Consider ViaTs we can write: Gray8/16, RGB8/16, RGBA8/16, LA8/16. Pick the best conversion from ctx.m_sourceType via TypeConverter with highest conversionLoss (intrinsicLoss=1.0 as PNG is lossless at chosen depth).
- Hints (ctx.m_formatHint), defaults in parentheses:
  - pngCompressionLevel: int [0..9] (6)
  - pngFilter: one of {auto, none, sub, up, avg, paeth} (auto)
  - pngInterlace: bool (false)
  - Optional metadata (later): pngSRGB (bool), pngGamma (float). No color transform unless explicitly requested.
- Encode implementation:
  - Create png_structp/png_infop; set IHDR (width, height, bitDepth=8 or 16, colorType, interlace).
  - Apply compression/filter/interlace from hints.
  - For 16‑bit on little‑endian hosts: png_set_swap.
  - Provide row pointers from the Array and call png_write_image (or png_write_row loop); finish with png_write_end.

Preserved‑bits model
- Exact mapping at equal bit depth and channel layout: 1.0.
- Palette→RGB(A) and tRNS→alpha: 1.0 (lossless expansion).
- Gray 1/2/4 → 8: intrinsicLoss = bitDepth/8.0.
- 16→8 reductions should only occur via TypeConverters (not inside this handler by default); chain loss reflects 8/16.

API/Registration design
- Namespace: Ravl2 (anonymous ns for registration, mirroring JPEG).
- Input registration: name "PNGNative" (or "LibPNG"), extensions "png", protocol "file", priority 10 to win over OpenCV.
- Output registration: same name/priority.
- initPngImageIO(): public symbol in PngImageIO.hh to force TU linkage and, if needed, initialize any related conversions (not required per current scope assumptions).

CMake integration
- In src/Ravl2/ImageIO/CMakeLists.txt: find_package(PNG QUIET). If found, link PNG::PNG and define RAVL2_HAVE_PNG=1; otherwise act as a stub so OpenCV can handle PNG.

Testing plan (Catch2)
1) Load tests:
   - Load into Array<PixelRGB8,2>, Array<PixelRGBA8,2>, and Array<uint8_t,2>. Validate dimensions and basic pixel ranges.
   - If repository lacks data/lena.png, first save a synthetic PNG via our saver and then load it to exercise both paths.
2) Save→Load round‑trip:
   - RGB8, RGBA8, Gray8: exact equality expected (lossless, no gamma transform).
   - 16‑bit Gray/RGB(A): round‑trip equality expected when RAVL2_HAVE_PNG.
3) Palette/tRNS (non‑priority): generate a tiny palettized PNG with one transparent index via libpng; load to RGBA8 and validate alpha at a known pixel.
4) Interlaced write/read: enable interlace in hints, round‑trip and validate dimensions (and, optionally, content).

Implementation steps
1) Add files: PngImageIO.hh (init function), PngImageIO.cc (registrations + probe/plan + decode/encode).
2) Wire CMake: find_package(PNG QUIET), link, and define RAVL2_HAVE_PNG.
3) Implement input probe: signature check, create PngDecodeContext, parse IHDR, enumerate ViaTs, score via TypeConverter, compute intrinsicLoss for gray 1/2/4 expansion, build StreamInputPlan capturing context.
4) Implement decode lambda: configure libpng transforms for chosen ViaT; use row pointers to write directly into Array rows; handle interlace and 16‑bit endianness; robust error handling.
5) Implement output probe: evaluate chains from source to Gray8/16, RGB8/16, RGBA8/16, LA8/16; parse hints (compression, filter, interlace); build StreamOutputPlan.
6) Implement encode lambda: set IHDR and options; write rows directly; finalize.
7) Add tests under test/: load, save→load round‑trips (8/16 bit), palette/tRNS, interlace; guard 16‑bit specific tests behind RAVL2_HAVE_PNG.
8) Documentation cross‑refs: ensure ImageIO_Overview.md mentions this plan; Requirements.md lists PNG as supported with direct loaders/savers.

Future extensions (optional)
- Gamma/color management: opt‑in sRGB→linear (and back) via hints with explicit documentation; carry sRGB/gAMA/iCCP metadata through.
- Memory‑source/memory‑sink support via custom read/write callbacks working on ctx.mStream or in‑memory blobs.
- Ancillary chunks: expose read/write for text/time/custom chunks via hints or a metadata channel if needed by applications.
