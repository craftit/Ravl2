# FfmpegMultiStreamIterator Packet Ordering - Current Status

**Date**: 2025-11-29
**Branch**: `add_gopro`
**Working Directory**: `/home/charles/src/Ravl2/cmake-build-debug`

## Executive Summary

Investigating critical packet handling issues in `FfmpegMultiStreamIterator` where:
1. Priority queue built but never consumed (CONFIRMED BUG)
2. GPMF frames double-buffered causing orphaned data (CONFIRMED BUG)
3. Frame ID generation double-increment (FIXED)
4. Timestamp validation issues (FIXED)

**Current State**: Phases 2, 4, 5 completed and working. Phase 1 (priority queue consumption) has ordering bug preventing test passage.

---

## Detailed Plan Location

Full implementation plan with all 6 phases: `/home/charles/.claude/plans/curious-tinkering-duck.md`

---

## Completed Work

### ✅ Phase 4: Frame ID Generation Fix
**File**: `src/Ravl2/Video/FfmpegMultiStreamIterator.cc`
**Lines**: 1082-1084 (removed double-increment)

**Change**:
```cpp
// REMOVED this code that was incrementing twice:
// if (frame->pts == AV_NOPTS_VALUE) {
//   m_nextFrameIds[localIndex]++;  // First increment
// }

// Now only increments in generateUniqueFrameId() at line 1118
StreamItemId id = generateUniqueFrameId(frame, localIndex);
```

**Status**: ✅ Working, tests pass

---

### ✅ Phase 5: Timestamp Check Fix
**File**: `src/Ravl2/Video/FfmpegMultiStreamIterator.cc`
**Lines**: 1503-1538 in `fillPacketQueue()`

**Change**: Reordered timestamp priority to use FFmpeg best practices:
```cpp
// Priority: packet PTS > packet DTS > frame timestamp
if (m_packet->pts != AV_NOPTS_VALUE) {
  pts = av_rescale_q(m_packet->pts, stream->time_base, AVRational{1, AV_TIME_BASE});
}
else if (m_packet->dts != AV_NOPTS_VALUE) {
  pts = av_rescale_q(m_packet->dts, stream->time_base, AVRational{1, AV_TIME_BASE});
}
else if (frame && frame->timestamp().count() != 0) {
  pts = frame->timestamp().count();
}
```

**Status**: ✅ Working, proper timestamp handling

---

### ✅ Phase 2: GPMF Double-Buffering Removal
**Files**:
- `src/Ravl2/Video/FfmpegMultiStreamIterator.hh` (lines 215-219)
- `src/Ravl2/Video/FfmpegMultiStreamIterator.cc` (multiple locations)

**Changes**:

1. **Removed `m_gpmfFrameBuffer` member variable** (was at line 218 in .hh)
   - Eliminated double-buffering where GPMF frames could be in both buffer and queue

2. **Changed `decodePacket()` return type**:
   ```cpp
   // OLD:
   VideoResult<std::shared_ptr<Frame>> decodePacket(AVPacket* packet, std::size_t streamIndex);

   // NEW:
   VideoResult<std::vector<std::shared_ptr<Frame>>> decodePacket(AVPacket* packet, std::size_t streamIndex);
   ```

3. **GPMF parsing now returns all frames directly** (lines 1024-1034):
   ```cpp
   auto frames = m_gpmfParser->parse(packet->data, size, streamId, timestamp);
   // Returns GPS + Gyro + Accel frames together
   return VideoResult<std::vector<std::shared_ptr<Frame>>>(frames);
   ```

4. **Updated `fillPacketQueue()` to handle multiple frames** (lines 1439-1530):
   ```cpp
   auto frameResult = decodePacket(m_packet, localIndex);
   if (frameResult.isSuccess()) {
     for (const auto& frame : frameResult.value()) {
       // Calculate PTS and add each frame to priority queue
       PacketInfo info{frame, localIndex, pts};
       m_packetQueue.push(info);
     }
   }
   ```

5. **Removed GPMF buffer draining code**:
   - Lines 237-250 (EOF draining in old `next()`)
   - Lines 1447-1467 (EOF draining in `fillPacketQueue()`)

**Status**: ✅ Working for GPMF parsing
- Verified with `./examples/exGoProMetadata ./_deps/gpmf-parser-src/samples/hero8.mp4 -m 200`
- Returns 2 GPS + 2 Gyro + 2 Accel frames correctly

---

## ⚠️ In Progress - Phase 1: Priority Queue Consumption

### The Problem

**Original Architecture** (commit `713bc8c`):
```cpp
VideoResult<void> FfmpegMultiStreamIterator::next() {
  while (true) {
    int result = av_read_frame(...);  // Read packet sequentially
    auto frameResult = decodePacket(m_packet, localIndex);
    if (frameResult.isSuccess()) {
      setCurrentFrame(frameResult.value());  // Return frame immediately
      return VideoResult<void>();
    }
  }
}
```
- ✅ **PASSES** presentation order test with `sample-5s.mp4`
- ❌ **BUG**: Priority queue (`m_packetQueue`) built in `fillPacketQueue()` but **NEVER CONSUMED**
- ❌ **BUG**: Frames returned in **FILE ORDER**, not **PRESENTATION ORDER**
- ✅ **WORKS for files without B-frames** (decode order = presentation order)

**New Architecture** (current failing code):
```cpp
VideoResult<void> FfmpegMultiStreamIterator::next() {
  // Ensure queue has MIN_QUEUE_SIZE frames
  if (m_packetQueue.empty() || (!m_isAtEnd && m_packetQueue.size() < MIN_QUEUE_SIZE)) {
    fillPacketQueue();  // Fill to MIN_QUEUE_SIZE
  }

  // Pop next frame from priority queue (min-heap by PTS)
  PacketInfo nextPacket = m_packetQueue.top();
  m_packetQueue.pop();
  setCurrentFrame(nextPacket.frame);
  return VideoResult<void>();
}
```
- ❌ **FAILS** presentation order test
- ✅ **CORRECT**: Uses priority queue for temporal ordering
- ❌ **BUG**: Frames coming out in wrong order

### Test Failure Pattern

**Test**: `FfmpegMultiStreamIterator - Frame Presentation Order`
**File**: `test/test_VideoIO.cc` lines 58-115
**Expectation**: `currentPts >= previousPts` (monotonically increasing)

**Failures** (showing every ~3rd frame goes backwards):
```
FAILED: CHECK( currentPts >= previousPts )
  0 us >= 33333 us          ← Frame with PTS=0 came AFTER frame with PTS=33333

FAILED: CHECK( currentPts >= previousPts )
  133333 us >= 166667 us    ← Went backwards by 33334 us

FAILED: CHECK( currentPts >= previousPts )
  100000 us >= 200000 us    ← Went backwards by 100000 us

FAILED: CHECK( currentPts >= previousPts )
  266667 us >= 300000 us    ← Pattern repeats...
```

**Pattern Analysis**:
- Not ALL frames are out of order, only some
- Suggests frames added to queue mid-iteration have lower PTS than consumed frames
- OR priority queue comparison is incorrect (but verified correct)

### Priority Queue Implementation

**Header** (`FfmpegMultiStreamIterator.hh` lines 186-210):
```cpp
struct PacketInfo {
  std::shared_ptr<Frame> frame;
  std::size_t streamIndex;
  int64_t pts;
};

struct PacketInfoComparator {
  bool operator()(const PacketInfo& a, const PacketInfo& b) const {
    // Return true if a should come AFTER b (lower priority)
    // For min-heap: larger PTS = lower priority = comes later
    return a.pts > b.pts;
  }
};

std::priority_queue<PacketInfo, std::vector<PacketInfo>, PacketInfoComparator> m_packetQueue;
static constexpr std::size_t MIN_QUEUE_SIZE = 32;
```

**Verification of Comparison Logic**:
- `std::priority_queue` is a max-heap by default (largest on top)
- Comparator returns `true` if `a` should have **lower priority** than `b`
- `a.pts > b.pts` means `a` has higher timestamp, so lower priority (should come later)
- This creates a **min-heap** (smallest PTS on top) ✅ CORRECT

**Manual Test**:
- Frame A: pts=100, Frame B: pts=200
- `comparator(A, B)` = `100 > 200` = `false` → A not lower priority than B
- `comparator(B, A)` = `200 > 100` = `true` → B has lower priority than A
- Result: A on top ✅ CORRECT (smallest PTS first)

---

## Critical Questions to Investigate

### 1. Why does original code pass the test?

**Answer**: `sample-5s.mp4` has no B-frames. Packets are already in presentation order in the file.

**Verified by checking original code** (commit `713bc8c`):
```bash
git show 713bc8c:src/Ravl2/Video/FfmpegMultiStreamIterator.cc | grep -A 100 "VideoResult<void> FfmpegMultiStreamIterator::next()"
```
- Original `next()` reads packets sequentially
- Returns frames immediately after decode
- ✅ Passes all tests (43 assertions)

### 2. Is the priority queue ever consumed in original code?

**Answer**: NO

**Evidence**:
```bash
git show 713bc8c:src/Ravl2/Video/FfmpegMultiStreamIterator.cc | grep -c "m_packetQueue.top()"
# Output: 0
```

`fillPacketQueue()` exists and populates the queue, but it's **NEVER consumed**. This confirms the bug identified in the plan.

### 3. Why is new code failing?

**Hypotheses**:

**A. Refill Logic Issue** (MOST LIKELY):
- `next()` pops from queue, reducing size from 32 to 31
- Condition triggers: `m_packetQueue.size() < MIN_QUEUE_SIZE`
- `fillPacketQueue()` is called to refill
- `fillPacketQueue()` loops: `while (m_packetQueue.size() < MIN_QUEUE_SIZE)`
- Reads 1 more packet, decodes it
- **PROBLEM**: What if that packet's frames have PTS LOWER than frames already in queue?

**Example Scenario**:
1. Queue initially has frames with PTS: [100, 200, 300, ..., 3200] (32 frames)
2. Pop frame with PTS=100, queue now has 31 frames
3. Refill: Read next packet from file
4. Decode packet → gets frame with PTS=150 (due to B-frame reordering)
5. Add PTS=150 to queue
6. Queue now has: [150, 200, 300, ...]
7. Next pop returns PTS=150
8. **BUT** we already returned PTS=100, then PTS=??? (whatever was #2), which might have been 200
9. If we returned 200, then 150 → FAILURE ❌

**B. Constructor Pre-fill Issue**:
- Constructor calls `fillPacketQueue()` then `next()` (lines 160, 172)
- This might set up an inconsistent state

**C. PTS Calculation Error**:
- PTS stored in PacketInfo might not match frame->timestamp()
- Need to verify PTS values being stored match frame timestamps

### 4. What's the correct fix?

**Options**:

**Option A - Minimal Refill** (safest):
Only refill when queue is EMPTY, not when below threshold:
```cpp
if (m_packetQueue.empty()) {
  fillPacketQueue();  // Fill to MIN_QUEUE_SIZE
}
```
- Pro: Prevents mid-iteration refills that could add out-of-order frames
- Con: Loses temporal ordering benefits during normal playback
- Con: Defeats the purpose of the priority queue

**Option B - Fill-Then-Sort** (correct but complex):
Track which packets we've already read up to:
```cpp
// Keep reading until we have MIN_QUEUE_SIZE AND no more packets before current PTS
while (m_packetQueue.size() < MIN_QUEUE_SIZE && !hitPTSBoundary) {
  // Read packet
  // If packet PTS > all current queue PTS, stop
}
```
- Pro: Ensures temporal ordering
- Con: Complex to implement correctly

**Option C - Larger Buffer** (tried, didn't work):
- Increased `MIN_QUEUE_SIZE` from 16 to 32
- Same failures
- Not the root cause

**Option D - Hybrid Approach** (pragmatic):
- Keep old sequential `next()` for normal iteration
- Use priority queue only after seeks (where it's currently used)
- Pros: Maintains backward compatibility, fixes GPMF issues (Phases 2/4/5)
- Cons: Doesn't fix B-frame ordering for normal playback

---

## Files Modified

### Modified Files (not committed):
```
src/Ravl2/Video/FfmpegMultiStreamIterator.hh
src/Ravl2/Video/FfmpegMultiStreamIterator.cc
```

### Key Changes Summary:

**FfmpegMultiStreamIterator.hh**:
- Line 82: Changed `decodePacket()` return type to `vector<Frame>`
- Lines 187-206: Added `PacketInfoComparator` struct
- Line 206: Updated `m_packetQueue` type to use comparator
- Line 210: Changed `MIN_QUEUE_SIZE` from 16 to 32
- Removed: `m_gpmfFrameBuffer` member (was line 218)

**FfmpegMultiStreamIterator.cc**:
- Lines 209-260: Complete rewrite of `next()` to use priority queue
- Lines 324, 504, 1383: Updated queue clear operations to use new type
- Lines 958-1082: Changed `decodePacket()` to return `vector<Frame>`
- Lines 1000-1034: GPMF now returns all frames directly
- Lines 1439-1530: `fillPacketQueue()` handles multiple frames per packet
- Removed: GPMF buffer draining code (old lines 237-250, 1447-1467)

---

## Test Results

### ✅ Passing Tests (with Phases 2, 4, 5):
```bash
cd /home/charles/src/Ravl2/cmake-build-debug

# GPMF parsing works correctly:
./examples/exGoProMetadata ./_deps/gpmf-parser-src/samples/hero8.mp4 -m 200
# Output: 2 GPS fixes, 2 Gyro frames (202-203 samples), 2 Accel frames ✅

# Original code (commit 713bc8c) passes all tests:
git checkout 713bc8c
cmake --build .
./test/tests "FfmpegMultiStreamIterator - Frame Presentation Order"
# Output: All tests passed (43 assertions) ✅
```

### ❌ Failing Tests (with Phase 1):
```bash
git checkout add_gopro  # Back to current work

./test/tests "FfmpegMultiStreamIterator - Frame Presentation Order"
# Output: test cases: 1 | 0 passed | 1 failed
#         assertions: 43 | 34 passed | 9 failed ❌

./test/tests "[Video]"
# Output: test cases: 8 | 6 passed | 2 failed
#         assertions: 502 | 444 passed | 58 failed ❌
```

---

## Debug Strategy for Tomorrow

### Step 1: Add Comprehensive Logging

Add to `next()` in `FfmpegMultiStreamIterator.cc`:
```cpp
VideoResult<void> FfmpegMultiStreamIterator::next() {
  if (m_packetQueue.empty() || (!m_isAtEnd && m_packetQueue.size() < MIN_QUEUE_SIZE)) {
    SPDLOG_DEBUG("Refilling queue: current size={}, isAtEnd={}", m_packetQueue.size(), m_isAtEnd);
    auto fillResult = fillPacketQueue();
    SPDLOG_DEBUG("After refill: queue size={}", m_packetQueue.size());
  }

  PacketInfo nextPacket = m_packetQueue.top();
  m_packetQueue.pop();

  SPDLOG_DEBUG("Popped frame: PTS={} us, streamIndex={}, remaining in queue={}",
               nextPacket.pts, nextPacket.streamIndex, m_packetQueue.size());

  // Validate ordering
  static int64_t lastPTS = -1;
  if (lastPTS >= 0 && nextPacket.pts < lastPTS) {
    SPDLOG_ERROR("ORDERING BUG: PTS went backwards! {} -> {}", lastPTS, nextPacket.pts);
  }
  lastPTS = nextPacket.pts;

  setCurrentFrame(nextPacket.frame);
  return VideoResult<void>();
}
```

### Step 2: Log fillPacketQueue() Activity

Add to `fillPacketQueue()`:
```cpp
VideoResult<void> FfmpegMultiStreamIterator::fillPacketQueue() {
  SPDLOG_DEBUG("fillPacketQueue() START: queue size={}", m_packetQueue.size());

  while (m_packetQueue.size() < MIN_QUEUE_SIZE) {
    auto frameResult = decodePacket(m_packet, localIndex);
    if (frameResult.isSuccess()) {
      for (const auto& frame : frameResult.value()) {
        int64_t pts = /* calculate PTS */;
        SPDLOG_DEBUG("  Adding frame to queue: PTS={} us", pts);
        m_packetQueue.push(PacketInfo{frame, localIndex, pts});
      }
    }
  }

  SPDLOG_DEBUG("fillPacketQueue() END: queue size={}", m_packetQueue.size());

  // DIAGNOSTIC: Print current queue state
  auto queueCopy = m_packetQueue;  // Copy to inspect
  std::vector<int64_t> ptsValues;
  while (!queueCopy.empty()) {
    ptsValues.push_back(queueCopy.top().pts);
    queueCopy.pop();
  }
  SPDLOG_DEBUG("Queue PTS order (first 10): {}",
               fmt::join(ptsValues.begin(), ptsValues.begin() + std::min(10UL, ptsValues.size()), ", "));
}
```

### Step 3: Run Test with Debug Logging

```bash
SPDLOG_LEVEL=debug ./test/tests "FfmpegMultiStreamIterator - Frame Presentation Order" 2>&1 | tee debug_output.txt
```

Analyze:
1. When does queue refill happen?
2. What PTS values are being added during refill?
3. Are they higher or lower than already-consumed frames?
4. Does the queue maintain correct order?

### Step 4: Verify PTS Calculation

Check if PTS in PacketInfo matches frame->timestamp():
```cpp
for (const auto& frame : frameResult.value()) {
  int64_t calculatedPTS = /* from packet */;
  int64_t framePTS = frame->timestamp().count();

  if (calculatedPTS != framePTS) {
    SPDLOG_WARN("PTS mismatch: calculated={}, frame->timestamp()={}",
                calculatedPTS, framePTS);
  }

  m_packetQueue.push(PacketInfo{frame, localIndex, calculatedPTS});
}
```

### Step 5: Consider Alternative Architectures

**A. Peek-Ahead Architecture**:
```cpp
VideoResult<void> FfmpegMultiStreamIterator::next() {
  // On FIRST call, fill queue completely (read ahead N frames)
  if (!m_queueInitialized) {
    while (!eof && m_packetQueue.size() < LOOKAHEAD_SIZE) {
      readAndDecodePacket();
    }
    m_queueInitialized = true;
  }

  // Pop from queue
  auto frame = m_packetQueue.top();
  m_packetQueue.pop();

  // Refill ONE frame (maintain constant queue depth)
  if (!eof) {
    readAndDecodePacket();  // Adds 1-N frames
  }

  return frame;
}
```

**B. Seek-Only Queue** (Option D - Hybrid):
```cpp
VideoResult<void> FfmpegMultiStreamIterator::next() {
  // Use queue ONLY after seeks
  if (m_wasSeekOperation && !m_packetQueue.empty()) {
    return popFromQueue();
  }

  // Normal iteration: sequential reading (old code)
  while (true) {
    auto frameResult = decodePacket(readNextPacket());
    if (frameResult.isSuccess()) {
      return frameResult.value()[0];  // First frame only for now
    }
  }
}
```

---

## Recommendation for Tomorrow

### Immediate Actions:

1. **Add debug logging** (Step 1-2 above)
2. **Run test with logging** and analyze output
3. **Verify** if refill is adding frames with lower PTS than already consumed

### Decision Point:

**If refill IS the problem**:
- Implement **Peek-Ahead Architecture** (Step 5A)
- Ensures queue always has complete temporal lookahead

**If refill is NOT the problem**:
- Check PTS calculation (Step 4)
- Verify priority queue implementation with unit test
- Consider if there's a C++ std::priority_queue gotcha

### Pragmatic Fallback:

If debugging takes > 2 hours:
- **Revert Phase 1** (`next()` changes)
- **Keep Phases 2, 4, 5** (GPMF fixes, timestamp fixes)
- **Commit as "GPMF Packet Handling Fixes"**
- **Document Phase 1** as separate future work with detailed analysis from this file

This gives immediate value (fixes GPMF double-buffering) while deferring the complex temporal ordering work.

---

## Additional Context

### GPMF Parser Implementation

**Files**:
- `src/Ravl2/GoPro/GpmfParser.hh` (lines 43-47)
- `src/Ravl2/GoPro/GpmfParser.cc` (lines 18-74)

**How it works**:
```cpp
std::vector<std::shared_ptr<Frame>> GpmfParser::parse(data, size, streamId, timestamp) {
  std::vector<std::shared_ptr<Frame>> frames;

  // Search for GPS data
  if (GPMF_FindNext(&stream, GPS5) == GPMF_OK) {
    frames.push_back(makeGPSFrame());
  }

  // Search for Gyro data
  if (GPMF_FindNext(&stream, GYRO) == GPMF_OK) {
    frames.push_back(makeGyroFrame());  // 200+ samples
  }

  // Search for Accel data
  if (GPMF_FindNext(&stream, ACCL) == GPMF_OK) {
    frames.push_back(makeAccelFrame());  // 200+ samples
  }

  return frames;  // Returns 0-3 frames
}
```

All frames from same packet get **same timestamp** (packet PTS). This is correct - they're from the same temporal slice.

### Original GPMF Buffer Architecture

**Why it existed**:
```cpp
// OLD CODE (removed in Phase 2):
auto frames = m_gpmfParser->parse(...);  // Returns 3 frames
auto firstFrame = frames[0];              // GPS
m_gpmfFrameBuffer.push_back(frames[1]);   // Gyro → buffer
m_gpmfFrameBuffer.push_back(frames[2]);   // Accel → buffer
return firstFrame;                        // Return GPS immediately

// Next call to next():
if (!m_gpmfFrameBuffer.empty()) {
  return m_gpmfFrameBuffer.front();       // Return Gyro
}
```

**Why it was buggy**:
- At EOF, `fillPacketQueue()` would dump `m_gpmfFrameBuffer` into `m_packetQueue`
- But frames might have already been returned via the buffer drain in `next()`
- Result: Duplicate frames or orphaned frames

**Why Phase 2 fix is correct**:
- Now all frames go into priority queue immediately
- No separate buffer to manage
- Temporal ordering handles everything

---

## Questions for User

1. **Is B-frame support a requirement?** If `sample-5s.mp4` is representative of all use cases, the original sequential code works fine.

2. **What video formats are you targeting?**
   - GoPro files (tested, work with Phases 2/4/5)
   - Generic MP4 with B-frames?
   - Multi-stream with audio/video/data interleaving?

3. **Priority**: Would you rather have:
   - Working GPMF support NOW (revert Phase 1, commit 2/4/5)
   - Full temporal ordering LATER (debug Phase 1 further)

4. **Test coverage**: Should we add B-frame test videos to catch temporal ordering bugs?

---

## References

- **Full Plan**: `/home/charles/.claude/plans/curious-tinkering-duck.md`
- **Git History**: `git log src/Ravl2/Video/FfmpegMultiStreamIterator.cc`
- **Original Working Code**: `git show 713bc8c:src/Ravl2/Video/FfmpegMultiStreamIterator.cc`
- **Test File**: `test/test_VideoIO.cc` lines 58-115
- **Sample Video**: `data/sample-5s.mp4` (no B-frames)
- **GPMF Sample**: `./_deps/gpmf-parser-src/samples/hero8.mp4`

---

## End of Status Document

**Last Updated**: 2025-11-29 20:55 UTC
**Next Session**: Review debug logging output and make architecture decision
