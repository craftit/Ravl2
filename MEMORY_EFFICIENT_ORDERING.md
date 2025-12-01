# Memory-Efficient Frame Ordering Strategies

**Problem**: Current approach can hold 300+ decompressed frames (~3.7GB for 4K), which is excessive.

---

## Current Approach Analysis

### Memory Usage
- **Full HD (1920×1080)**: ~3.1 MB/frame × 300 = **930 MB**
- **4K (3840×2160)**: ~12.4 MB/frame × 300 = **3.7 GB**
- **8K (7680×4320)**: ~49.6 MB/frame × 300 = **14.9 GB**

### Why We Currently Need Large Queue
The queue size is calculated as `2-3× GOP size` because:
1. B-frames can be decoded up to GOP size away from presentation position
2. We need lookahead to properly sort presentation timestamps
3. Multi-stream scenarios need extra buffering

---

## Better Approach: Packet-Level Sorting 🎯 RECOMMENDED

### Concept
Instead of decoding all frames and sorting them, **sort packets first, then decode only what we need**.

### How It Works
```
1. Read N compressed packets (lightweight - ~1KB each)
2. Sort packets by PTS
3. Decode ONLY the next frame to deliver
4. Discard decoded frame after delivery
5. Refill packet buffer as needed
```

### Memory Savings
- **Compressed packets**: ~1 KB/packet × 300 = **300 KB**
- **One decoded frame**: ~12.4 MB (4K)
- **Total**: ~12.7 MB vs 3.7 GB = **99.7% reduction!**

### Implementation

```cpp
class FfmpegMultiStreamIterator
{
private:
  // Instead of queue of decoded frames:
  // std::priority_queue<PacketInfo> m_packetQueue;  // OLD - holds decoded frames

  // New: Queue of compressed packets with metadata
  struct PacketMetadata
  {
    AVPacket* packet;           // Compressed packet (~1KB)
    std::size_t streamIndex;    // Which stream
    int64_t pts;                // Presentation timestamp
    bool isKeyframe;            // For seeking
  };

  std::priority_queue<PacketMetadata,
                      std::vector<PacketMetadata>,
                      PacketMetadataComparator> m_compressedPacketQueue;

  // Keep only ONE decoded frame at a time
  std::shared_ptr<Frame> m_currentDecodedFrame;
};

VideoResult<void> FfmpegMultiStreamIterator::next()
{
  // 1. If packet queue is low, refill with compressed packets
  if (m_compressedPacketQueue.size() < m_minQueueSize)
  {
    fillCompressedPacketQueue();  // Read & sort COMPRESSED packets
  }

  // 2. Pop next packet in PTS order
  PacketMetadata nextPacket = m_compressedPacketQueue.top();
  m_compressedPacketQueue.pop();

  // 3. Decode ONLY this one packet
  auto frameResult = decodePacket(nextPacket.packet, nextPacket.streamIndex);

  // 4. Free the compressed packet immediately
  av_packet_unref(nextPacket.packet);

  // 5. Return the decoded frame
  if (frameResult.isSuccess() && !frameResult.value().empty())
  {
    m_currentDecodedFrame = frameResult.value()[0];
    setCurrentFrame(m_currentDecodedFrame);
    return VideoResult<void>();
  }

  return VideoResult<void>(VideoErrorCode::DecodingError);
}
```

### Advantages
✅ **99%+ memory reduction** - only compressed packets + 1 decoded frame
✅ **Simpler logic** - sort packets, decode one at a time
✅ **Better for streaming** - don't need to buffer huge amounts
✅ **Scales to 8K/16K** - memory usage stays constant

### Challenges
⚠️ **Decoder state** - Can't randomly decode packets, must decode in sequence
⚠️ **B-frame dependencies** - Need to decode reference frames even if not displayed
⚠️ **Seeking** - Must reset decoder and decode from keyframe

---

## Alternative: Hybrid Approach with Lazy Decoding 🎯 PRACTICAL

### Concept
Keep packets in queue, but only decode when actually requested.

### How It Works
```cpp
struct LazyFrame
{
  AVPacket* compressedPacket;  // Keep compressed (~1KB)
  std::size_t streamIndex;
  int64_t pts;

  mutable std::shared_ptr<Frame> decodedFrame;  // Decode on-demand
  mutable bool isDecoded = false;

  std::shared_ptr<Frame> getFrame(Decoder* decoder) const
  {
    if (!isDecoded)
    {
      decodedFrame = decoder->decode(compressedPacket, streamIndex);
      isDecoded = true;
    }
    return decodedFrame;
  }
};

std::priority_queue<LazyFrame> m_lazyFrameQueue;
```

### Advantages
✅ **90%+ memory reduction** - most frames stay compressed
✅ **Decode only what's accessed** - perfect for seeking/skipping
✅ **Maintains current API** - minimal code changes

### Disadvantages
⚠️ **Decoder state issues** - Still need to decode in order for B-frames
⚠️ **Complexity** - Need to track decoder position

---

## Practical Solution: Reduce Queue Size Intelligently 🎯 EASIEST

### Reality Check
Most videos don't actually need 300 frames buffered:

1. **Typical GOP sizes**:
   - Webcam/streaming: GOP=60 (2s at 30fps)
   - Professional: GOP=25-30 (1s at 25-30fps)
   - Broadcast: GOP=12-15 (0.5s)
   - **Only long-form video uses GOP=300**

2. **B-frame reordering distance**:
   - Typically 2-4 frames
   - Rarely exceeds 16 frames
   - Maximum reordering is `max_b_frames`, not GOP size

3. **Our current calculation is conservative**:
   ```cpp
   requiredSize = gopSize * 2;  // or * 3 for multi-stream
   ```

### Better Calculation

```cpp
void FfmpegMultiStreamIterator::calculateQueueSize()
{
  m_minQueueSize = 32;  // Minimum baseline

  for (size_t i = 0; i < m_streamIndices.size(); ++i)
  {
    auto* codecContext = m_codecContexts[i];
    if (!codecContext || codecContext->codec_type != AVMEDIA_TYPE_VIDEO)
      continue;

    // Get actual B-frame reordering distance, not GOP size
    int maxReorder = 0;

    // Check max_b_frames (H.264/H.265)
    if (codecContext->max_b_frames > 0)
    {
      maxReorder = codecContext->max_b_frames + 2;  // B-frames + safety margin
      SPDLOG_DEBUG("Video stream {} has max_b_frames: {}", i, codecContext->max_b_frames);
    }
    else if (codecContext->has_b_frames)
    {
      // Codec has B-frames but doesn't report count
      maxReorder = 16;  // Conservative default
    }
    else
    {
      // No B-frames (e.g., baseline H.264, MJPEG)
      maxReorder = 1;
      SPDLOG_DEBUG("Video stream {} has no B-frames, minimal queue needed", i);
    }

    // Need buffer for reordering + safety margin
    // NOT based on GOP size - that's irrelevant for reordering!
    std::size_t requiredSize = static_cast<std::size_t>(maxReorder * 4);

    // Multi-stream: add buffer for interleaving
    if (m_streamIndices.size() > 1)
    {
      requiredSize += 32;
    }

    m_minQueueSize = std::max(m_minQueueSize, requiredSize);
  }

  // Cap at reasonable maximum
  m_minQueueSize = std::min(m_minQueueSize, std::size_t(128));

  SPDLOG_INFO("Set packet queue minimum size to: {} (based on B-frame reordering)",
              m_minQueueSize);
}
```

### Memory Savings
- **Old**: GOP=300 → queue=600 → 600×12.4MB = **7.4 GB**
- **New**: max_b_frames=4 → queue=48 → 48×12.4MB = **595 MB**
- **Savings**: **92% reduction** for typical content

### Advantages
✅ **Simple** - just change calculation
✅ **No architecture changes** - works with current code
✅ **Still handles B-frames correctly**
✅ **Scales reasonably** - 128 frame cap

---

## Recommendation

### Short Term (Immediate): Fix Queue Size Calculation
Implement the better calculation based on `max_b_frames`, not `gop_size`.

**Effort**: 30 minutes
**Memory savings**: 90%+
**Risk**: Low

### Medium Term: Add Queue Size Cap Parameter
Allow users to configure max queue size:

```cpp
FfmpegMultiStreamIterator(container, streams, maxQueueSize = 128);
```

**Effort**: 15 minutes
**Benefit**: Users can tune memory vs latency

### Long Term: Packet-Level Sorting
Refactor to sort compressed packets and decode on-demand.

**Effort**: 4-8 hours
**Memory savings**: 99%+
**Risk**: Medium - significant refactoring

---

## Current Code Issue

Looking at our current implementation (line 298-321):

```cpp
if (codecContext && codecContext->gop_size > 0)
{
  gopSize = codecContext->gop_size;  // WRONG - GOP size != reordering distance
}

requiredSize = static_cast<std::size_t>(gopSize * 2);  // Way too large!
```

**Problem**: We're using GOP size as a proxy for reordering distance, but they're different:
- **GOP size**: Distance between keyframes (can be 300+)
- **Reordering distance**: How far apart frames are in decode vs display order (typically 2-16)

**Fix**: Use `max_b_frames` instead of `gop_size`.

---

## Implementation Priority

1. **NOW**: Fix queue calculation to use `max_b_frames` instead of `gop_size`
2. **SOON**: Add configurable queue size cap
3. **LATER**: Consider packet-level sorting if memory still an issue

This will reduce memory from **7.4 GB → 595 MB** for 4K video with minimal code changes!
