Jpeg loader design and implementation plan (libjpeg‑turbo)

Overview
This document outlines a concrete plan to implement a high‑quality JPEG loader that decodes directly into Ravl2 array and planar types, integrates with the IO probing/plan mechanism, and cooperates with the type conversion framework to maximize preserved bits (minimize conversion loss). The OpenCV loader is used as a comparative reference, but the JPEG loader will decode straight into Ravl2 containers to avoid intermediate cv::Mat and improve control over color spaces and subsampling.

Relevant infrastructure in Ravl2
- Input probing and planning
  - InputFormatCall and inputFormatMap(): Register a handler with: (name, extensions, source, priority, probe lambda)
  - ProbeInputContext: carries m_filename, m_targetType, m_verbose
  - StreamInputPlan: holds a StreamInputCall<T> or InputStreamContainer<T>, a ConversionChain, and a loss score
  - StreamInputCall<T>: lazy pull interface: callable returning optional<T> per position/read
  - InputStreamContainer<T>: simple container when data is already available
  - ConversionChain and typeConverterMap(): find conversion chain between types, prepend conversion, compute conversionLoss()

- Pixel and image types
  - Array<T, 2>: Packed images such as Array<uint8_t,2>, Array<PixelRGB8,2>, Array<PixelY8,2>, etc.
  - Planar images and pixel planes (PixelPlane.hh): RGBPlanarImage<T>, RGBAPlanarImage<T>, YUV444Image<T>, YUV422Image<T>, YUV420Image<T>
  - Pixel channels enumeration: ImageChannel::{Red, Green, Blue, Alpha, Luminance, ChrominanceU, ChrominanceV, …}
  - convertToPlanar/convertToPacked helpers for interop if needed.

Goals for JPEG loader
1) Decode via libjpeg‑turbo directly into Ravl2 containers without intermediate conversions.
2) Use probing to choose the best native decode target based on the requested final type, maximizing conversion chain preserved bits.
3) Support grayscale and color JPEGs, including subsampling 4:4:4, 4:2:2, 4:2:0, 4:1:1.
4) Support packed RGB8 output and YUV planar output families (YUV444/422/420) as primary decode targets.
5) Report conversion loss accurately and prefer plans with the highest score.
6) Clean error handling and minimal logging at the appropriate boundary.

Probing strategy
Given ProbeInputContext ctx with ctx.m_targetType:
1) Header read only (no full decode): Use jpeg_read_header() to learn:
   - colorspace (JCS_GRAYSCALE, JCS_YCbCr, JCS_RGB, etc.)
   - components (1 or 3)
   - subsampling factors per component (h_samp, v_samp); deduce 4:4:4/4:2:2/4:2:0
   - bit depth (baseline is 8; libjpeg‑turbo also supports 12‑bit with proper build, but often 8)
2) Construct candidate “via” Ravl2 output types that we can decode to natively:
   - If grayscale: Array<uint8_t,2> and Array<PixelY8,2>
   - If color (YCbCr): preferred native candidates
     - YUV444Image<uint8_t> if sampling is 4:4:4
     - YUV422Image<uint8_t> if 4:2:2
     - YUV420Image<uint8_t> if 4:2:0 or 4:1:1 (map appropriately)
     - As a fallback, packed Array<PixelRGB8,2>
   - If encoder colorspace is RGB (rare in JPEG files): Array<PixelRGB8,2>
3) For each candidate native type ViaT, query typeConverterMap().find(ctx.m_targetType, typeid(ViaT)). If a chain exists, compute total loss. Choose the candidate with best (highest) preserved‑bits score. If multiple tie, favor planar YUV matching the file’s native subsampling over packed.
4) Build a StreamInputPlan where the stream callable performs the actual libjpeg decode into the chosen ViaT and returns it at pos==0, and std::nullopt afterwards (still image). Prepend a final conversion to ctx.m_targetType using ConversionChain::prepend if needed.

Decode paths (libjpeg‑turbo)
- Grayscale
  - Set out_color_space = JCS_GRAYSCALE, output_components = 1
  - Allocate Array<uint8_t,2> or Array<PixelY8,2> with width×height
  - Read scanlines into row buffers that alias Array memory or copy in place, depending on stride alignment

- YCbCr with subsampling
  Two approaches are common:
  1) Use libjpeg’s YCbCr to RGB expansion (JCS_EXT_RGB) to packed RGB8. This is simpler but loses exact chroma sampling structure.
  2) Extract raw iMCU blocks to reconstruct planar Y, Cb, Cr matching subsampling. This yields perfect mapping to YUV444/422/420 planes. libjpeg‑turbo provides access to raw data via jpeg_read_raw_data when supported by the input.

  Plan:
  - Prefer raw planar decode using jpeg_read_raw_data when subsampled. Map to:
    - 4:4:4 -> YUV444Image<uint8_t>
    - 4:2:2 -> YUV422Image<uint8_t>
    - 4:2:0 / 4:1:1 -> YUV420Image<uint8_t> (note: 4:1:1 behaves like 420 with only horizontal factor 4; we can downsample to 420 plane layout after extracting Cb/Cr blocks, or introduce a YUV411 type later)
  - If raw data path isn’t available, fall back to JCS_EXT_RGB packed decode into Array<PixelRGB8,2>.

Bit depth / HDR considerations
- Baseline JPEG is 8‑bit. If 12‑bit JPEG is enabled in libjpeg‑turbo build, we can optionally support Array<uint16_t,2> / YUV*Image<uint16_t>. For first iteration, implement 8‑bit only and document extension point.

Conversion loss model
- Our candidate native outputs should be scored as follows (guideline):
  - Exact colorspace + subsampling preservation: 1.0
  - Exact colorspace but subsampling change (e.g., 420 -> 444 by upsampling inside decoder): 0.98
  - YCbCr -> RGB packed via libjpeg color convert: 0.96 (loss of full chroma resolution if source was subsampled; also quantization of conversion)
  - Grayscale exact mapping to Y8: 1.0
  - For any component width reduction (e.g., 12‑bit -> 8‑bit), reduce proportionally, e.g. preservedBits = 8/12.
- Implement these values by controlling the ConversionChain used:
  - When we select a ViaT to decode, we consult typeConverterMap for ViaT -> target. The chain returns its own loss; the JPEG loader should only add its own intrinsic loss for any implicit step it performs (e.g., internal upsample to 444 before building a planar image). If we are decoding raw planes matching the source subsampling, use 1.0.

API/Registration design
- Namespace: Ravl2::ImageIO::JPEGTurbo (or simply inside Ravl2 anonymous ns collocated with registration, mirroring OpenCV loader style).
- Register in inputFormatMap with name "JPEGTurbo", extensions "jpg,jpeg", source "file", priority slightly above OpenCV generic (e.g., 10) so it wins for JPEGs.
- Probe lambda outline:
  - Open file, init jpeg_decompress_struct and jpeg_error_mgr
  - jpeg_read_header()
  - Determine native candidate types and pick best ViaT by consulting typeConverterMap against ctx.m_targetType
  - If ctx.m_verbose, log chosen plan and estimated loss
  - Build StreamInputPlan with
    - stream: StreamInputCall<ViaT> that decodes at pos==0 and returns std::nullopt afterwards
    - chain: convChain (if target != ViaT) possibly with prepend(makeTypeConversion<ViaT, ViaT>(identity)) when needed by API
    - loss: convChain.conversionLoss() multiplied by our intrinsic path loss

Decoding implementation details
- Memory layout
  - Allocate Ravl2 arrays with exact dimensions. For planar images, planes sized according to subsampling via PixelPlane helpers (construct with masterRange and scaling=true).
  - When using jpeg_read_scanlines (RGB or gray), set out_color_space accordingly and copy rows into Array backing store.
  - When using jpeg_read_raw_data, process per iMCU: Y plane is full resolution, Cb/Cr strides determined by subsampling. Use PixelPlane::atMaster or direct plane.data() indices derived from PlaneScale to write blocks efficiently.
- Alpha channel
  - JPEG has no alpha; if target expects RGBA, rely on type converters to add opaque alpha. Avoid duplicating logic in loader.

Logging and errors
- Use SPDLOG_INFO for major state transitions when verbose: selected decode path, colorspace, subsampling.
- Use SPDLOG_WARN for recoverable anomalies (e.g., fallback to RGB path because raw decode unsupported).
- Use exceptions for logical/corrupt file errors during decode; don’t double‑log at multiple layers.

CMake integration
- Add optional JPEGTurbo support behind option(ENABLE_JPEGTURBO ON)
- Find library (prefer package config or FindJPEGTurbo), link to a small library target ravl2_imageio_jpegturbo that exposes only internal code; register static init via TU with function initJpegTurboImageIO() called by higher‑level init.
  - Alternatively, mirror OpenCV loader pattern and ensure registration happens via TU static initialization.

Testing plan (Catch2)
1) Unit tests exercising probing and planning:
   - Given target types: Array<PixelRGB8,2>, RGBPlanarImage<uint8_t>, YUV420Image<uint8_t>, Array<PixelY8,2>
   - Ensure the chosen ViaT matches expectation for grayscale and color JPEG files (use small sample images in test data)
   - Verify conversionLoss ordering prefers planar YUV when target is planar and next‑best when not available
2) Decode correctness:
   - For known synthetic images, validate plane sizes and a small set of pixel values
3) Edge cases:
   - Progressive JPEGs (libjpeg‑turbo supports; treat similarly)
   - CMYK JPEGs: for first iteration, fall back to RGB conversion with documented lower score; future support may add CMYK path
4) Performance sanity: ensure decode completes within reasonable time for small images (no strict perf test in unit by default).

Implementation steps
1) Add new files under Ravl2/src/Ravl2/ImageIO:
   - JpegTurboImageIO.cc: registration + probing + plan builder + decoding stream callables
   - JpegTurboImageIO.hh: declare initJpegTurboImageIO()
2) Wire into CMake with an option and link against libjpeg‑turbo
3) Implement probing logic selecting ViaT and computing intrinsic loss
4) Implement grayscale and RGB packed decode paths
5) Implement raw planar YUV decode path using jpeg_read_raw_data for 4:2:0 and 4:2:2, and 4:4:4 straightforward
6) Add logging at INFO/WARN per guidelines, guarded by ctx.m_verbose when applicable
7) Add Catch2 tests under test/imageio_jpeg_tests.cc

Notes on differences vs OpenCV loader
- OpenCV loader has to guess color spaces and then look for conversions. The JPEG loader will know the precise JPEG header info and can select an exact ViaT (e.g., YUV420Image<uint8_t>), which increases preserved‑bits and avoids unnecessary conversions.
- Using planar types ties in naturally with PixelPlane’s scaling helpers, preserving subsampling structure for subsequent processing stages.

Future extensions
- 12‑bit JPEG: map to uint16_t planes/arrays with preservedBits=12/16 when converting to 16‑bit types, or 12/12 if converting to 12‑bit container later.
- CMYK and AdobeApp14 color transforms: add dedicated CMYK path and convert with minimal loss when target supports it.
- Hardware accelerated decode (TurboJPEG API) as an alternative implementation for improved throughput.
