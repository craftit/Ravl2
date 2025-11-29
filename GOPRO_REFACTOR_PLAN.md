# GoPro Metadata Implementation Refactor Plan

## Overview
This plan addresses the issues identified in REVIEW.md while maintaining compatibility with the `Ravl2::Video::Frame` interface.

## Constraints
- **MUST NOT** change the `Ravl2::Video::Frame` interface
- **MUST NOT** break existing code using the current frame types
- **SHOULD** maintain backward compatibility where possible
- **SHOULD** follow RAVL2 coding conventions (C++23, no raw pointers, etc.)

---

## Phase 1: Simplify Type Hierarchy (Low Risk)

### Task 1.1: Remove GpmfFrameBase
**Files**: `src/Ravl2/GoPro/GpmfFrame.hh`, `GpsFrame.hh`, `GyroFrame.hh`, `AccelFrame.hh`

**Current**:
```cpp
class GpmfFrameBase : public Video::MetaDataFrameBase { };  // Empty
class GpsFrame : public Video::MetaDataFrame<GpsFix>, public GpmfFrameBase { };
```

**New**:
```cpp
// Remove GpmfFrame.hh entirely
class GpsFrame : public Video::MetaDataFrame<GpsFix> { };
```

**Impact**: None - `GpmfFrameBase` contributes no functionality
**Testing**: Verify existing code compiles and runs

---

## Phase 2: Zero-Copy Data Access (Medium Risk)

### Task 2.1: Add span-based access to GyroFrame
**Files**: `src/Ravl2/GoPro/GyroFrame.hh`, `GyroFrame.cc` (if exists)

**Current**:
```cpp
[[nodiscard]] Array<float, 2> asVectorArray() const {
  Array<float, 2> result({mData.size(), 3});
  // Copies all samples...
}
```

**New**:
```cpp
// Add zero-copy span access
[[nodiscard]] std::span<const GyroSample> samples() const {
  return std::span<const GyroSample>(mData);
}

// Keep legacy method but mark deprecated
[[deprecated("Use samples() for zero-copy access")]]
[[nodiscard]] Array<float, 2> asVectorArray() const;
```

**Rationale**:
- `std::span` is C++23-friendly and avoids allocation
- Maintains backward compatibility with deprecation warning
- Aligns with RAVL2 preference for spans over copies

**Testing**:
- Benchmark showing zero allocations with `samples()`
- Verify `asVectorArray()` still works

### Task 2.2: Mirror changes to AccelFrame
Same pattern as 2.1, applied to `AccelFrame.hh`

---

## Phase 3: Validation and Logging (Low Risk)

### Task 3.1: Add sample rate validation to GyroFrame
**Files**: `src/Ravl2/GoPro/GyroFrame.hh`

**Current**:
```cpp
GyroFrame(std::vector<GyroSample> data, /*...*/ float sampleRate)
  : /*...*/ mSampleRate(sampleRate) { }  // No validation
```

**New**:
```cpp
GyroFrame(std::vector<GyroSample> data, /*...*/ float sampleRate)
  : /*...*/ mSampleRate(validateSampleRate(sampleRate)) { }

private:
  static float validateSampleRate(float rate) {
    if (rate <= 0.0F) {
      SPDLOG_ERROR("Invalid gyro sample rate: {} Hz, using 200 Hz default", rate);
      return 200.0F;  // GoPro typical rate
    }
    if (rate < 50.0F || rate > 400.0F) {
      SPDLOG_WARN("Unusual gyro sample rate: {} Hz (expected 50-400 Hz)", rate);
    }
    return rate;
  }
```

**Rationale**:
- Catches invalid data early
- Provides sensible defaults for GoPro hardware
- Logs issues for debugging

### Task 3.2: Add validation to AccelFrame
Mirror Task 3.1 for accelerometer (typical range: 50-400 Hz)

### Task 3.3: Add parse failure logging to GpmfParser
**Files**: `src/Ravl2/GoPro/GpmfParser.cc`

**Changes**:
```cpp
std::vector<GyroSample> GpmfParser::parseGyro(GPMF_stream* stream, float& sampleRate) {
  std::vector<GyroSample> samples;

  if (stream == nullptr) {
    SPDLOG_ERROR("parseGyro called with null stream");
    return samples;
  }

  uint32_t sampleCount = GPMF_Repeat(stream);
  if (sampleCount == 0) {
    SPDLOG_DEBUG("No gyro samples in GPMF stream");
    return samples;
  }

  auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
  if (rawData == nullptr) {
    SPDLOG_WARN("Failed to get gyro raw data from GPMF stream");
    return samples;
  }

  // ... rest of parsing
}
```

---

## Phase 4: Parser Correctness (Medium Risk)

### Task 4.1: Fix getScaleFactor() to be FourCC-specific
**Files**: `src/Ravl2/GoPro/GpmfParser.cc`

**Current**:
```cpp
float GpmfParser::getScaleFactor(GPMF_stream* stream, [[maybe_unused]] uint32_t fourcc) const {
  // Ignores fourcc parameter!
  if (GPMF_FindPrev(&tempStream, MAKEID('S', 'C', 'A', 'L'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
    // Returns first SCAL found
  }
}
```

**New**:
```cpp
float GpmfParser::getScaleFactor(GPMF_stream* stream, uint32_t fourcc) const {
  if (stream == nullptr) {
    return 1.0f;
  }

  // Save current position
  GPMF_stream tempStream = *stream;

  // Navigate to the specific sensor (fourcc) first
  GPMF_stream searchStream = *stream;
  if (GPMF_FindPrev(&searchStream, fourcc, GPMF_RECURSE_LEVELS) != GPMF_OK) {
    SPDLOG_DEBUG("Could not find sensor {} for scale lookup", fourcc);
    return 1.0f;
  }

  // Now look for SCAL within that sensor's context
  if (GPMF_FindPrev(&searchStream, MAKEID('S', 'C', 'A', 'L'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
    auto* scaleData = static_cast<uint32_t*>(GPMF_RawData(&searchStream));
    if (scaleData != nullptr) {
      return 1.0f / static_cast<float>(*scaleData);
    }
  }

  // Fallback to default scales for known sensors
  if (fourcc == MAKEID('G', 'Y', 'R', 'O')) {
    return 1.0f / 32768.0f;  // Typical GoPro gyro scale
  }
  if (fourcc == MAKEID('A', 'C', 'C', 'L')) {
    return 1.0f / 4096.0f;   // Typical GoPro accel scale
  }

  return 1.0f;
}
```

### Task 4.2: Implement proper getSampleRate() parsing
**Files**: `src/Ravl2/GoPro/GpmfParser.cc`

**Current**:
```cpp
float GpmfParser::getSampleRate([[maybe_unused]] GPMF_stream* stream) const {
  return 200.0f;  // Always returns 200 Hz!
}
```

**New**:
```cpp
float GpmfParser::getSampleRate(GPMF_stream* stream) const {
  if (stream == nullptr) {
    return 0.0f;
  }

  // Look for ORIN (original sample rate) or TSMP (total samples) + duration
  GPMF_stream tempStream = *stream;

  // Try ORIN first (original sample rate in Hz)
  GPMF_stream searchStream = tempStream;
  if (GPMF_FindPrev(&searchStream, MAKEID('O', 'R', 'I', 'N'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
    auto* rateData = static_cast<uint32_t*>(GPMF_RawData(&searchStream));
    if (rateData != nullptr) {
      return static_cast<float>(*rateData);
    }
  }

  // Calculate from sample count and time duration
  uint32_t sampleCount = GPMF_Repeat(&tempStream);

  // Look for time scale information
  // ... (more sophisticated parsing based on GPMF spec)

  // Fallback to typical GoPro rates
  SPDLOG_WARN("Could not determine sample rate from GPMF, using 200 Hz default");
  return 200.0f;
}
```

**Note**: This requires deeper study of GPMF specification for timing information.

---

## Phase 5: Math Utilities Fixes (Low Risk)

### Task 5.1: Fix integrateGyroToOrientation() small-angle handling
**Files**: `src/Ravl2/GoPro/GpmfUtilities.cc`

**Current**:
```cpp
if (rotationMagnitude < 1e-6) {
  deltaQ = Quaternionf::identity();  // Drops small rotations!
}
```

**New**:
```cpp
if (rotationMagnitude < 1e-6) {
  // Use small-angle approximation: q ≈ [1, ω·dt/2]
  // This ensures we don't accumulate drift from dropped micro-rotations
  Vector3f halfAngle = angularVelocity * (dt / 2.0f);
  deltaQ = Quaternionf(1.0f, halfAngle.x(), halfAngle.y(), halfAngle.z());
  deltaQ.normalize();
} else {
  // ... existing code for normal rotations
}
```

### Task 5.2: Add bounds checking to lowPassFilter()
**Files**: `src/Ravl2/GoPro/GpmfUtilities.cc`

**Current**:
```cpp
Vector3f lowPassFilter(const Vector3f& current, const Vector3f& previous, float alpha) {
  return alpha * current + (1.0f - alpha) * previous;  // No bounds check!
}
```

**New**:
```cpp
Vector3f lowPassFilter(const Vector3f& current, const Vector3f& previous, float alpha) {
  // Clamp alpha to valid range
  float clampedAlpha = std::clamp(alpha, 0.0f, 1.0f);

  if (clampedAlpha != alpha) {
    SPDLOG_WARN("Low-pass filter alpha {} out of range [0,1], clamped to {}",
                alpha, clampedAlpha);
  }

  return clampedAlpha * current + (1.0f - clampedAlpha) * previous;
}
```

---

## Phase 6: Reduce Boilerplate (Optional, Lower Priority)

### Task 6.1: Create metadata frame traits
**Files**: New file `src/Ravl2/GoPro/MetadataTraits.hh`

**Approach**:
```cpp
namespace Ravl2::GoPro {

  // Traits for sensor metadata
  template<typename T>
  struct SensorTraits;

  // Specialization for GPS
  template<>
  struct SensorTraits<GpsFix> {
    static constexpr const char* typeName() { return "GoPro GPS"; }
    static constexpr const char* description() { return "GPS location and velocity data"; }
  };

  // Specialization for Gyro
  template<>
  struct SensorTraits<std::vector<GyroSample>> {
    static constexpr const char* typeName() { return "GoPro Gyroscope"; }
    static constexpr const char* description() { return "Angular velocity measurements"; }
    static constexpr float expectedRateMin() { return 50.0f; }
    static constexpr float expectedRateMax() { return 400.0f; }
  };

  // Base template for sampled sensor frames (gyro, accel)
  template<typename SampleT, typename Traits>
  class SampledSensorFrame : public Video::MetaDataFrame<std::vector<SampleT>> {
  public:
    using Sample = SampleT;

    [[nodiscard]] std::span<const Sample> samples() const {
      return std::span<const Sample>(this->mData);
    }

    [[nodiscard]] float sampleRate() const { return mSampleRate; }

    [[nodiscard]] std::string dataTypeName() const override {
      return Traits::typeName();
    }

  protected:
    float mSampleRate;

    static float validateSampleRate(float rate) {
      if (rate <= 0.0F) {
        SPDLOG_ERROR("Invalid {} sample rate: {} Hz", Traits::typeName(), rate);
        return 200.0F;
      }
      if (rate < Traits::expectedRateMin() || rate > Traits::expectedRateMax()) {
        SPDLOG_WARN("Unusual {} sample rate: {} Hz (expected {}-{} Hz)",
                    Traits::typeName(), rate,
                    Traits::expectedRateMin(), Traits::expectedRateMax());
      }
      return rate;
    }
  };

  // Now GyroFrame becomes trivial:
  class GyroFrame : public SampledSensorFrame<GyroSample, SensorTraits<std::vector<GyroSample>>> {
    using Base = SampledSensorFrame<GyroSample, SensorTraits<std::vector<GyroSample>>>;
    using Base::Base;

    // Only custom helpers go here
    [[nodiscard]] Array<float, 2> asVectorArray() const { /* ... */ }
  };
}
```

**Rationale**:
- Eliminates repeated validation/logging code
- Makes adding new sensor types trivial
- Centralizes type names and expected ranges
- Maintains same public API

---

## Phase 7: Typed Access Helpers (Optional, Doesn't Modify Frame)

### Task 7.1: Create frameDataView<T> helper
**Files**: New file `src/Ravl2/Video/FrameHelpers.hh`

**Approach**:
```cpp
namespace Ravl2::Video {

  // Zero-overhead typed access to frame metadata
  template<typename T>
  [[nodiscard]] std::optional<std::reference_wrapper<const T>>
  frameDataView(const Frame& frame) {
    try {
      const T& data = std::any_cast<const T&>(frame.frameData());
      return std::cref(data);
    } catch (const std::bad_any_cast&) {
      return std::nullopt;
    }
  }

  // Convenience for dynamic_cast pattern
  template<typename FrameT>
  [[nodiscard]] const FrameT* frameCast(const Frame& frame) {
    return dynamic_cast<const FrameT*>(&frame);
  }

  // Usage example:
  void processFrame(const Frame& frame) {
    // Old way (verbose):
    auto* gpsFrame = dynamic_cast<const GoPro::GpsFrame*>(&frame);
    if (gpsFrame) {
      const auto& fix = gpsFrame->data();
      // ...
    }

    // New way (concise):
    if (auto* gpsFrame = frameCast<GoPro::GpsFrame>(frame)) {
      const auto& fix = gpsFrame->data();
      // ...
    }

    // Or with std::any:
    if (auto data = frameDataView<GoPro::GpsFix>(frame)) {
      const auto& fix = data->get();
      // ...
    }
  }
}
```

**Rationale**:
- Doesn't modify `Frame` interface
- Provides safer, more ergonomic access patterns
- Optional - users can still use dynamic_cast or std::any_cast directly

---

## Phase 8: Documentation

### Task 8.1: Expand doxygen comments with examples
**Files**: All GoPro frame headers

**Add to each class**:
```cpp
//! @brief GoPro gyroscope telemetry frame
//!
//! Contains angular velocity measurements from a GoPro camera's gyroscope sensor.
//! Each frame typically contains multiple samples (e.g., 200+ samples at 200 Hz).
//!
//! @par Example Usage:
//! @code
//! auto* gyroFrame = dynamic_cast<GoPro::GyroFrame*>(frame.get());
//! if (gyroFrame) {
//!   // Zero-copy access to samples
//!   std::span<const GyroSample> samples = gyroFrame->samples();
//!
//!   // Process each sample
//!   for (const auto& sample : samples) {
//!     Vector3f angularVel = sample.angularVelocity();
//!     // ... integrate orientation, etc.
//!   }
//!
//!   // Or convert to array for legacy code
//!   Array<float, 2> arr = gyroFrame->asVectorArray();  // Allocates
//! }
//! @endcode
//!
//! @see GyroSample, integrateGyroToOrientation()
class GyroFrame : /*...*/ { };
```

---

## Implementation Order (Recommended)

### Sprint 1: Quick Wins (1-2 days)
1. Task 1.1: Remove `GpmfFrameBase`
2. Task 3.1-3.3: Add validation and logging
3. Task 5.2: Add `lowPassFilter()` bounds check

### Sprint 2: Data Access (2-3 days)
4. Task 2.1-2.2: Add span-based access to Gyro/AccelFrame
5. Task 8.1: Add documentation examples

### Sprint 3: Parser Fixes (3-4 days)
6. Task 4.1: Fix `getScaleFactor()` FourCC handling
7. Task 4.2: Implement proper `getSampleRate()` parsing
8. Task 5.1: Fix small-angle gyro integration

### Sprint 4: Optional Refactoring (3-5 days)
9. Task 6.1: Create metadata traits system
10. Task 7.1: Create typed access helpers

---

## Testing Strategy

### Unit Tests Required
- **GyroFrame/AccelFrame**:
  - Test `samples()` returns correct span
  - Verify zero allocations with `samples()`
  - Test sample rate validation (negative, zero, out-of-range)
  - Verify `asVectorArray()` still works

- **GpmfParser**:
  - Test scale factor extraction with multiple sensors
  - Test sample rate parsing from real GPMF data
  - Test parse failure logging (mock SPDLOG)

- **GpmfUtilities**:
  - Test small-angle quaternion integration (< 1e-6 rad)
  - Test `lowPassFilter()` with alpha out of bounds
  - Test accumulated drift with/without small-angle fix

### Integration Tests
- Test full pipeline: hero8.mp4 → all frames parsed correctly
- Verify GPS/gyro/accel counts match expected (5/5/5 from hero8.mp4)
- Benchmark memory usage before/after span changes

---

## Risk Assessment

| Task | Risk | Mitigation |
|------|------|------------|
| Remove GpmfFrameBase | Low | Empty class, no functionality lost |
| Add span access | Low-Medium | Keep deprecated legacy methods |
| Sample rate validation | Low | Defaults prevent breakage |
| Fix getScaleFactor() | Medium | Need real GPMF test data |
| Fix getSampleRate() | Medium | Complex GPMF spec, needs research |
| Traits system | Medium | Large refactor, optional task |
| Small-angle fix | Low | Mathematical correctness, well-tested formula |

---

## Success Criteria

1. ✅ All existing tests pass
2. ✅ hero8.mp4 still produces 5 GPS + 5 gyro + 5 accel frames
3. ✅ No performance regression (benchmark with/without changes)
4. ✅ Zero allocations when using span-based access
5. ✅ Invalid sample rates logged and handled gracefully
6. ✅ GPMF parse failures logged with context
7. ✅ Small-angle rotations no longer dropped
8. ✅ Documentation includes usage examples

---

## Notes

- Phase 1-5 can proceed immediately
- Phase 6 (traits) is optional polish - defer if time-constrained
- Phase 7 (helpers) requires no changes to existing code, purely additive
- Parser fixes (Phase 4) may require study of GPMF specification and testing with diverse GoPro footage
