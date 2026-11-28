# Changes that may affect client code (review fixes, 2026-11-28)

The fixes from the library review (`ReviewFindings.md`) mostly correct wrong results, but some change APIs,
behaviour that callers may have relied on (or worked around), or data written to disk. Check code that uses the
items below. Commits are on `devel`.

Legend: **Build** = code may stop compiling; **Behaviour** = same code, different result; **Data** = files written
by older code; **Throws** = now throws where it used to return garbage.

## Data already saved

| What | Kind | Commit | Check |
|---|---|---|---|
| 16-bit PNGs (Gray16, RGB16, RGBA16) written since 9b41d17 (2025-11-19) were stored byte-swapped on little-endian machines | Data | 6a1cc88 | They now read back byte-swapped, as other tools always did. Repair: load, swap each sample's bytes, save. See "Compatibility notes" in `src/Ravl2/ImageIO/ImageIO_Overview.md`. 8-bit PNGs are fine. |
| Binary cereal files with more than one object | Data | 6a1cc88 | Were corrupted on write (header overwritten), so they can't be read; re-create them. Single-object files are unaffected. |

## Arrays and index ranges

| What | Kind | Commit | Check |
|---|---|---|---|
| Strides, `stride()`, `strides()`, origin offsets are `std::ptrdiff_t` (were `int`); `WindowedArray` requires `strides() -> const std::ptrdiff_t *` | Build | 4aa5a5e | Code storing a stride in `int`, or `strides()` in `const int *`, or array types modelling `WindowedArray`. `-Wconversion` will point at them. |
| `IndexRange<N>::area()`, `IndexRangeSet::area()` return `std::ptrdiff_t` | Build | 4aa5a5e | Assignments to `int`. |
| `IndexRange<1>::size()` is 0 for an empty range (was negative, e.g. -3 for `{6,3}`) | Behaviour | 783c3cc | Code that used a negative size to detect or measure an inverted range. `empty()` is the test for emptiness. |
| Iterating an empty range or array visits nothing (used to hang or read out of bounds); `IndexRange<1>::end() == begin()` when empty | Behaviour | 783c3cc | Loops that relied on the old (broken) end position. |
| 1-D `begin()/end()` start at `range().min()` (were at index 0) | Behaviour | 783c3cc | Anything iterating 1-D sub-arrays or arrays with a non-zero origin got the wrong elements before. |
| N-D `copy(dest, src)` writes to the same indices in `dest` (was `dest`'s first rows) and `convert()` only covers `src`'s range | Behaviour | 783c3cc | Callers copying into a larger `dest` expecting corner placement. 1-D `copy()` is unchanged (prefix copy). |
| Iterator distance (`it - begin`) counts elements for any N | Behaviour | 783c3cc | Code that compensated for the old value. |
| `ArrayIter::window()` / zip `window()` return `ArrayView` (was `ArrayAccess`) | Build | 783c3cc | Code naming the return type. |
| `ArrayAccess::operator()(i, j, ...)` returns a reference for a full index (was a copy) | Behaviour | 783c3cc | `acc(i, j) = v` now writes. `auto x = acc(i, j)` still copies. |
| `ScanWindow<T,N>` (N >= 2) is not copyable | Build | 783c3cc | Code copying or returning a ScanWindow by value from an lvalue. |
| `operator-(n, ArrayIter<T,1>)` removed; `it - n` now subtracts | Build/Behaviour | 783c3cc | |

## IO, video and images

| What | Kind | Commit | Check |
|---|---|---|---|
| Range-for over an input stream with the default end now reads every object (it read none) | Behaviour | 6a1cc88 | Code that worked around it with `get()` loops or explicit end offsets. `streamPosEnd()` is the "no end / next" position. |
| RavlIO video reading ends at end of file (used to repeat the last frame forever) | Behaviour | 6a1cc88 | Loops that stopped after a fixed frame count to avoid the hang. |
| Type conversion picks the least lossy chain (it picked the lossiest) | Behaviour | 6a1cc88 | Loaded/saved results may differ slightly, e.g. colour images no longer going through grey. JPEG may now decode via its planar YUV path. |
| `YUV422Image` chroma planes are half width, full height (were half height) | Behaviour | 6a1cc88 | Code indexing 4:2:2 chroma planes directly. FFmpeg yuv422p, JPEG 4:2:2 and webcam YUYV now agree. |
| Unsigned chroma zero is 128 / 32768 (was 127 / 32767) | Behaviour | 4488c41 | YUV values differ by one; grey now round-trips exactly. |
| Corrupt JPEG/PNG files fail to load (a corrupt JPEG used to call `exit()`) | Behaviour | 6a1cc88 | |
| Saving images with a non-zero origin writes the image's own pixels | Behaviour | 6a1cc88 | |
| OpenCV `toCvMat()` honours the row step of sub-arrays, and throws `std::invalid_argument` for negative strides | Behaviour/Throws | f890e37 | Code that pre-cloned views to get correct results; flipped views. |
| Registered `Array -> cv::Mat` conversions return a deep copy | Behaviour | f890e37 | Code expecting the converted Mat to alias the array (it could dangle). `toCvMat()` itself still returns a view. |
| Qt: `toQImage()`/`toArray()` use rows as height and columns as width (were swapped) | Behaviour | f890e37 | Any code that transposed to compensate. |
| Qt: `toArrayView(QImage)` returns `ArrayView<const T, 2>` and throws on a format mismatch | Build/Throws | f890e37 | It didn't compile before, so there should be no callers. |
| Qt: `toArray()` in Auto mode shares the QImage's pixels (writes alias other QImages sharing them); `toQImage()` keeps the Array alive | Behaviour | f890e37 | Use `CopyModeT::Always` for an independent copy. |
| Dlib: `set_image_size()` gives the requested size (was one smaller); RGB conversion and `DLibArray` respect the array origin | Behaviour | f890e37 | |

## Display

| What | Kind | Commit | Check |
|---|---|---|---|
| Polyline overlay points are (row, col) (were (x, y)) | Behaviour | 88d1057 | Code building overlay polylines as (x, y) must swap; RAVL2 boundaries/polygons can now be passed directly. |
| `display://` controls: flags match whole tokens (`:Clear` no longer matches `:ClearOverlays`, `:ClearPlot`, ...); keys and values are case-insensitive; `Norm=Pct:low,high` keeps its arguments | Behaviour | f890e37 | URLs that relied on `:ClearOverlays` also clearing the channel, or on `mode=replace` being ignored. |
| Percentile normalisation applies on the bgfx path | Behaviour | f890e37 | |
| Image updates replace only the base image under overlays, and new float images take the channel's normalisation | Behaviour | f890e37 | Code that re-added overlays after each image update. |
| Default overlay colour is magenta (was yellow) | Behaviour | f890e37 | |

## Maths and geometry

| What | Kind | Commit | Check |
|---|---|---|---|
| `Quaternion::angle()` returns the full rotation angle in [0, pi] (was half the angle) | Behaviour | 833632d | Any caller of `angle()`; values double. |
| `affineFromScaleAngleTranslation(scale, ...)` applies `scale[i]` to axis i (scale[0] was applied to axis 1) | Behaviour | 833632d | Callers with unequal scales. |
| `MeanVariance` `+=` / `-=` of another MeanVariance include the squared mean difference | Behaviour | 4488c41 | Merged variances were wrong whenever the means differed. |
| `Sums1d2C::operator-=` reduces the count | Behaviour | 4488c41 | |
| FFT of length 0 or 1 returns the input (used to hang) | Behaviour | 4488c41 | |
| `Range::mostEmpty()` / `largest()` cover negative values | Behaviour | 4488c41 | |
| `Circle(p1, p2, p3)` fits the circle (gave radius 0) and throws `std::invalid_argument` for collinear points; `Circle::Angle()` is `atan2` about the centre | Behaviour/Throws | 4488c41 | Use `Circle::fit()` to test for collinearity without exceptions. |
| `intersects(Line2ABC, Range<.,2>)` uses the right box edges | Behaviour | 4488c41 | |
| `Projection::atInfinity()` is the line mapping to infinity (empty for affine); `isNearAffine()` is true for affine transforms; `projectedBounds()` no longer returns `largest()` for affine warps | Behaviour | 4488c41 | |
| `Affine` `a / b` is `a o b^-1`, so `(a / b)(b(x)) == a(x)`; throws `std::domain_error` if `b` is singular | Behaviour/Throws | 4488c41 | |
| `fitIsometry()` returns a unit-scale transform | Behaviour | 4488c41 | |
| `Polygon::contains()` correct on rays along collinear edges; `isSelfIntersecting()` no longer reports simple concave polygons | Behaviour | 4488c41 | |
| `sumOfSqr(scalar)` returns the square (was abs); `angle(a, b)` no longer NaN for parallel vectors; `LinePP::ParClosest` uses all dimensions; `Quaternion::fromAngleAxis` normalises the axis | Behaviour | 4488c41 | |
| `Ellipse(Vector<.,6>)` throws `std::invalid_argument` for a conic that isn't an ellipse | Throws | 4488c41 | |
| `TriMesh`: flip/normalise/stream-load now change the mesh; `operator+` indices correct; copy assignment and moves keep faces on the right vertices; constructors throw `std::out_of_range` for bad face indices | Behaviour/Throws | 4488c41 | Vertex normals are now unit length. |
| `PinholeCamera1`: inverse distortion works for k1 <= 0 and small radii; Jacobian corrected; `projectCheck()` (also `PinholeCamera2`) rejects points nearer than `nearPlane()` | Behaviour | 4488c41 | Undistorted points and ray directions differ for barrel distortion; points just in front of the camera may now be rejected. |
