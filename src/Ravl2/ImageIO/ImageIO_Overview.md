ImageIO in Ravl2: Authoring loaders and savers

Overview
This document explains how Image IO works in Ravl2 and provides a practical checklist for implementing new image loaders (readers) and savers (writers). It builds on the IO infrastructure (InputFormat/OutputFormat, TypeConverter, StreamInput/StreamOutput) and the pixel/image containers (Array, Pixel, PixelPlane, PlanarImage).

Key concepts
- Input probing and planning
  - InputFormat: a handler that can recognise a source and return a StreamInputPlan describing how to read it.
  - ProbeInputContext: immutable context with url, filename, protocol, extension, target C++ type, look‑ahead bytes (ctx.m_data), and JSON hints.
  - StreamInputPlan: contains a StreamInput<T> (or container) that yields the decoded type ViaT once (for still images) and a ConversionChain from ViaT to the requested target type; also carries an overall preserved‑bits score (loss).

- Output probing and planning
  - OutputFormat: a handler that can write a given source type to a destination URL.
  - ProbeOutputContext: immutable context with url, filename, protocol, extension, source C++ type and JSON hints.
  - StreamOutputPlan: contains a StreamOutput<ViaT> that consumes ViaT and writes it, plus a ConversionChain from the user’s source type to ViaT; carries a preserved‑bits score.

- Type conversion and loss accounting
  - TypeConverter: registry of conversion functions between C++ types.
  - ConversionChain: a chain of registered conversions with an aggregate conversionLoss() score in [0, 1]. Higher is better (fewer bits lost).
  - Image IO handlers should select a native ViaT that maximises the final preserved‑bits score when combined with the chain from/to the user’s type.

Design principles for Image IO handlers
- Choose native ViaT wisely
  - Prefer decoding/encoding to a type that preserves the file’s native color space and subsampling (e.g., planar YUV for JPEG with 4:2:0), then use TypeConverter to reach the target.
  - If the file format doesn’t support alpha, don’t invent it; rely on converters to add an opaque alpha if required by the target.

- Probe cheaply, decode/encode lazily
  - Probe should be fast and avoid heavy work. Use header parsing and look‑ahead bytes (ctx.m_data) to recognise formats.
  - Delay real IO until the returned StreamInput/StreamOutput is invoked.

- Keep the ProbeInput/OutputContext immutable
  - Do not mutate ctx. If you must carry state (open file handle, decompressor state), capture it in the Stream plan via a small RAII context owned by a shared_ptr and consumed exactly once.

- Logging and errors
  - Follow repository guidelines: INFO for state transitions when ctx.m_verbose is true, WARN for recoverable anomalies, ERROR only where action is taken (e.g., failing to read/write). Avoid double‑logging the same error from multiple layers.

- Threading
  - Plans should be safe to use on a single thread by default. If you use background threads, document thread‑safety, ownership, and cancellation.

Minimal loader structure (pattern)
1) Registration
   - Register your handler in inputFormatMap() with:
     - name (e.g., "PNGNative")
     - extensions (e.g., "png")
     - protocol (e.g., "file")
     - priority (integer; higher values probed first for the same extension)

2) Probe lambda
   - Validate protocol/extension quickly.
   - Optionally check magic bytes in ctx.m_data to reject early.
   - Open the file and parse minimal header information to identify color model, component depth, subsampling, etc.
   - Construct one or more candidate native ViaT types you can decode to.
   - For each candidate, ask typeConverterMap().find(ctx.m_targetType, typeid(ViaT)). The best plan maximises convChain.conversionLoss() × intrinsicLoss, where intrinsicLoss models any loss inside your handler (e.g., RGB expansion from subsampled YUV).
   - Build and return StreamInputPlan:
     - mStream: a StreamInputCall<ViaT> that returns the decoded image at pos==0 and std::nullopt thereafter.
     - mConversion: the found ConversionChain (possibly empty when ViaT matches the target type).
     - mLoss: the aggregate score.

3) Decode lambda
   - Perform the actual format decode into ViaT’s backing memory.
   - Avoid per‑pixel copying if the container layout allows direct row/plane writes.
   - On failure, return std::nullopt and let the caller handle logging at the boundary.

Minimal saver structure (pattern)
1) Registration
   - Register your handler in outputFormatMap() with name, extensions, protocol, and priority.

2) Probe lambda
   - Prepare candidate ViaT types you can encode (e.g., Array<PixelRGB8,2> and Array<uint8_t,2> for JPEG).
   - For each ViaT, ask typeConverterMap().find(typeid(ViaT), ctx.m_sourceType). Choose the chain with the highest conversionLoss().
   - Build and return StreamOutputPlan with:
     - mStream: a StreamOutputCall<ViaT> that writes the image (pos==0 for stills; return new position).
     - mConversion: the chain from the user type to ViaT.
     - mLoss: the aggregate score.

3) Encode lambda
   - Configure encoder from ctx.m_formatHint (e.g., quality for JPEG/PNG compression level). Validate and clamp user‑provided values.
   - Write scanlines or planes directly from ViaT without per‑pixel loops when possible.

Choosing priorities
- Handlers for the same (protocol, extension) are probed in decreasing priority (implementation detail: current maps iterate insertion order per extension; give a higher priority number to preferred native handlers). Use a priority above generic fallback handlers (like the OpenCV loader) to ensure your native path is selected when available.

Example: JPEG (libjpeg‑turbo) loader/saver anatomy
- Files: see JpegTurboImageIO.cc and JpegLoaderPlan.md in this directory.
- Probe
  - Check magic (FFD8) from ctx.m_data if present.
  - Open file, read header once, and create a JpegDecodeContext with FILE* and jpeg_decompress_struct.
  - Identify grayscale vs color, and (optionally) subsampling.
  - Score ViaT candidates (Array<uint8_t,2> for gray; Array<PixelRGB8,2> or YUV planar types for color) using the TypeConverter map.
- Decode
  - Configure out_color_space based on ViaT.
  - Decode scanlines directly into Array row buffers (no per‑pixel copy).
- Save
  - Choose between RGB8 and Gray8 ViaTs based on best conversion score.
  - Use jpeg_set_quality from hints; write scanlines directly.

Planar images and subsampling
- Prefer planar types when the file format natively stores planes (e.g., YUV420/422/444 for JPEG, various modes for codecs like HEIF/AVIF).
- Ravl2 provides:
  - PixelPlane with compile‑time channel tags and per‑dimension scaling factors.
  - PlanarImage aggregating planes and helpers to convert to/from packed pixel arrays.
- Writing planar‑aware loaders preserves more information for downstream processing and can improve the final preserved‑bits score.

Testing checklist (Catch2)
- Provide a simple load test that:
  - Probes and loads a known test image into a common target (e.g., Array<PixelRGB8,2>), checks dimensions and basic pixel sanity.
- Provide a save→load round‑trip test for typical ViaTs (RGB8, Gray8) and verify dimensions and basic pixel properties.
- Keep tests tolerant of optional dependencies by falling back to other handlers (e.g., OpenCV) when a native library is absent.

CMake integration tips
- Create a small library target under src/Ravl2/ImageIO and link against Ravl2Core.
- Gate optional format libraries (libjpeg, libpng, etc.) behind find_package(...) and add compile definitions (e.g., RAVL2_HAVE_JPEG) to keep builds working without those deps.
- Expose a tiny header (e.g., JpegTurboImageIO.hh) with init functions that force TU linkage when a user explicitly calls it from tests or an app.

Coding conventions
- Follow the project’s general guidelines (see repository guidelines):
  - Use spdlog for logging and guard INFO output by ctx.m_verbose.
  - Prefer std::expected in new APIs that return error info (where applicable); within IO callbacks it’s common to return std::optional and log at the boundary.
  - Use Eigen for math as needed, std::chrono for time, and nlohmann::json for hints.
  - Keep implementation details inside .cc files; headers are for small inline helpers or templates only.

Authoring checklist
1) Decide on target ViaTs you’ll support natively (packed and/or planar).
2) Add a new handler registration in inputFormatMap()/outputFormatMap() with appropriate priority.
3) Implement probing:
   - Quick signature validation using ctx.m_data when possible.
   - Parse minimal headers to determine color space, bit depth, subsampling.
   - Score and choose the best ViaT using the TypeConverter map.
4) Implement lazy decode/encode stream callbacks that operate directly on the destination container.
5) Account for intrinsic loss (if any) and multiply by the conversion chain loss.
6) Add unit tests (load and save) and wire them into test/CMakeLists.txt.
7) Document any format‑specific hints (e.g., quality), default values, and ranges.

See also
- Requirements.md in this directory for high‑level goals and supported formats.
- JpegLoaderPlan.md for a worked design document and rationale.
- IO/InputFormat.hh and IO/OutputFormat.hh for reference documentation of the probing APIs.
