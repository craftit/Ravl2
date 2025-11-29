# GoPro Metadata Frame Design Review

## Scope
- `src/Ravl2/GoPro/GpmfFrame.hh`
- `src/Ravl2/GoPro/GpsFrame.hh`
- `src/Ravl2/GoPro/GyroFrame.hh`
- `src/Ravl2/GoPro/AccelFrame.hh`
- `src/Ravl2/GoPro/GpmfParser.hh` / `src/Ravl2/GoPro/GpmfParser.cc`
- `src/Ravl2/GoPro/GpmfUtilities.hh` / `src/Ravl2/GoPro/GpmfUtilities.cc`
- `src/Ravl2/Video/Frame.hh`

## Key Findings

### 1. Redundant inheritance layer (`src/Ravl2/GoPro/GpmfFrame.hh`)
`GpmfFrameBase` subclasses `Video::MetaDataFrameBase` but contributes no state or API—it only re-exposes constructors. Every concrete metadata frame already inherits `Video::MetaDataFrame<T>`, so removing the empty intermediate type would simplify the hierarchy with no behavior change. If a common interface is required, prefer declaring actual pure-virtual hooks (e.g., `sensorKind()`) instead of an empty marker.

### 2. Overuse of `std::any` in the frame core (`src/Ravl2/Video/Frame.hh`)
`Frame::frameData()` forces all metadata to be exposed through `std::any`, which allocates and requires callers to know the concrete payload type. Since metadata frames already use `MetaDataFrame<T>`, consider returning typed references (CRTP) or a `std::variant` of supported payloads. This would:
- Eliminate `std::bad_any_cast` failure paths in downstream code.
- Reduce allocations when shipping metadata-heavy GoPro streams.
- Let compilers optimize hot paths that repeatedly touch gyro/GPS data.
Since the frame system must stay open-ended (no master list of metadata types), the improvement focus should be on helper layers that stay out-of-tree: e.g., a `frameDataView<T>` utility that queries a `TypeConverter` registry for adapters and returns `std::optional<std::reference_wrapper<const T>>`, or a lightweight visitor that operates on registered adapter IDs instead of a closed `std::variant`. These approaches keep `Frame` unmodified yet still provide typed access without `std::bad_any_cast` surprises.

### 3. Boilerplate metadata subclasses (`src/Ravl2/GoPro/GpsFrame.hh`, `src/Ravl2/GoPro/GyroFrame.hh`)
Both classes only customize `dataTypeName()` and expose minor helpers while deferring storage and lifetime to the base template. A traits-based pattern (e.g., `MetaDataFrame<T, Traits>`) or even simple alias helpers would avoid proliferating nearly empty subclasses for each sensor type. This also keeps naming consistent and makes it easier to add future telemetry frames without touching multiple headers.

### 4. Gyro data access copies unnecessarily (`src/Ravl2/GoPro/GyroFrame.hh`)
`GyroFrame::asVectorArray()` allocates and copies every sample on each call despite the underlying `std::vector<GyroSample>` already storing `Vector3f` values. Returning a `std::span<const Vector3f>` (or an iterator pair) would provide zero-copy access for signal-processing code, reduce heap churn, and better align with the project preference for `std::span`.

### 5. Missing validation/logging for gyro sample rate (`src/Ravl2/GoPro/GyroFrame.hh`)
`mSampleRate` defaults to `0` and accepts any float without guards. Invalid sample rates silently propagate into downstream computations. Enforcing `sampleRate > 0`, clamping to expected GoPro ranges, and logging via `spdlog` on invalid inputs would make failures obvious and match the "log critical failures" guideline.

### 6. Documentation gaps
Current doxygen comments explain intent but not usage nuances (e.g., that gyro frames may hold dozens of samples or that `frameData()` requires `std::any` casts). Adding brief usage notes or examples would make the metadata API easier to consume for non-GoPro callers.

### 7. Accelerometer frame mirrors gyro issues (`src/Ravl2/GoPro/AccelFrame.hh`)
`AccelFrame` duplicates the same sample-rate storage and `asVectorArray()` copy pattern as `GyroFrame`, including the lack of validation/logging and the per-call heap churn. Once the gyro frame is addressed (validation + span access), mirror the changes here or generalize them via a shared helper.

### 8. Parser leaves scaling/timing as stubs (`src/Ravl2/GoPro/GpmfParser.cc`)
`getScaleFactor()` ignores the requested FourCC and simply walks to the nearest `SCAL`, which can mix scales when multiple telemetry blocks exist. `getSampleRate()` always returns `200 Hz`, so exported `GyroFrame`/`AccelFrame` instances silently claim constant rates regardless of source footage. Both behaviors make downstream analytics unreliable.

### 9. Parser resilience/logging gaps (`src/Ravl2/GoPro/GpmfParser.cc`)
When `parseGyro()`/`parseAccel()` encounter malformed payloads, the parser just returns empty vectors without logging. That makes it hard to distinguish "no sensor in stream" from "sensor present but parse failed". Tying failures to `SPDLOG_WARN`/`SPDLOG_ERROR` (with packet offsets) would help triage ingest issues.

### 10. Utility math drops small rotations (`src/Ravl2/GoPro/GpmfUtilities.cc`)
`integrateGyroToOrientation()` sets `deltaQ = identity()` whenever the instantaneous rotation magnitude falls below `1e-6`, which means all sub-threshold angular velocity samples are treated as zero rotation. Over thousands of samples that accumulates noticeable drift. Even the small-angle branch should update `deltaQ` with the linearized quaternion `[1, ω·dt/2]`. Similarly, `lowPassFilter()` never bounds `alpha` to `[0,1]`, so passing user input outside that range yields unstable results.

## Recommended Next Steps
1. Remove `GpmfFrameBase` and migrate existing frames to inherit directly from `Video::MetaDataFrame<T>`; add a lightweight concept/trait if a shared interface is needed.
2. Build typed-access helpers (adapter registry or `frameDataView<T>` utilities) on top of `Video::Frame` so metadata consumers can avoid `std::any` casts without freezing the set of supported frame types.
3. Introduce a metadata traits helper to consolidate per-sensor boilerplate such as `dataTypeName()` while keeping custom helpers (e.g., `location()`) nearby.
4. Rework `GyroFrame::asVectorArray()`/`AccelFrame::asVectorArray()` to provide `std::span<const Vector3f>` or cached views, validate sample rates, and add unit tests proving no allocations occur when accessing sensor vectors.
5. Validate and log invalid gyro/accel sample rates at construction time; consider storing them as `float mSampleRate{0.0F};` with assertions or errors for non-positive values.
6. Expand doxygen comments with short usage examples (e.g., how to convert gyro samples to `Vector3f` spans) to align with the project documentation expectations.
7. Implement proper FourCC-specific scaling/sample-rate extraction in `GpmfParser`, and add logging when parsing fails so ingest issues surface immediately.
8. Fix `integrateGyroToOrientation()` to handle sub-micro rotations and clamp `lowPassFilter()`'s `alpha`; add quick unit tests covering both edge cases.
