# GPMF Parser and Frame Buffering Analysis

**Date**: 2025-12-01
**Status**: Issues Identified - Recommendations Provided

---

## Executive Summary

Analysis of `GpmfParser` reveals **interface inconsistencies** and potential **buffering issues** when GPMF metadata streams are present. The current queue sizing algorithm does not account for GPMF frame generation patterns.

---

## Interface Consistency Issues

### Issue 1: Inconsistent Frame Generation Between Sensor Types

**GPS Handling** (`parseGps()` - GpmfParser.cc:117-221):
- **Creates multiple frames** - one per GPS sample
- **Interpolates timestamps** for each sample
- A 1-second packet with 18 GPS samples → **18 separate frames** with timestamps spread across the second
```cpp
// Lines 208-211
Video::MediaTime fixTimestamp = timestamp;
if(timeDelta.count() > 0) {
  fixTimestamp = timestamp + Video::MediaTime(timeDelta.count() * static_cast<int64_t>(i));
}
```

**Gyro/Accel Handling** (`parseGyro()`/`parseAccel()` - GpmfParser.cc:223-333):
- **Creates ONE frame** containing ALL samples
- **NO timestamp interpolation** - all samples get the packet timestamp
- A 1-second packet with 200 gyro samples → **1 frame** containing 200 samples
```cpp
// Lines 273-276
frames.push_back(std::make_shared<Video::MetaDataFrame<GyroSamples>>(
  GyroSamples(samples, sampleRate),
  streamId + mNextId++,
  timestamp));  // <-- ALL samples share packet timestamp
```

**Recommendation**: **This inconsistency is actually CORRECT** for the different data types:
- GPS fixes are **sparse, independent measurements** → separate frames make sense
- Gyro/Accel are **high-frequency continuous signals** → bulk frames make sense

However, this should be **documented** in the header to explain the design rationale.

---

## Frame Generation Patterns

### Typical GoPro Hero 8+ Video (30fps)

| Stream Type | Packet Rate | Samples/Packet | Frames Generated/Packet | Total Frames/Sec |
|-------------|-------------|----------------|------------------------|------------------|
| **Video**   | 30/sec      | 1              | 1                      | 30               |
| **Audio**   | variable    | ~1024          | 1                      | ~46              |
| **GPS5**    | 1/sec       | 18             | **18**                 | **18**           |
| **GYRO**    | 1/sec       | 200            | **1**                  | **1**            |
| **ACCL**    | 1/sec       | 200            | **1**                  | **1**            |
| **TOTAL**   |             |                |                        | **~96 frames/sec** |

### Timestamp Distribution

For a 1-second GPMF packet at PTS = 1,000,000μs:

**GPS Frames** (18 frames, ~55ms apart):
```
GPS[0]:  PTS = 1,000,000μs
GPS[1]:  PTS = 1,055,556μs
GPS[2]:  PTS = 1,111,111μs
...
GPS[17]: PTS = 1,944,444μs
```

**Gyro/Accel Frames** (1 frame each):
```
GYRO[0-199]: PTS = 1,000,000μs (all 200 samples)
ACCL[0-199]: PTS = 1,000,000μs (all 200 samples)
```

**Video Frames** (30 frames, ~33ms apart):
```
Video[0]:  PTS = 1,000,000μs
Video[1]:  PTS = 1,033,333μs
Video[2]:  PTS = 1,066,667μs
...
Video[29]: PTS = 1,966,667μs
```

---

## Queue Buffering Implications

### Current Queue Size Calculation

**Location**: `FfmpegMultiStreamIterator::calculateQueueSize()` (lines 285-349)

**Current Behavior**:
```cpp
// ONLY examines VIDEO streams
for (size_t i = 0; i < m_streamIndices.size(); ++i)
{
  auto* stream = m_streams[i];
  if (!stream || stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
    continue;  // <-- SKIPS audio, data, subtitle streams
```

**Result**:
- Queue size is calculated based solely on video B-frame reordering (typically 32-128 frames)
- **DOES NOT** account for GPMF frame generation
- **DOES NOT** account for audio buffering

### Potential Issues

#### Issue 2: Insufficient Queue Size for Multi-Stream with GPMF

**Scenario**: Video at 30fps + GPMF with GPS (18 frames/sec)
- Video B-frames need: ~16-64 frames (depending on max_b_frames)
- GPS frames generated per second: 18 frames
- With 1 second of GPMF buffering: 18 GPS + 30 video = 48 frames minimum

**Current queue size**: 32-128 frames (line 346)

**Analysis**:
- With the current 32 frame minimum, we might not have enough space for:
  - B-frame reordering (16 frames)
  - 1 second of GPS data (18 frames)
  - Safety margin
- This could cause timestamp ordering issues if frames arrive in bursts

#### Issue 3: GPMF Packet Arrival Pattern

GPMF packets arrive **once per second** and generate **18 GPS frames** (for GPS5) that need to be interleaved with video frames.

**Decoding pattern**:
```
t=0s:    Decode video packets → queue grows to ~30 frames
t=1s:    Decode GPMF packet  → queue gets +18 GPS frames (+2 gyro/accel)
         Queue now has: 30 video + 18 GPS + 2 IMU = 50 frames
t=1.03s: Pop frames in PTS order (video + GPS interleaved)
```

**Problem**: If `m_minQueueSize = 32`, and we try to buffer for proper ordering:
- Video frames (B-frame reordering): need 16-32 frames
- GPS frames (1 second worth): need 18 frames
- **Total needed**: ~50 frames minimum

**Current minimum**: 32 frames ❌ **TOO SMALL**

---

## Recommendations

### Recommendation 1: Document Frame Generation Strategy ✅ LOW PRIORITY

Add documentation to `GpmfParser.hh` explaining why different sensor types use different frame generation strategies:

```cpp
//! Parse GPS data from GPMF stream and append frames
//! @note GPS data is parsed into INDIVIDUAL frames (one per sample) with interpolated
//!       timestamps because GPS fixes are sparse, independent measurements.
```

```cpp
//! Parse gyroscope data from GPMF stream and append frame
//! @note Gyro data is parsed into a SINGLE frame containing ALL samples with the packet
//!       timestamp, because gyro is a high-frequency continuous signal where individual
//!       sample timestamps are less meaningful. The sample rate is stored in the frame.
```

### Recommendation 2: Enhance Queue Size Calculation 🔧 MEDIUM PRIORITY

**Modify** `calculateQueueSize()` to account for GPMF streams:

```cpp
void FfmpegMultiStreamIterator::calculateQueueSize()
{
  m_minQueueSize = 32;

  // Check video streams for B-frame reordering
  for (size_t i = 0; i < m_streamIndices.size(); ++i)
  {
    auto* stream = m_streams[i];
    if (!stream) continue;

    // Video: B-frame reordering
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      // ... existing B-frame logic ...
    }
    // Data streams: Check for GPMF metadata
    else if (stream->codecpar->codec_type == AVMEDIA_TYPE_DATA)
    {
      // GPMF packets arrive at ~1 Hz and may generate multiple frames
      // GPS5: ~18 frames/packet, GYRO: 1 frame/packet, ACCL: 1 frame/packet
      // Add buffer for at least 1 second of GPMF data
      std::size_t gpmfBuffer = 20;  // ~18 GPS + 1 GYRO + 1 ACCL
      m_minQueueSize = std::max(m_minQueueSize,
                                static_cast<std::size_t>(m_minQueueSize + gpmfBuffer));
      SPDLOG_DEBUG("Data stream {} detected (likely GPMF), adding buffer: {}",
                   i, gpmfBuffer);
    }
  }

  // Multi-stream safety margin
  if (m_streamIndices.size() > 1)
  {
    m_minQueueSize += 32;
  }

  // Cap at reasonable maximum
  m_minQueueSize = std::min(m_minQueueSize, std::size_t(128));

  SPDLOG_INFO("Set packet queue size to: {}", m_minQueueSize);
}
```

**Impact**:
- Ensures sufficient buffering for GPMF + video interleaving
- Prevents timestamp ordering issues
- Minimal memory overhead (~320KB-1.6MB additional for 4K video)

### Recommendation 3: Test with GoPro Footage 🧪 HIGH PRIORITY

**Action**: Run tests with actual GoPro footage containing GPMF metadata to verify:
1. Timestamp ordering is correct (no non-monotonic warnings)
2. GPS frames are properly interleaved with video frames
3. Queue size is sufficient (monitor queue depth during playback)

**Test Command**:
```bash
SPDLOG_LEVEL=debug ./test/tests "FfmpegMultiStreamIterator - GoPro*"
```

Look for:
- ❌ "Non-monotonic PTS detected" warnings
- ❌ Queue emptying prematurely
- ✅ Smooth interleaving of video and metadata frames

---

## Alternative Approaches Considered

### Option A: Make GPS Frame Generation Match Gyro/Accel ❌

Create one GPS frame per packet containing all fixes, like gyro/accel.

**Rejected**: GPS fixes are independent measurements, not continuous signals. Separate frames provide better:
- Timestamp accuracy
- Spatial/temporal filtering
- Integration with mapping APIs

### Option B: Make Gyro/Accel Frame Generation Match GPS ❌

Create individual frames for each gyro/accel sample with interpolated timestamps.

**Rejected**: Would generate **200 frames/sec per sensor** (400 total for gyro+accel), overwhelming the queue and providing no benefit since these are continuous signals.

### Option C: Dynamic Queue Sizing Based on Content ⚠️ FUTURE CONSIDERATION

Dynamically adjust queue size based on observed frame generation rates.

**Deferred**: More complex, adds runtime overhead. Current approach with static calculation is simpler and sufficient if we account for GPMF.

---

## Current Status

### What Works ✅
- GPS timestamp interpolation is correct
- Frame timestamp ordering via priority queue is correct
- B-frame reordering is handled correctly

### What Needs Attention ⚠️
- Queue size calculation doesn't account for GPMF streams
- Lack of documentation explaining frame generation strategy differences
- No specific tests for GPMF + video interleaving

### Risk Assessment

**Current Risk**: **LOW-MEDIUM**
- Current queue minimum (32) might be insufficient for video with GPMF
- Multi-stream buffer addition (+32 at line 339) *might* compensate by accident
- Likely works in practice but not by design

**Mitigation**: Implement Recommendation 2 (enhance queue size calculation)

---

## Testing Checklist

- [ ] Test with GoPro Hero 8+ footage (GPS5 + GYRO + ACCL + video)
- [ ] Verify no non-monotonic PTS warnings
- [ ] Check queue depth doesn't drop to zero during playback
- [ ] Test seeking with GPMF streams
- [ ] Verify GPS frame timestamps are properly interpolated
- [ ] Verify gyro/accel bulk frames have correct sample counts

---

## Related Documents

- `TIMESTAMP_ORDERING_ISSUES.md` - Timestamp ordering fixes (completed)
- `MEMORY_EFFICIENT_ORDERING.md` - Queue sizing analysis (completed)
- `FFMPEG_MULTISTREAM_FIXES.md` - General fixes (completed)

---

## Conclusion

The GPMF parser interface is **functionally correct but inconsistent**. The different frame generation strategies (multiple frames for GPS vs single frames for gyro/accel) are appropriate for the data types but should be documented.

The more pressing issue is that **queue size calculation doesn't account for GPMF frame generation**, which could cause problems with proper interleaving. Recommendation 2 should be implemented to ensure robust operation with GoPro footage.
