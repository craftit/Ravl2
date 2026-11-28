# Changelog

All notable changes to RAVL2 are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). RAVL2 has not made a tagged release yet, so everything
is under [Unreleased]; when a version is tagged, that heading becomes the version and date.

Entries marked **Breaking** may stop client code compiling. Entries under *Changed* alter results that callers
may have relied on or worked around: check code that uses them.

## [Unreleased]

### Data compatibility
- **16-bit PNGs written since 2025-11-19 (9b41d17) are byte-swapped.** On little-endian machines the writer stored
  Gray16/RGB16/RGBA16 samples little-endian; PNG requires big-endian. The old reader had the matching mistake, so
  round trips looked correct, but other tools always read these files swapped, and now RAVL2 does too. Repair by
  loading, swapping each sample's bytes (`v = uint16_t((v >> 8) | (v << 8))`) and saving again. 8-bit PNGs are
  unaffected. See "Compatibility notes" in `src/Ravl2/ImageIO/ImageIO_Overview.md`.
- **Videos written with RAVL2 `YUV420Image` input** stored RAVL2's full-range YUV as if it were limited range
  (contrast reduced, blacks lifted, in other players), and full-range video read as YUV came back unexpanded. RGB
  input was unaffected. Files can't be told apart afterwards; re-encode from source if it matters.
- **Binary cereal files holding more than one object** were corrupted when written (the header was overwritten) and
  can't be read; re-create them. Single-object files are unaffected.

### Changed
- **Breaking:** video read through FFmpeg as YUV is **full range** (0-255), RAVL2's YUV convention, instead of the
  file's usual limited range (16-235). Pixel values of YUV frames change for nearly all video; code that assumed
  limited range (or compensated for it) must be checked. The matrix is left as in the file (BT.709 for most HD).
  Frames delivered as RGB use the file's own range and matrix, so their colours are now correct.
- **Breaking:** video read through FFmpeg as RGB/RGBA is `Array<PixelRGB8, 2>` / `Array<PixelRGBA8, 2>` (was
  `RGBPlanarImage`/`RGBAPlanarImage`, which `dataType()` reported but was never what the frames held). 10/12-bit
  YUV is delivered as `YUV420Image<uint16_t>` scaled to the full 16-bit range (P010 frames were 64x too bright).
- The FFmpeg writer tags YUV output as limited range BT.601 (`tv`, `smpte170m`), matching what it writes.
- **Breaking:** `camera://` URLs go to the FFmpeg capture handler first (priority 0, was -2); OpenCV's camera handler
  (-1) only takes a bare index, as a fallback. A device path or index with unsupported options now throws
  `std::invalid_argument` naming the option and listing the device's modes, instead of opening in another mode.
  See `src/Ravl2/Video/CaptureDevices.md`.
- `FloodRegionC::GrowRegion(..., maxSize)` fails for regions larger than `maxSize` (false, or 0 and an empty
  mask), stopping the flood there. It was ignored for boundaries, and masks were left partly unwritten.
- `SummedAreaTable`: the extra row and column after the image hold the totals (they held running sums of the last
  image column and row), which changes `sampleGrid()` at the far edges.
- `extendImageMirror()`/`mirrorEdges()` work for any border width and image size, reflecting repeatedly
  (`... 1 | 0 1 2 | 1 0 ...`); a 1-pixel image is replicated. They threw, or read unwritten border.
- `ImagePyramid::findLevel()` picks the level with the closest per-axis scale (summed |log ratio|).
- Geometry `operator>>` read back what `operator<<` writes: `Affine` (new), `LinePP`, `LinePV` (it read two points;
  `<<` writes a point and a direction), `Plane3PVV`, `VectorOffset`, `Conic2`, `Ellipse`, `Projection`, `Moments2`.
- **Breaking:** `Boundary::ConvexHull()` (declared, never defined) is replaced by `convexHull<RealT>(boundary)`,
  returning a `Polygon`. The `Pixel` value constructor needs one value per channel.
- `warpScale(result, image)` places enlarged samples corner to corner, step (in-1)/(out-1): the first and last
  output pixels sample the first and last image pixels. It threw `std::out_of_range` on every enlargement.
- **Breaking:** array strides, `stride()`, `strides()` and origin offsets are `std::ptrdiff_t` (were `int`), and
  the `WindowedArray` concept requires `strides() -> const std::ptrdiff_t *`. Strides stay signed (they may be
  negative). `IndexRange<N>::area()` and `IndexRangeSet::area()` return `std::ptrdiff_t`. Check code that stores
  strides or areas in `int`.
- **Breaking:** `ArrayIter::window()` and the zip iterator's `window()` return an `ArrayView`, which owns its
  range (was an `ArrayAccess` pointing at a temporary).
- **Breaking:** `ScanWindow<T,N>` for N >= 2 is not copyable; its iterator refers to its own scan area.
- **Breaking:** Qt `toArrayView(QImage)` returns `ArrayView<const T, 2>` and throws if the format doesn't match the
  pixel type. (It didn't compile before.)
- **Breaking:** polyline overlay points sent to the display are (row, col), like other RAVL2 points (were (x, y)).
  Swap any overlays built as (x, y); boundaries and polygons can now be passed directly.
- `IndexRange<1>::size()` is 0 for an empty range (was negative, e.g. -3 for `{6,3}`); use `empty()` to test.
  Iterating an empty range or array now visits nothing, and `end() == begin()` for an empty 1-D range.
- N-D `copy(dest, src)` writes to the same indices in `dest` (was `dest`'s first rows), and `convert()` only covers
  `src`'s range. 1-D `copy()` still copies a prefix.
- `ArrayAccess::operator()` returns a reference for a full index, so `acc(i, j) = v` writes (it wrote a copy).
- The type converter picks the least lossy conversion chain (it picked the lossiest), so some loads and saves take
  different routes; e.g. colour no longer goes through grey, and JPEG may decode via planar YUV.
- `YUV422Image` chroma planes are half width and full height (were half height). FFmpeg yuv422p, JPEG 4:2:2 and
  webcam YUYV now agree. Check code that indexes 4:2:2 chroma directly.
- Unsigned chroma zero is 128 / 32768 (was 127 / 32767), so YUV values may differ by one; grey round-trips exactly.
- OpenCV `toCvMat()` honours the row step of sub-arrays and throws `std::invalid_argument` for negative strides.
  The registered `Array -> cv::Mat` conversions return deep copies (`toCvMat()` itself still returns a view).
- Qt `toQImage()` and `toArray()` use rows as height and columns as width (were swapped). `toArray()` in Auto mode
  shares the QImage's pixels; `toQImage()` keeps the Array alive. Use `CopyModeT::Always` for independent copies.
- Dlib `set_image_size()` gives the requested size (was one smaller in each dimension).
- `display://` controls match whole tokens, so `:Clear` no longer also matches `:ClearOverlays`, `:ClearPlot`, etc.
  Keys and values are case-insensitive (`mode=replace` now works), and `Norm=Pct:low,high` keeps its arguments.
- Display image updates replace only the base image under overlays, and new float images take the channel's
  normalisation. The default overlay colour is magenta, as documented (was yellow).
- `Quaternion::angle()` returns the full rotation angle in [0, pi] (was half the angle).
- `affineFromScaleAngleTranslation(scale, ...)` applies `scale[i]` to axis i (`scale[0]` was applied to axis 1).
- `Affine` `a / b` is `a o b^-1`, so `(a / b)(b(x)) == a(x)`.
- `Circle(p1, p2, p3)` throws `std::invalid_argument` for collinear points (use `Circle::fit()` to test without
  exceptions). `Ellipse(Vector<.,6>)` throws for a conic that isn't an ellipse. `Affine` division throws
  `std::domain_error` for a singular divisor. `TriMesh` constructors throw `std::out_of_range` for bad face indices.
- `PinholeCamera1`/`PinholeCamera2` `projectCheck()` reject points nearer than `nearPlane()` (was only z near 0).
- `LocatePeakSubPixel()` returns (row, col) with the sub-pixel correction on the right axes (they were swapped);
  this affects Harris, Harris-affine and patch matching positions.
- `ConnectedComponents::apply()` returns the number of label values used including the reserved 0 (it returned
  the number of provisional labels); with `ignoreZero` it is the renumbered count.
- `warpSubsample()` only produces output pixels whose footprint lies inside the image (one fewer at the far edge),
  and reallocates a result that is larger than that; `warpScaleBilinear()` throws `std::out_of_range` for such a
  result.
- `MatchNormalisedCorrelation`: scores are correlations in [-1, 1], `searchArea` restricts the template positions,
  and the default threshold is 0.5 (it was 10, so no match was ever reported); `SetThreshold()` added.
- Susan corners report (vertical, horizontal) gradients in that order (they were swapped).
- `Segmentation::Areas()` has one entry per label (it had one extra).
- Video seeking finds the keyframe at or before the target (at or after for `SeekFlags::Next`); `Precise` then
  decodes forward to the first frame at or after the target. Seeking past the end fails (it reported success).
- A new `FfmpegMultiStreamIterator` on a container that has already been read starts at the beginning (seekable
  inputs); `isAtEnd()` is only true once no decoded frames remain; `getFrameById()` opens the file again rather
  than disturbing the calling iterator.
- FFmpeg audio streams report `AudioChunk<SampleT>` as their data type (was `Array<SampleT, 2>`), matching the
  frames, so typed audio iterators work.
- The FFmpeg writer rejects audio chunks whose channel count doesn't match the encoder.
- `BayesianOptimise`: the GP works on inputs scaled to the domain, so `gpLengthScale` is a fraction of the domain
  (it was in raw units), and on standardised targets; the initial design has at least 2*dim+1 points; an explicit
  all-zero start point is used (it was ignored).
- `RandomSearchOptimise` grid sampling uses `gridPointsPerDim` (it was sized from the batch).
- `ConfigFactory` registration functions return false for duplicates (they terminated the program), and creating
  a type with no default constructor registered is a clear `std::runtime_error`.
- **Breaking:** `GLWindow` is not movable (the old moves left GLFW callbacks pointing at the moved-from window).
- `RAVL2_MIN_DEPS` disables the Qt, dlib and Display modules even when those libraries are installed.
- CMake no longer reads or writes the user package registry (`~/.cmake/packages`); Eigen's build used to register
  its build directory there.
- `Ravl2Core` links `nlohmann_json::nlohmann_json` publicly (its headers include it; it only worked because a system
  copy was on the default include path).
- GoPro GPS fields use their own SCAL divisors (altitude and speeds were 10^4-10^5 too small), and gyro/accel
  SCAL is read with its 16-bit type (values were about 65536x too small).
- CMake: the Display tests and display examples are only built when the Display stack is enabled; ImGui, ImPlot,
  ImGuizmo and the bgfx ImGui headers are SYSTEM includes.
- ImGui (`docking`), ImPlot (`master`) and gpmf-parser (`main`) are pinned to commits; see
  "Pinned dependencies" in `README_dependencies.md` for how to review and update them.

### Fixed
- Capture: `format=mjpeg` (or any compressed format) was ignored on Linux (it was passed as FFmpeg's raw
  `pixel_format`, not `input_format`); a device that substituted another size was used silently; OpenCV camera
  capture couldn't deliver colour frames ("Don't know how to convert OpenCV image type: 16") and reused one
  buffer, overwriting images already returned; `camera://8?size=...` went to OpenCV, which dropped the options.
- ctest reports Catch2 tests that skip themselves (exit code 4) as skipped, not failed.
- Templates that didn't compile when used: `forEachIndexFirst`, `fft2`, the double FFT (also not instantiated),
  `span(const VectorT&)`, `Sums1d2C` streaming, `ArrayView<T,1>` from a vector, `PixelY8(uint8_t)` and other
  one-channel pixels, `PinholeCameraArray::Triangulate` (its singularity test also rejected perpendicular rays
  rather than parallel ones), `ImagePyramid::findLevel`, OpenCV `toAffine`/`toCvAffine`/`toCvMatx`, the geometry
  `operator>>`s above, and `SegmentExtrema::apply(img, roi)`/`applyMask(img, roi)` (the callback overload captured
  them; non-byte pixel types also failed to compile).
- `SummedAreaTable::buildTable()` returned an empty table for images under 2x2.
- `PlanarImage::range()` and subsampled plane ranges: an even-sized 4:2:0 image reported a range one row and column
  too big (the planes' coverage was unioned and rounded up), and conversions read past the chroma data.
- Pixel channel conversions between integer types overflowed `int` for 16-bit values (undefined behaviour; wrong
  pixels converting `uint16_t` YUV).
- `warpScaleBilinear` could read one column past the image edge (positions accumulated float error, and only the
  row neighbour was clamped). `warpSubsample` and `warpScale` didn't compile for different input and output
  element types.
- Range-for over an input stream with the default end read nothing (`numeric_limits<std::streampos>::max()` is 0).
- RavlIO video reading never ended; it repeated the last frame.
- A corrupt JPEG ended the process (libjpeg's default `exit()`); JPEG and PNG errors are now reported as load or
  save failures.
- PNG: grey+alpha files corrupted the heap, 16-bit grey was byte-swapped, interlaced files failed, palette and
  alpha transforms were chosen from the wrong colour type, and images with a non-zero origin were saved from the
  wrong memory (JPEG too). Planar JPEG decode wrote chroma to the wrong rows.
- 1-D arrays with a non-zero origin iterated the wrong memory; empty ranges hung or read out of bounds;
  `elements()` gave 1 for some empty ranges and overflowed for large volumes; copying into a larger array read past
  the source; N-D iterator distances were wrong; debug bounds checks skipped the last dimension;
  `ScanWindow<T,1>::index()` was inverted; `ThreadedQueue` could pop an empty queue after a spurious wake-up.
- OpenCV sub-array views were read with the wrong row pitch; 4-channel images were read with the wrong stride;
  multi-step save chains could read freed memory.
- Qt conversions dropped the last row when copying and asserted Grayscale8 for every type; Dlib conversions
  ignored the array origin and `toArrayView` didn't compile.
- Display: a crash at exit (bgfx textures destroyed after shutdown), a GUI deadlock when using the Controls panel
  while streaming, ImGui/ImPlot/3D mouse-wheel scrolling cancelling out, percentile normalisation ignored on the
  bgfx path, overlays erased by image updates, wrong colours in the SDL fallback, and on macOS a hang at exit for
  programs that never display anything.
- `MeanVariance` merge/remove omitted the square of the mean difference; `Sums1d2C::operator-=` increased the
  count; an FFT of length 1 hung.
- `Range::mostEmpty()`/`largest()` ignored negative values; `Circle`'s 3-point constructor discarded its fit and
  `Circle::Angle()` was wrong; `intersects(Line2ABC, Range)` used the wrong box edges; `Projection::atInfinity()`
  and `isNearAffine()` were wrong (so `projectedBounds()` returned `largest()` for affine warps); `fitIsometry()`
  applied a scale; `Polygon::contains()` was wrong on rays along edges and `isSelfIntersecting()` reported simple
  concave polygons; `sumOfSqr(scalar)`, `angle(a, b)` for parallel vectors, 3-D `LinePP::ParClosest` and
  `Quaternion::fromAngleAxis` with a non-unit axis were wrong.
- `TriMesh`: flipping, normal normalisation and stream loading had no effect, `operator+` produced garbage indices,
  and copy assignment left faces pointing at the source mesh.
- `PinholeCamera1`: inverse distortion was skipped for barrel distortion and small radii, and the projection
  Jacobian's radial term was wrong.
- Image: `edgeSobel()` used the wrong pixel in its vertical kernel; `SegmentExtrema` overflowed its region map on
  checkerboard-like and 1x1 images; the wide `DrawFrame()` swapped rows and columns (breaking `extendImageFill()`
  and flood-fill padding on non-square images); `warpSubsample()` read past the image edge;
  `warpScaleBilinear()` didn't interpolate rows when the image origin wasn't 0; Warp Clamp/Wrap/Mirror modes left
  out-of-source pixels unwritten (and didn't compile); `SummedAreaTable2C::Sum()` returned the wrong sum and
  `SummedAreaTable::verticalDifference2()` had two signs flipped; segmentation adjacency and boundary lengths
  missed the first column of each row; `ConnectedComponents` asserted on single-row/column images;
  `toPolygon(Boundary)` offset the first vertex twice; the reference DCT was wrong for non-square images and
  non-zero origins.
- FFmpeg writer: a failed packet flush freed packets twice, a failed first frame leaked its encoder and a retry
  added a second audio stream, and `sws_scale` read past unpadded pixel buffers. FFmpeg iterators: a constructor
  that threw left the container's iterator count raised; on macOS every captured frame leaked a buffer reference.
- `BayesianOptimise` with `adaptiveLengthScale` crashed on its second iteration, and adapted length scales
  compounded every iteration.
- `ConfigNode::setValue()` on an empty node recorded the old (void) type.
- OpenGL: `GLWindow` used its window after `glfwTerminate()`; `View3D` rotated about the wrong point, rendered
  without a null check or lock, and wireframe `DTriMesh` drawing used `GL_LINE` (not a primitive).
- `searchMinAbsDifference()` and `searchMinAbsDifferenceCreep()` didn't compile; the first compared the template
  with the whole image rather than each window, and the second discarded its sub-pixel result.
  `matchSumAbsDifference()` accepts any 2-D view as the image.
- Builds against FFmpeg older than 7.1 (`avcodec_get_supported_config()` is used when available).
- Configuring with `RAVL2_ENABLE_DISPLAY_STACK=OFF` failed; fetching bgfx failed with a user-level
  `submodule.recurse=true`; ImGui header warnings broke `-Werror` builds.

### Added
- Capture options on the URL: `camera:///dev/video8?size=1280x720&fps=60&format=mjpeg&latest=1` (also
  `camera://8`). `latest=1` delivers only the newest frame via `Video::LatestFrameReader`, which can also be used
  directly. `Video/CaptureDevice.hh` parses capture URLs, lists a V4L2 device's formats, sizes and frame rates
  (`listCaptureModes()`), and checks a request against them. URL forms, timing and buffering are documented in
  `src/Ravl2/Video/CaptureDevices.md`, with design notes for a native V4L2 backend and audio capture.
- FFmpeg reader: any pixel format FFmpeg decodes is readable (ProRes, 10/12/16-bit, `gbrp`, `gray16`, alpha, palette
  …): formats without a direct RAVL2 type are converted with swscale (alpha to RGBA, grey to `PixelI8`, other RGB
  to RGB, high bit depth YUV to `YUV420Image<uint16_t>`, other YUV to `YUV420Image<uint8_t>`). They used to throw
  "Unsupported pixel format". RavlIO video plans cover every delivered type, including grey and 10-bit.
- Install and export of the core libraries: `cmake --install` then `find_package(ravl2 CONFIG)` and link
  `ravl2::Ravl2Core`, `ravl2::Ravl2Image`, etc. (see README_building.md). CPM-downloaded fmt, spdlog, Eigen, cereal
  and nlohmann_json install alongside. Display, OpenGL, Qt and Dlib remain build-tree only.
- `streamPosEnd()` (`IO/StreamPosition.hh`): the "no end / next object" stream position.
- `Display/UrlControls.hh`: tested parser for `display://` URL controls.
- `ChannelRegistry::clearAll()` and `baseContentSlot()` for display scene content.
- `Video/FfmpegResource.hh`: `std::unique_ptr` types for FFmpeg frames, packets, codec contexts, scalers,
  resamplers and audio FIFOs. The writer and the stream iterator now own their FFmpeg objects through them, so
  error and exception paths no longer leak (e.g. an iterator constructor that threw after allocating).
- Tests: array ranges and iteration, OpenCV and Qt interop, Display commands and URL parsing, stream and video EOF,
  PNG variants, corrupt JPEG, TriMesh edits, camera distortion, and the maths and geometry fixes.

### Removed
- `operator-(n, ArrayIter<T,1>)`, which had no sensible meaning; `it - n` now subtracts.
