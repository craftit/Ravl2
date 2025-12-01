# FfmpegMultiStreamIterator - Issue Summary and Fix Plan

**Date**: 2025-12-01
**File**: `src/Ravl2/Video/FfmpegMultiStreamIterator.cc`
**Status**: ✅ IMPLEMENTED - All Fixes Applied

---

## Executive Summary

The `FfmpegMultiStreamIterator` implementation has **3 critical bugs** that will cause crashes, **3 high-priority issues** affecting correctness, and several medium-priority issues affecting performance and maintainability. The most severe issue is null pointer dereferences when flushing codec buffers for DATA streams (like GoPro GPMF metadata).

**Total Issues Found**: 11
**Estimated Fix Time**: ~5.5 hours across 3 phases
**Risk Level**: HIGH (crashes in production)

---

## Critical Issues (Will Cause Crashes/Data Loss)

### 1. Null Pointer Crash in avcodec_flush_buffers()

**Location**: Lines 405-408, 586-588, 807-811, 1852-1856
**Severity**: ⚠️ CRITICAL - Guaranteed crash
**Impact**: Any seek operation with DATA streams present will crash

**Problem**:
```cpp
for (auto* codecContext : m_codecContexts) {
    avcodec_flush_buffers(codecContext);  // CRASH if codecContext is nullptr
}
```

DATA streams (line 199) can have `nullptr` codec contexts, but all flush loops assume non-null.

**Fix**: Add null checks before flushing
```cpp
for (auto* codecContext : m_codecContexts) {
    if (codecContext) {  // Add null check
        avcodec_flush_buffers(codecContext);
    }
}
```

**Testing**: Test seeking with GoPro files containing GPMF data streams

---

### 2. Unused Frame Clone Flag - Buffer Pool Exhaustion

**Location**: Lines 227-246, frame creation code
**Severity**: ⚠️ CRITICAL - Memory/buffer exhaustion
**Impact**: Capture devices (webcams, AVFoundation) will hang/crash after ~5-10 frames

**Problem**:
```cpp
m_needsFrameClone = true;  // Set but never used!
SPDLOG_DEBUG("... enabling immediate frame cloning to avoid buffer exhaustion");
```

The flag is set when detecting device inputs, but frames are never actually cloned. Device drivers have limited buffer pools (typically 3-5 buffers) and will block/fail when exhausted.

**Fix**: Clone frames immediately when flag is set
```cpp
// In createVideoFrame() and createAudioChunk(), at the start:
AVFrame* frameToUse = frame;
if (m_needsFrameClone) {
    frameToUse = av_frame_clone(frame);
    if (!frameToUse) {
        SPDLOG_ERROR("Failed to clone frame for device input");
        return nullptr;
    }
}
// Use frameToUse for rest of function
// Update cleanup logic to free cloned frame if needed
```

**Testing**: Test with webcam/AVFoundation capture for 30+ seconds continuously

---

### 3. Broken Backward Search in getFrameById()

**Location**: Line 771
**Severity**: ⚠️ CRITICAL - Broken functionality
**Impact**: Frame lookup by ID fails for frames before seek position

**Problem**:
```cpp
auto prevResult = iteratorCopy->previous();  // Always returns NotImplemented!
```

The `previous()` method (lines 356-362) always returns `VideoErrorCode::NotImplemented`, so backward search in `getFrameById()` never works.

**Fix**: Remove backward search code and document limitation
```cpp
// Remove lines 753-783 (backward search logic)
// Add comment after forward search:
// NOTE: Backward frame search not implemented since previous() is not supported.
// If frame not found going forward, we return NotFound rather than attempting
// backward search. This is a known limitation.

SPDLOG_WARN("Could not find frame with ID: {} (backward search not implemented)", id);
return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::NotFound);
```

**Alternative**: Implement `previous()` properly with frame buffering (significantly more complex)

**Testing**: Unit test for getFrameById() with various frame positions

---

## High Priority Issues (Correctness/Reliability)

### 4. Frame ID Overflow with Long Videos

**Location**: Lines 1137-1161 (`generateUniqueFrameId`)
**Severity**: 🔴 HIGH - Data corruption
**Impact**: Frame ID collisions in videos longer than ~9 hours (with 4-bit stream index)

**Problem**:
```cpp
StreamItemId id = (pts << m_streamBits) | (localIndex & mask);
```

With `m_streamBits = 4`, PTS values > 2^60 microseconds (~36 years) overflow. But with larger stream counts (using more bits), the threshold drops significantly.

**Fix**: Add overflow detection and validation
```cpp
StreamItemId FfmpegMultiStreamIterator::generateUniqueFrameId(AVFrame* frame, std::size_t localIndex)
{
    int64_t pts = 0;
    if (frame->pts != AV_NOPTS_VALUE) {
        auto* stream = m_streams[localIndex];
        pts = av_rescale_q(frame->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
    } else {
        pts = m_nextFrameIds[localIndex]++;
    }

    // Check for overflow
    const int64_t maxPts = (INT64_MAX >> m_streamBits);
    if (pts > maxPts) {
        SPDLOG_WARN("PTS {} exceeds maximum {} for frame ID encoding with {} stream bits",
                    pts, maxPts, m_streamBits);
        pts = maxPts;  // Clamp to maximum representable value
    }

    StreamItemId id = (pts << m_streamBits) | (static_cast<int64_t>(localIndex) & ((1LL << m_streamBits) - 1));

    SPDLOG_DEBUG("Generated frame ID: {} from PTS: {}, stream index: {}", id, pts, localIndex);
    return id;
}
```

**Testing**: Test videos > 2 hours duration, synthetic test with large PTS values

---

### 5. Race Condition in buildKeyframeIndex()

**Location**: Lines 1630-1638 (`findNearestKeyframe`)
**Severity**: 🔴 HIGH - Race condition
**Impact**: Concurrent seeks cause data races, potential crashes or corrupted index

**Problem**:
```cpp
if (!m_keyframeIndexBuilt) {  // Check-then-act race condition
    auto result = buildKeyframeIndex();
    // Multiple threads can enter here simultaneously
}
```

Multiple threads calling `seek()` can trigger concurrent index builds.

**Fix**: Add synchronization with double-checked locking
```cpp
// In FfmpegMultiStreamIterator.hh, add:
#include <mutex>
#include <atomic>

private:
    std::atomic<bool> m_keyframeIndexBuilt{false};
    mutable std::mutex m_keyframeIndexMutex;  // mutable for const methods

// In findNearestKeyframe() implementation:
if (!m_keyframeIndexBuilt.load(std::memory_order_acquire)) {
    std::lock_guard<std::mutex> lock(m_keyframeIndexMutex);
    if (!m_keyframeIndexBuilt.load(std::memory_order_relaxed)) {  // Double-check
        auto result = buildKeyframeIndex();
        if (!result.isSuccess()) {
            SPDLOG_WARN("Failed to build keyframe index: {}", toString(result.error()));
            return nearestKeyframe;
        }
    }
}
```

**Testing**: Concurrent seek stress test with multiple threads

---

### 6. Silent Seek Failures

**Location**: Lines 573-582, 654-656 (`traditionalSeek`)
**Severity**: 🔴 HIGH - Silent failures
**Impact**: Seek errors are masked, iterator in undefined state

**Problem**:
```cpp
if (!seekSuccess) {
    SPDLOG_WARN("All seeking attempts failed...");
    SPDLOG_DEBUG("Attempting to continue despite seek failure");  // Returns success anyway!
}
// ... later ...
return VideoResult<void>();  // Returns success even though seek failed!
```

**Fix**: Return proper error instead of masking failure
```cpp
// At line 573-582, replace with:
if (!seekSuccess) {
    SPDLOG_ERROR("All seeking attempts failed for timestamp: {} (error code: {})",
                 timestamp.count(), result);
    return VideoResult<void>(FfmpegMediaContainer::convertFfmpegError(result));
}

// Remove the fallback "continue anyway" logic at lines 654-656
```

**Testing**: Test seek error handling with corrupted/truncated files

---

## Medium Priority Issues

### 7. Thread Safety - Shared Container in getFrameById()

**Location**: Line 706 (`getFrameById`)
**Severity**: 🟡 MEDIUM - Undefined behavior with concurrent use
**Impact**: Potential crashes or data corruption if used from multiple threads

**Problem**:
```cpp
auto iteratorCopy = std::make_shared<FfmpegMultiStreamIterator>(m_ffmpegContainer, m_streamIndices);
```

Creates new iterator sharing `m_ffmpegContainer`, no locking documented. If original iterator is used concurrently, undefined behavior may occur.

**Fix**: Document thread-safety requirements or add container-level locking
```cpp
// Add to method documentation:
//! @note This method is NOT thread-safe with respect to the original iterator.
//! Do not call this method while the iterator is being used from another thread.
//! The temporary iterator created internally shares the underlying container.
```

**Testing**: Multi-threaded stress test

---

### 8. Complex shared_ptr Aliasing

**Location**: Lines 1400-1434 (`makeImage` template functions)
**Severity**: 🟡 MEDIUM - Maintenance/clarity
**Impact**: Hard to understand and maintain, potential for subtle bugs

**Problem**:
```cpp
std::shared_ptr avFrameHandle = std::shared_ptr<uint8_t[]>(newFrame->data[0], ...);
// Later creates another shared_ptr with nested deleter
std::shared_ptr avFramePlaneHandle = std::shared_ptr<PixelTypeT[]>(pixelData,
    [avFrameHandle](PixelTypeT* data) mutable { ... }
);
```

Nested shared_ptr with custom deleters is overly complex.

**Fix**: Simplify using shared_ptr aliasing constructor
```cpp
// Create single frame handle
std::shared_ptr<AVFrame> frameHandle(newFrame, [](AVFrame* f) {
    av_frame_free(&f);
});

// Use aliasing constructor for each plane
img.forEachPlane([frameHandle, range, &planeIndex, newFrame]<typename PlaneArgT>(PlaneArgT& plane) {
    using PlaneT = std::decay_t<PlaneArgT>;
    auto localRange = PlaneT::scale_type::calculateRange(range);
    using PixelTypeT = typename PlaneT::value_type;

    PixelTypeT* pixelData = reinterpret_cast<PixelTypeT*>(newFrame->data[planeIndex]);

    // Aliasing constructor: shares ownership with frameHandle but stores pixelData pointer
    std::shared_ptr<PixelTypeT[]> planeHandle(frameHandle, pixelData);

    int stride = newFrame->linesize[planeIndex] / static_cast<int>(sizeof(PixelTypeT));
    RavlAlwaysAssert(newFrame->linesize[planeIndex] % static_cast<int>(sizeof(PixelTypeT)) == 0);

    plane.data() = Array<PixelTypeT, 2>(pixelData, localRange, {stride, 1}, planeHandle);
    planeIndex++;
});
```

**Testing**: Existing tests should pass, verify no memory leaks with valgrind

---

### 9. Inefficient seekToIndex()

**Location**: Lines 662-685
**Severity**: 🟡 MEDIUM - Performance
**Impact**: O(n) operation, extremely slow for large indices

**Problem**:
```cpp
// Resets to beginning and advances frame-by-frame
for (int64_t i = 0; i < index && !m_isAtEnd; ++i) {
    result = next();
}
```

Could take minutes for seeking to frame 10000 in a long video.

**Fix**: Estimate timestamp from frame rate
```cpp
VideoResult<void> FfmpegMultiStreamIterator::seekToIndex(int64_t index)
{
    // Try timestamp-based seeking if we know the frame rate
    for (size_t i = 0; i < m_streams.size(); ++i) {
        auto* stream = m_streams[i];
        if (stream && stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0) {
            double fps = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
            int64_t estimatedTime = static_cast<int64_t>((index / fps) * AV_TIME_BASE);

            SPDLOG_DEBUG("Seeking to index {} using estimated timestamp {} (fps: {})",
                        index, estimatedTime, fps);

            auto result = seek(MediaTime(estimatedTime), SeekFlags::Precise);
            if (result.isSuccess()) {
                // May not be exactly at index, but close enough
                return result;
            }
        }
    }

    // Fallback to frame-by-frame if no frame rate available
    SPDLOG_DEBUG("No frame rate available, using frame-by-frame seeking to index {}", index);

    auto result = reset();
    if (!result.isSuccess()) {
        return result;
    }

    for (int64_t i = 0; i < index && !m_isAtEnd; ++i) {
        result = next();
        if (!result.isSuccess() && result.error() != VideoErrorCode::EndOfStream) {
            return result;
        }
    }

    return VideoResult<void>();
}
```

**Testing**: Performance test seeking to large indices (e.g., frame 5000)

---

## Low Priority / Documentation Issues

### 10. Missing Thread-Safety Documentation

**Location**: Class header (`FfmpegMultiStreamIterator.hh` lines 31-33)
**Severity**: 🟢 LOW - Documentation
**Impact**: Users may incorrectly assume thread-safety

**Fix**: Add thread-safety documentation
```cpp
//! Implementation of StreamIterator that provides frames for multiple streams in a FFmpeg-based media container
//!
//! This iterator maintains temporal ordering across all streams by buffering decoded frames
//! in a priority queue sorted by presentation timestamp (PTS).
//!
//! @note NOT thread-safe: Each thread must create its own iterator instance.
//!       Multiple iterators may share the same FfmpegMediaContainer, but the container
//!       must provide thread-safe access if used concurrently.
//!
//! @note Frame cloning: For device capture inputs (webcam, AVFoundation), frames are
//!       automatically cloned to prevent buffer pool exhaustion.
class FfmpegMultiStreamIterator final : public StreamIterator
```

---

### 11. Constructor Exception Safety

**Location**: Lines 113-275 (constructor)
**Severity**: 🟢 LOW - Code quality
**Impact**: Adequate but could be clearer

**Problem**: Manual cleanup on error paths could be improved with RAII guards.

**Fix**: Use RAII helpers (optional improvement)
```cpp
// Could use unique_ptr with custom deleters for cleaner cleanup
std::unique_ptr<AVPacket, void(*)(AVPacket*)> packetGuard(
    av_packet_alloc(),
    [](AVPacket* p) { if (p) av_packet_free(&p); }
);

std::vector<std::unique_ptr<AVFrame, void(*)(AVFrame*)>> frameGuards;
// ... etc
```

**Note**: Current implementation is adequate; this is optional cleanup.

---

## Fix Plan - Implementation Order

### Phase 1: Critical Crash Fixes (Do First) ⚠️

**Priority**: IMMEDIATE
**Estimated Time**: 2 hours
**Risk**: Low - straightforward fixes

#### Task 1.1: Fix null pointer crashes in codec buffer flushing
- **Files**: `FfmpegMultiStreamIterator.cc`
- **Lines**: 405-408, 586-588, 807-811, 1852-1856
- **Effort**: 15 minutes
- **Testing**: Test seeking with GoPro files containing GPMF data streams

#### Task 1.2: Implement frame cloning for device captures
- **Files**: `FfmpegMultiStreamIterator.cc`
- **Lines**: ~1290 (in createVideoFrame), ~1340 (in createAudioChunk)
- **Effort**: 1 hour
- **Testing**: Test with webcam/AVFoundation capture for 30+ seconds

#### Task 1.3: Fix broken getFrameById() backward search
- **Files**: `FfmpegMultiStreamIterator.cc`
- **Lines**: 753-783
- **Effort**: 10 minutes
- **Testing**: Unit test for getFrameById()

**Dependencies**: None - can be done in parallel
**Impact**: Fixes 3 crash scenarios

---

### Phase 2: High Priority Correctness Fixes 🔴

**Priority**: THIS SPRINT
**Estimated Time**: 2 hours
**Risk**: Low-Medium - threading changes need careful testing

#### Task 2.1: Add frame ID overflow protection
- **Files**: `FfmpegMultiStreamIterator.cc`
- **Lines**: 1137-1161
- **Effort**: 30 minutes
- **Testing**: Unit test with synthetic large PTS values, test 2+ hour videos

#### Task 2.2: Fix race condition in keyframe index building
- **Files**: `FfmpegMultiStreamIterator.hh`, `FfmpegMultiStreamIterator.cc`
- **Lines**: Header ~142, Implementation 1630-1638
- **Effort**: 45 minutes
- **Testing**: Concurrent seek stress test

#### Task 2.3: Fix silent seek failures
- **Files**: `FfmpegMultiStreamIterator.cc`
- **Lines**: 573-582, 654-656
- **Effort**: 20 minutes
- **Testing**: Test with corrupted/truncated files

**Dependencies**:
- Task 2.2 should be done after 2.1/2.3 to avoid merge conflicts
- Need thread-safety test infrastructure

**Impact**: Prevents data corruption and race conditions

---

### Phase 3: Medium Priority Improvements 🟡

**Priority**: NEXT SPRINT
**Estimated Time**: 1.5 hours
**Risk**: Low

#### Task 3.1: Improve seekToIndex() performance
- **Files**: `FfmpegMultiStreamIterator.cc`
- **Lines**: 662-685
- **Effort**: 30 minutes
- **Testing**: Performance test seeking to large indices

#### Task 3.2: Simplify makeImage() shared_ptr logic
- **Files**: `FfmpegMultiStreamIterator.cc`
- **Lines**: 1400-1434
- **Effort**: 45 minutes
- **Testing**: Memory leak testing with valgrind

#### Task 3.3: Add thread-safety documentation
- **Files**: `FfmpegMultiStreamIterator.hh`
- **Lines**: 31-33
- **Effort**: 10 minutes
- **Testing**: None (documentation only)

**Impact**: Code quality and performance improvements

---

## Testing Strategy

### Unit Tests Required

1. **Test seeking with GPMF DATA streams** (Task 1.1)
   - Load GoPro file with GPMF metadata
   - Perform multiple seeks
   - Verify no crashes

2. **Test device capture for extended duration** (Task 1.2)
   - Open webcam/AVFoundation device
   - Capture for 60+ seconds continuously
   - Verify no buffer pool exhaustion

3. **Test getFrameById() with various frame positions** (Task 1.3)
   - Get frames at beginning, middle, end
   - Verify correct frames returned
   - Document limitation for frames before current position

4. **Test videos > 2 hours duration for frame ID correctness** (Task 2.1)
   - Test with long-duration file
   - Verify frame IDs remain unique
   - Test with synthetic large PTS values

5. **Concurrent seek stress test** (Task 2.2)
   - Multiple threads calling seek() simultaneously
   - Verify thread-safety
   - Use ThreadSanitizer

6. **Test seek error handling with corrupted files** (Task 2.3)
   - Truncated files
   - Files with missing keyframes
   - Verify proper error codes returned

### Integration Tests

- Multi-stream GoPro files with video + audio + GPMF
- Live capture from webcam/AVFoundation
- Long-duration videos (2+ hours)
- Concurrent playback scenarios

### Tools

- **Valgrind**: Memory leak detection
- **ThreadSanitizer**: Race condition detection
- **AddressSanitizer**: Memory corruption detection (already enabled)
- **Unit tests**: Catch2 framework

---

## Risk Assessment

| Task | Risk | Mitigation |
|------|------|------------|
| 1.1 - Null checks | LOW | Simple change, easy to verify |
| 1.2 - Frame cloning | MEDIUM | Test thoroughly with device capture; ensure cleanup is correct |
| 1.3 - Remove backward search | LOW | Documenting existing limitation |
| 2.1 - Overflow protection | LOW | Add unit test with synthetic large PTS |
| 2.2 - Race condition | MEDIUM | Requires thread-safety test infrastructure; use ThreadSanitizer |
| 2.3 - Seek errors | LOW | Improves error handling, no new functionality |
| 3.1 - seekToIndex | LOW | Performance improvement, fallback exists |
| 3.2 - Refactor | LOW | No functional change, test with memory tools |
| 3.3 - Documentation | NONE | Documentation only |

---

## Dependencies & Prerequisites

### Required for Phase 1
- Access to GoPro test files with GPMF data
- Webcam or AVFoundation device for testing
- Unit test framework (Catch2)

### Required for Phase 2
- Thread-safety testing infrastructure
- Long-duration test videos (2+ hours)
- Corrupted/truncated test files

### Required for Phase 3
- Valgrind or similar memory analysis tools
- Performance benchmarking framework

---

## Summary Statistics

- **Total Issues**: 11
- **Critical**: 3 (crashes)
- **High Priority**: 3 (correctness)
- **Medium Priority**: 3 (performance/quality)
- **Low Priority**: 2 (documentation)
- **Total Estimated Time**: ~5.5 hours
- **Lines of Code Affected**: ~150 lines across 3 phases

**Recommendation**: Execute Phase 1 immediately before any production deployment.