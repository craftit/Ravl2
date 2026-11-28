# Ravl2 review findings (2026-11-28)

Library-wide review for bugs, gaps and improvements. Line numbers are as of commit 8808ebf.
Tick items off (`[x]`) as they are fixed, noting the commit. Items marked *(verified)* were
reproduced with a scratch program (ASan/UBSan/valgrind) during the review; the rest are from
reading the code.

Work order: Batch 1 → 4, then the remaining bugs by area, then gaps/improvements.

Fixes that change APIs, behaviour callers may rely on, or saved data are listed in `CHANGELOG.md` (under Changed and Data compatibility).

---

## Batch 1 — silent failures in IO, Video and image files

- [x] **IO-1** `IO/StreamInput.hh:27,73,103,139,155`, `StreamOutput.hh:28,76,159`, `VideoIO.cc:264`, `Cereal.hh` — `std::numeric_limits<std::streampos>::max()` is 0 (not specialised), so the default end offset is 0, `begin()==end()` and range-for over a video/file stream runs 0 times. *(verified)*
- [x] **IO-2** `IO/Cereal.hh:223-251` — writing >1 object to a binary cereal stream overwrites the header (seekp to object index, caused by IO-1); line 243 tests `BinaryInputArchive` in the output class so `mEnd` counts objects not bytes; input `get()` re-reads object 0. *(verified)*
- [x] **IO-3** `IO/TypeConverter.cc:38` — `findInternal` pops the *lowest*-quality chain from the multimap (higher is better), so the lossiest conversion route wins. *(verified)*
- [x] **VID-1** `Video/VideoIO.cc:61-81` + `FfmpegMultiStreamIterator.cc:424-428` — at EOS `next()` keeps `mCurrentFrame`; the RavlIO reader returns the last frame forever. *(verified)*
- [x] **IMG-IO-1** `ImageIO/JpegTurboImageIO.cc:74,488,535` — libjpeg default `error_exit` calls `exit()`; a corrupt JPEG terminates the process. *(verified)*
- [x] **IMG-IO-2** `ImageIO/PngImageIO.cc:118-194` — row bytes never checked against destination pixel size; grey+alpha → `uint8_t` corrupts the heap; `png_get_color_type` read before `png_read_update_info` so strip_alpha skipped for RGB. *(verified)*
- [x] **IMG-IO-3** `ImageIO/PngImageIO.cc:127,194` — Gray16 lacks `png_set_swap` (byte-swapped on little-endian); `png_read_image` called once per interlace pass so Adam7 PNGs fail. *(verified)*
- [x] **IMG-IO-4** `ImageIO/PngImageIO.cc:389+`, `JpegTurboImageIO.cc:509,557` — encoders index `img[{y,0}]`; non-zero-origin images assert / read out of bounds. *(verified)*
- [x] **IMG-IO-5** `ImageIO/JpegTurboImageIO.cc:232` — planar YUV decode writes chroma rows at `r` instead of `yPosChroma+r`.

- [x] **IMG-IO-6** *(found while fixing)* `ImageIO/PngImageIO.cc` — the writer called `png_set_swap()` before `png_write_info()`, where it has no effect, so 16-bit PNGs were written little-endian (the reader's missing swap hid it in round trips). **Data impact:** 16-bit PNGs saved since 9b41d17 (2025-11-19) are byte-swapped and now read back swapped; see "Compatibility notes" in `ImageIO/ImageIO_Overview.md` for how to repair them.
- [x] **PIX-2** *(found while fixing)* `Pixel/PixelPlane.hh:756` — `YUV422Image` chroma planes were scaled (2,1), halving rows; 4:2:2 halves columns. Broke FFmpeg yuv422p frames, the JPEG 4:2:2 path and contradicted `convertYUYV2Planar422`. Test rewritten in (row, col) order.

## Batch 2 — core array iteration and ranges

- [x] **CORE-1** `Array1.hh:545-566` — 1-D `begin()/end()` start at `m_data` (index 0) not `range.min()`; non-zero-origin 1-D arrays iterate wrong memory. *(verified)*
- [x] **CORE-2** Empty ranges: `Array.hh:250,303-310` 0-column 2-D array loops forever; `IndexRange1.hh:461-470` `end()` is `max+1` compared with `!=` so `{6,3}` runs to overflow; `IndexRange.hh:609-612` `valid()` checks only dim 0; `Array.hh:244` `begin()` on default array asserts. *(verified)*
- [x] **CORE-3** `Array.hh:877-886` — `copy(dest, src)` with dest larger than src overruns src; same in `Pixel/Colour.hh:207-219` `convert()`; `zip()` checks no sizes. *(verified)* — fixed copy/convert; `zip()` size checks still to do (IMP-1).
- [x] **CORE-4** `Array.hh:377-380`, `ArrayIterZip.hh:94-97` — `window()` stores a pointer to a temporary range; `ScanWindow.hh:104,112` keeps a pointer to its own member (breaks on copy). *(verified for first two)*
- [x] **CORE-5** Iterator distance: `Array.hh:431`, `IndexRange.hh:631-633` use `range(0).size()` not the row width; `Array1.hh:195-198` `operator-(it,n)` adds n. *(verified)*
- [x] **CORE-6** `IndexRange.hh:121-148` — `elements()`/`area()` multiply signed sizes (two negative dims → 1); int overflow above 2^31 elements; strides/offsets are int. *(verified sign issue)* — sign issue and `elements()` overflow fixed; strides/`area()` widened in IMP-4.
- [x] **CORE-7** `Array.hh:127,134` — `ArrayAccess::operator()(i,j)` returns by value; writes lost. *(verified)*
- [x] **CORE-8** `ScanWindow.hh:82` — `ScanWindow<T,1>::index()` inverted.
- [x] **CORE-9** `ThreadedQueue.hh:112-114,130-139` — `wait()`/`popWait()` have no predicate loop; spurious wakeup / competing consumer pops empty queue (UB).
- [x] **CORE-10** `Array.hh:79-83,116-121,604-608,616-620` — debug bounds checks skip the last dimension.

Batch 2 note: `IndexRange<1>::size()` now returns 0 for empty ranges (was negative), and iterators over empty
ranges start at the end. 1-D `copy()` still copies a prefix to `dest`'s min rather than matching indices.

## Batch 3 — interop and Display crashes

- [x] **CV-1** `OpenCV/Image.hh:21`, `Image.cc` BGR spec., `ImageIO.cc:301` — `toCvMat` passes no step; clipped views read with wrong pitch. *(verified)*
- [x] **CV-2** `OpenCV/Image.cc:61` — `toArray<PixelBGRA8>` divides step by `sizeof(PixelBGR8)`. *(verified)*
- [x] **CV-3** `OpenCV/ImageIO.cc:290-297` — Array→cv::Mat conversions return non-owning Mats of by-value temporaries; multi-step save chains read freed memory.
- [x] **DSP-1** `Display/Image2DNodeBase.cc:17`, `DebugDisplay.cc:295,975` — static `g_channels` destroys bgfx textures after `bgfx::shutdown()`; crash at exit.
- [x] **DSP-2** `Display/DebugDisplay.cc:853,877` — GUI thread uses blocking `g_queue.push` on the queue only it drains; deadlock under streaming.
- [x] **DSP-3** `Display/DebugDisplay.cc:843-844` — per-frame scroll delta passed where bgfx imgui expects cumulative; ImGui/ImPlot/3D scroll cancels out.
- [x] **DSP-4** `Display/IOFormatAdapter.cc:432` — `:Clear` substring match also matches `:ClearOverlays` etc.
- [x] **DSP-5** `Display/IOFormatAdapter.cc:478,516,559,590` — mode values compared case-sensitively (`replace`, `ringbuffer` ignored).
- [x] **DSP-6** `Display/IOFormatAdapter.cc:330-333,359` — `Norm=Pct:1,99` arguments truncated at `:`; percentile unsupported in bgfx upload (`Image2DNode.cc:166`).
- [x] **DSP-7** `Display/SetNormalization2D.cc:16`, `SetBaseImage2D.hh:33` — only top-level node inspected; overlays (CompositeNode) break normalisation and get wiped on image update.
- [x] **DSP-8** `Display/AddPolylineOverlay2D.cc:19`, `Polyline2DNode.cc` — polylines treated as (x,y), RAVL2 points are (row,col). *(convention decision)*
- [x] **DSP-9** `Display/DebugDisplay.cc:1075` — macOS `runMainLoop` spins forever if the app never displays.
- [x] **DSP-10** `Display/DebugDisplay.cc:670` — SDL fallback uses `RGBA8888` for byte-order RGBA (should be `ABGR8888`/`RGBA32`).
- [x] **QT-1** `Qt/Image.hh:69,99,121,157` — width/height swapped both directions. *(verified)*
- [x] **QT-2** `Qt/Image.hh:60,64,73,80,149` — `if constexpr(cond, "msg")` always true; `toArrayView` doesn't compile; Grayscale8 asserted for all types; copy loop drops last row. *(verified)*
- [x] **DLIB-1** `Dlib/Image.hh:75` — `set_image_size` makes (rows-1)x(cols-1); dlib overruns.
- [x] **DLIB-2** `Dlib/Image.cc:19`, `Dlib/Image.hh:179` — RGB conversion and `operator[]` ignore array origin.

Batch 3 notes: DSP-1, DSP-2, DSP-9 and DSP-10 change GUI-thread code that needs a display to run; they were
checked by reading and build, not at runtime. The Dlib fixes were compiled and run against a dlib checkout
(dlib isn't a dependency of the test build). DSP-8: overlay points are now (row, col), matching the rest of RAVL2.
Also fixed: the default overlay colour was yellow, not the magenta documented.

## Batch 4 — maths and geometry correctness

- [x] **MATH-1** `Math/MeanVariance.hh:154,191` — mean-difference term not squared in merge/remove; test at `test_math.cc:87` (`i % 1`) never exercises it. *(verified)*
- [x] **MATH-2** `Math/Sums1d2.hh:95` — `operator-=` adds counts. *(verified)*
- [x] **MATH-3** `Math/PrimeFactors.cc:91-108,65-66` — n=0/1 never terminates (FFT of size 1 hangs); `kpf[310]` read out of bounds. *(verified)*
- [x] **PIX-1** `Pixel/Pixel.hh:142` — unsigned chroma offset 127/32767 not 128/32768. *(verified)*
- [x] **GEO-1** `Geometry/Range.hh:121-130` — `mostEmpty()`/`largest()` use `min()` not `lowest()`.
- [x] **GEO-2** `Geometry/Circle.hh:37,107` — 3-point ctor discards fit; `Angle()` wrong (NaN). *(verified)*
- [x] **GEO-3** `Geometry/Line2ABC.hh:185-200` — `intersects()` boundary lines have wrong sign. *(verified)*
- [x] **GEO-4** `Geometry/Projection.hh:254` — `atInfinity()` uses translation column; `projectedBounds()` returns `largest()` for affine warps. *(verified)*
- [x] **GEO-5** `Geometry/Affine.hh:93-97` — `operator/` wrong composition; unchecked optional deref. *(verified)*
- [x] **GEO-6** `Geometry/FitSimilarity.hh:82,150` — `fitIsometry` still applies computed scale.
- [x] **GEO-7** `Geometry/Polygon.cc:271-303,114-130` — `isSelfIntersecting()` tests against infinite lines (and UB for size<3); `contains()` wrong along collinear edges. *(verified)*
- [x] **GEO-8** Lower impact *(verified)*: `Projection.hh:204` `isNearAffine()` always false; `LinePP.hh:192` `ParClosest` 2-D only; `Geometry.hh:171` `sumOfSqr(scalar)` returns abs; `Geometry.hh:102` `angle()` unclamped acos; `Quaternion.hh:205` `angle()` half-angle; `fromAngleAxis` normalises 4-vector; `Ellipse.hh:66` unchecked optional; `PinholeCameraArray.hh:91` rejects perpendicular rays; `Affine.hh:179` scale axes swapped (check intent).
- [x] **3D-1** `3D/TriMesh.hh:173,201,225,301,311,436` — `for(auto it : …)` copies; Flip/normalise/load do nothing. *(verified)*
- [x] **3D-2** `3D/TriMesh.hh:347-349` — `operator+` cross-array pointer subtraction. *(verified)*
- [x] **3D-3** `3D/TriMesh.hh:32` — copy assignment / move not defined; faces dangle. *(verified)*
- [x] **3D-4** `3D/TriMesh.hh:192` — assert on empty `faces`; no index bounds checks.
- [x] **3D-5** `3D/PinholeCamera1.hh:82,102,135-140`, `PinholeCamera2.hh:96` — k1 inverse skipped for k1≤0; Jacobian dr/dX wrong; `projectCheck` uses isNearZero not nearPlane.

Batch 4 notes: GEO-8 partly done — fixed `sumOfSqr(scalar)`, `angle()` clamping, `ParClosest` in N-D,
`fromAngleAxis` axis normalisation and `Ellipse(Vector6)`. Also fixed since: `Quaternion::angle()` returns the full angle, and
`affineFromScaleAngleTranslation` applies scale[i] to axis i. `isNearAffine()` also had a double default argument that didn't compile for float.
`test_geometry.cc` FitSimilarity/FitIsometry now use fixed seeds (part of GAP-12).

## Remaining bugs by area

### Image
- [ ] **IMG-1** `Image/Segmentation/SegmentExtrema.cc:54,116` — `mRegionMap` sized area/2; checkerboard/1x1 overflow; negative levels index `mHist`. *(verified)*
- [ ] **IMG-2** `Image/DrawFrame.hh:111+` — rect overload swaps row/col; breaks `extendImageFill` and `FloodRegionC::GrowRegion` with padding. *(verified)*
- [ ] **IMG-3** `Image/WarpScale.hh:139,251` — output footprint runs past image edge. *(verified)*
- [ ] **IMG-4** `Image/WarpScale.hh:276` — index compared with size; non-zero origins. *(verified)*
- [ ] **IMG-5** `Image/Warp.hh:241` — Clamp/Wrap/Mirror slow path never samples.
- [ ] **IMG-6** `Image/PeakDetector.hh:190,244` — sub-pixel correction row/col swapped; `test_image.cc:263` asserts the bug. *(verified)*
- [ ] **IMG-7** `Image/SummedAreaTable2.hh:94`, `MatchNormalisedCorrelation.cc:19,69` — `Sum` ignores decremented min; NCC ignores search area, unreachable threshold, int overflow. *(verified)*
- [ ] **IMG-8** `Image/EdgeSobel.hh:34,51` — kernel uses `[c-1]` twice. *(verified)*
- [ ] **IMG-9** `Image/Segmentation/Segmentation.cc:33,84,191` — adjacency/boundary length miss first window per row; `MergeComponents` iter>1 size mismatch. *(verified)*
- [ ] **IMG-10** `Image/Segmentation/ConnectedComponents.hh:246` — returns provisional label count; 1xN asserts; test masks it. *(verified)*
- [ ] **IMG-11** `Image/SummedAreaTable.hh:155` — `verticalDifference2` sign flipped. *(verified)*
- [ ] **IMG-12** `Image/Segmentation/Boundary.hh:225` — `toPolygon` adds half-pixel offset twice to first vertex. *(verified)*
- [ ] **IMG-13** `Image/CornerDetectorSusan.cc:304` — dVert/dHor swapped.
- [ ] **IMG-14** `Image/DCT2d.cc:93` — absolute indices in cosine terms; non-zero origins wrong.

### Video / GoPro
- [ ] **VID-2** `FfmpegMultiStreamIterator.cc:507-516,537-549,741` — single-stream seek no-ops yet reports success; "Precise" doesn't decode forward. *(verified)*
- [ ] **VID-3** `FfmpegMediaWriter.cc:828,848` — channel count checked only on first chunk; mono after stereo init over-reads. *(verified, valgrind)*
- [ ] **VID-4** `FfmpegMediaWriter.cc:370-375` — packets freed but left in `pendingPackets` on failure; double free.
- [ ] **VID-5** `FfmpegMultiStreamIterator.cc:845-853` — `getFrameById` temp iterator mutates shared demuxer/decoders.
- [ ] **VID-6** `FfmpegMediaWriter.cc:389,515,530` — failed init leaks codec context; audio retry adds a second stream. *(verified, valgrind)*
- [ ] **VID-7** `FfmpegMultiStreamIterator.cc:123` — active-iterator count leaks when the constructor throws.
- [ ] **VID-8** `FfmpegMultiStreamIterator.cc:1474-1500` — macOS cloned frame leaked every frame.
- [ ] **VID-9** `MediaContainer.cc:24-28`, `FfmpegMultiStreamIterator.cc:1089-1108` — audio type registration/dataType mismatch; typed audio iteration throws.
- [ ] **VID-10** `FfmpegMediaWriter.cc:200,773` — sws_scale reads past unpadded buffer. *(verified, valgrind)*
- [ ] **GOPRO-1** `GoPro/GpmfParser.cc:151,725-756` — GPS5 uses one SCAL for all fields; SCAL read as uint32 for int16 data.

### Display / OpenGL
- [ ] **GL-1** `OpenGL/GLWindow.cc:132,252`, `GLWindow.hh:37-50` — use after `glfwTerminate`; move ops leave stale user pointer and leak.
- [ ] **GL-2** `OpenGL/View3D.cc:564-572` — rotation pivot sign, null scene, missing lock; `DTriMesh.cc:125` `GL_LINE` not a primitive.

### Optimise
- [ ] **OPT-1** `Optimise/GaussianProcess.hh:55`, `BayesianOptimise.cc:166` — `adaptiveLengthScale` crashes (K_inv cleared before predict). *(verified)*
- [ ] **OPT-2** `GaussianProcess.cc:67`, `BayesianOptimise.cc:202` — BO with defaults stalls at first point (no normalisation, 1 initial point, early stop). *(verified)*
- [ ] **OPT-3** `SampleGenerators.cc:102` — Grid ignores `mPointsPerDim`. *(verified)*
- [ ] **OPT-4** `BayesianOptimise.cc:475,493` — length-scale factors compound.
- [ ] **OPT-5** `OptimisePowell.cc:168` — `fP` overwritten before direction-replacement test.
- [ ] **OPT-6** `BayesianOptimise.cc:94` — explicit all-zero start ignored. *(verified)*

### Core / Config
- [ ] **CFG-1** `Configuration.hh:66,85,104`, `Configuration.cc:56-68` — noexcept register functions can terminate on duplicate; empty "default" factory → bad_function_call.
- [ ] **CFG-2** `Configuration.cc:349` — `setValue` locks type before assigning.

### Build
- [ ] **BLD-1** `CMakeLists.txt`, `src/Ravl2/CMakeLists.txt:7-8,28,38-43` — `RAVL2_MIN_DEPS` doesn't disable Qt/dlib; Display added without bgfx.

---

## Gaps

- [ ] **GAP-1** Region growing stack (`RegionGrow*.hh`, `RegionSet.hh`, `SegmentRegion.hh`, `PixelClassify*`, `PixelSelectStack`, `BlueScreen`, `ChromaThreshold`) is unported RAVL1 code, not built.
- [ ] **GAP-2** Templates that don't compile when instantiated: `forEachIndexFirst`, `fft2`/`fftgr` for double, `Types.hh:87` `span(const Vector&)`, `Sums1d2 operator<<`, `ArrayView<T,1>(const vector&)`, `PixelY8(uint8_t)`, `PinholeCameraArray::Triangulate`, `ImagePyramid::findLevel`, OpenCV `toAffine`/`toCvAffine`, several geometry `operator<<`/`>>`, SegmentExtrema ROI overloads.
- [ ] **GAP-3** Build: install/export of library targets is a stub; `BUILD_SHARED_LIBS` not viable (no export macros, shared `Ravl2Core_export.hpp`); static link drops PSO/Bayes/RandomSearch registration.
- [ ] **GAP-4** Video: no sws_scale fallback for unsupported pixel formats (ProRes, gbrp, gray16, …); no `YUV420Image<uint16_t>` plan; grey video unreadable; `previous()` unimplemented; RavlIO video ignores `pos`; no audio/Frame via RavlIO; writer assumes sample rate.
- [ ] **GAP-5** OpenCV IO: video/camera input only single-channel and reuses one buffer; RGBA loads drop alpha; 16-bit colour unsupported.
- [ ] **GAP-6** Math: `LeastSquares` QR/fixed-rank `#if 0`; FFT limited to primes ≤2053, 1/n scaling undocumented; `SobolSequence<double>` UB; "Sobol" generator is Halton with 15 primes.
- [ ] **GAP-7** Geometry: fit routines never detect rank deficiency; `fitEllipse` `#if 0`; PinholeCamera1/2 lack `isInView`/`unprojectZ`/Jacobians; 3-D line utilities missing; `Circle(vector)` unimplemented.
- [ ] **GAP-8** Config: `checkAllFieldsUsed()` is an empty stub.
- [ ] **GAP-9** JSON cereal unregistered/untested; probe reads only 256 bytes.
- [ ] **GAP-10** Display: 3D view scaffolding only; no keyboard input on bgfx path; SDL fallback effectively dead; several plot options stored but unused; status bar stub. OpenGL module uses fixed-function GL in a core-profile context.
- [ ] **GAP-11** Image: small images (1-wide, smaller than kernel) not handled in SummedAreaTable, ConnectedComponents, extendImageMirror; `Boundary::ConvexHull` undeclared; `FloodRegion` maxSize handling.
- [ ] **GAP-12** Tests: no coverage for the cases above; tests that encode bugs (MeanVariance `i % 1`, PeakDetector, ConnectedComponents, Bayesian domain); `test_geometry.cc:610,687` nondeterministic seeds; Display URL parsing untested.

## Improvements

- [ ] **IMP-1** One rule for empty ranges and non-zero origins, plus an offset-origin test per module.
- [ ] **IMP-2** `inverse()` returns `std::optional` everywhere (Affine, Projection, ScaleTranslate, line intersections).
- [ ] **IMP-3** RAII for FFmpeg resources (`unique_ptr` + deleters); index-based TriMesh faces; vector-based SegmentExtrema histograms.
- [x] **IMP-4** Use `ptrdiff_t`/`size_t` for strides, offsets and element counts. Strides, `stride()`, origin offsets and N-D `area()` are now `std::ptrdiff_t` (signed: strides may be negative); `elements()` is `size_t`; indices and per-dimension `size()` stay `int`.
- [ ] **IMP-5** Fix `Concepts.hh:23` `IndexType` (accepts any type).
- [ ] **IMP-6** Display: GUI-thread-only registry, GPU lifetime tied to context, single tokenised URL parser, remove dead code, fewer per-frame copies, bound default plot memory, runtime X11/Wayland selection.
- [ ] **IMP-7** Video: one end-of-stream contract for StreamIterator; smaller probe queue; seek via `av_seek_frame` + decode forward; optional hardware decode; `best_effort_timestamp`; less hot-path logging; fix recursive `shared_mutex` lock in `createIterator`.
- [ ] **IMP-8** Numerics: Welford/compensated variance, relative tolerances for singularity and parallel tests, centred TLS line fit, Umeyama scale.
- [ ] **IMP-9** Optimise: normalise GP inputs/outputs, LLT solves, avoid n×n `K_ss`, use `explorationWeight`, diverse batches, per-call RNG in RandomSearch, drop ambiguous bool constructors, guard `hardware_concurrency()==0`.
- [ ] **IMP-10** Interop: owning conversions to cv::Mat/QImage/dlib, or a documented lifetime contract.
- [ ] **IMP-11** ImageIO: collapse the six PNG writer lambdas; libjpeg/libpng errors via exceptions or status, no objects across `longjmp`.
- [ ] **IMP-12** Minor: `fromString<bool>` case-insensitivity, `Base64` 0xFF, unlocked global type map with `std::cerr`, `ThreadedQueue::push(const T&&)`, Convolve rounding for integer output, single Sobel implementation, `Sums1d2.hh` missing `#pragma once`.
