// filepath: /home/charles/src/Ravl2/src/Ravl2/Video/FfmpegMultiStreamIterator.hh
//
// Created on September 12, 2025
//

#pragma once

#include <memory>
#include <vector>
#include <map>
#include <queue>
#include "Ravl2/Video/StreamIterator.hh"
#include "Ravl2/Video/FfmpegMediaContainer.hh"

// Forward declarations for FFmpeg structures
struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct AVStream;

#ifdef WITH_GPMF
namespace Ravl2::GoPro
{
  class GpmfParser;
}
#endif

namespace Ravl2::Video
{
  //! Implementation of StreamIterator that provides frames for multiple streams in a FFmpeg-based media container
  //!
  //! This iterator maintains temporal ordering across all streams by buffering decoded frames
  //! in a priority queue sorted by presentation timestamp (PTS).
  //!
  //! @note Thread Safety: This class is NOT thread-safe. Each thread must create its own iterator instance.
  //!       Multiple iterators may share the same FfmpegMediaContainer, but the container must provide
  //!       thread-safe access if used concurrently.
  //!
  //! @note Frame Cloning: For device capture inputs (webcam, AVFoundation, V4L2, DirectShow), frames are
  //!       automatically cloned to prevent buffer pool exhaustion. This is detected automatically based
  //!       on the input format.
  //!
  //! @note Keyframe Index: The keyframe index is built lazily on first seek.
  class FfmpegMultiStreamIterator final : public StreamIterator
  {
  public:
    //! Constructor taking a container and a vector of stream indices to include
    //! If no stream indices are provided, all streams will be included
    FfmpegMultiStreamIterator(std::shared_ptr<FfmpegMediaContainer> container,
                              const std::vector<std::size_t>&streamIndices = {});

    //! Destructor - ensures proper clean-up of FFmpeg resources
    ~FfmpegMultiStreamIterator() override;

    //! Check if the iterator is at the end of all streams
    [[nodiscard]] bool isAtEnd() const override;

    //! Move to the next frame across all streams (returns the earliest frame in timeline order)
    VideoResult<void> next() override;

    //! Move to the previous frame across all streams
    VideoResult<void> previous() override;

    //! Seek to a specific timestamp across all streams
    VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) override;

    //! Seek to a specific frame/chunk index
    VideoResult<void> seekToIndex(int64_t index) override;

    //! Get a specific frame by its unique ID
    //! @note This method creates a temporary iterator to seek to the frame, so it's not thread-safe
    //!       with respect to the original iterator. Do not call while using the iterator from another thread.
    //! @note Backward search is not implemented - only searches forward from the seek position.
    [[nodiscard]] VideoResult<std::shared_ptr<Frame>> getFrameById(StreamItemId id) const override;

    //! Reset the iterator to the beginning of all streams
    VideoResult<void> reset() override;

    //! Get the total duration of the streams (returns the longest stream duration)
    [[nodiscard]] MediaTime duration() const override;

    //! Check if the iterator can seek
    [[nodiscard]] bool canSeek() const override;

    //! Get the current position as a frame/chunk index
    [[nodiscard]] int64_t positionIndex() const override;

    //! Get the data element type held in the frames
    [[nodiscard]] std::type_info const& dataType() const override;

    //! Get the stream index for the current frame
    [[nodiscard]] std::size_t currentStreamIndex() const;

  private:
    //! Decode a packet for a specific stream
    //! Returns a vector of frames (GPMF packets may produce multiple frames: GPS, gyro, accel)
    VideoResult<std::vector<std::shared_ptr<Frame>>> decodePacket(AVPacket* packet, std::size_t streamIndex);

    //! Convert an FFmpeg frame to our Frame type
    [[nodiscard]] std::shared_ptr<Frame> convertFrameToFrame(AVFrame* frame, std::size_t streamIndex, StreamItemId id);

    //! Generate a unique frame ID based on PTS and stream index
    [[nodiscard]] StreamItemId generateUniqueFrameId(AVFrame* frame, std::size_t localIndex);

    //! Traditional seek implementation (used internally)
    VideoResult<void> traditionalSeek(MediaTime timestamp, SeekFlags flags);

    //! Create a video frame from FFmpeg data
    template<typename ImageT> [[nodiscard]] std::shared_ptr<VideoFrame<ImageT>> createVideoFrame(
      AVFrame* frame,
      std::size_t streamIndex,
      StreamItemId id);

    //! Create an audio chunk from FFmpeg data
    template<typename SampleT> [[nodiscard]] std::shared_ptr<AudioChunk<SampleT>> createAudioChunk(
      AVFrame* frame,
      std::size_t streamIndex,
      StreamItemId id);

    //! Create a metadata frame from FFmpeg data
    template<typename DataT> [[nodiscard]] std::shared_ptr<MetaDataFrame<DataT>> createMetadataFrame(
      AVFrame* frame,
      std::size_t streamIndex,
      StreamItemId id);

    //! Make an image from AVFrame data
    template<typename... PlaneTypes> bool makeImage(PlanarImage<2, PlaneTypes...>&img, const AVFrame* frame) const;

    //! Make a packed image from AVFrame data
    template<typename PixelT> bool makeImage(Array<PixelT,2>&img,const AVFrame* frame) const;

    //! Get direct access to the FfmpegMediaContainer
    [[nodiscard]] FfmpegMediaContainer& ffmpegContainer() const;

    //! Flush all codec contexts (skipping nullptr for DATA streams)
    void flushAllCodecs();

    //! Container for the media file
    std::shared_ptr<FfmpegMediaContainer> m_ffmpegContainer;

    //! Structure to store keyframe information for seeking
    struct KeyframeInfo
    {
      int64_t pts; //!< Presentation timestamp
      int64_t pos; //!< Byte position in file
      bool isKeyframe; //!< Whether this is a keyframe
      std::size_t streamIndex; //!< Stream index

      //! Compare operator for sorting
      bool operator<(const KeyframeInfo&other) const
      {
        return pts < other.pts;
      }
    };

    //! Vector to store keyframe information for each stream
    std::vector<std::vector<KeyframeInfo>> m_keyframeIndex;

    //! Flag to indicate if keyframe index has been built
    bool m_keyframeIndexBuilt = false;

    //! Build a keyframe index for faster seeking
    VideoResult<void> buildKeyframeIndex();

    //! Find the nearest keyframe to a given timestamp
    KeyframeInfo findNearestKeyframe(MediaTime timestamp, SeekFlags flags);

    //! List of stream indices we're tracking
    std::vector<std::size_t> m_streamIndices;

    //! FFmpeg codec contexts for each stream
    std::vector<AVCodecContext *> m_codecContexts;

    //! FFmpeg streams
    std::vector<AVStream *> m_streams;

    //! FFmpeg frames - container for decoded data
    std::vector<AVFrame *> m_frames;

    //! FFmpeg packet - container for compressed data
    AVPacket* m_packet = nullptr;

    //! Frame ID counters for each stream
    std::vector<StreamItemId> m_nextFrameIds;

    //! Number of bits needed to represent the stream index in frame IDs
    std::size_t m_streamBits = 0;

    //! Current stream index (which stream the current frame belongs to)
    std::size_t m_currentStreamIndex = 0;

    //! Frame counter for positionIndex()
    int64_t m_frameCounter = 0;

    //! Indicates if we've reached the end of all streams
    bool m_isAtEnd = false;

    //! Flag to track if we recently performed a seek operation
    bool m_wasSeekOperation = false;

    //! Fill the packet queue with decoded frames in presentation order
    VideoResult<void> fillPacketQueue();

    //! Packet buffer for presentation timestamp ordering
    struct PacketInfo
    {
      std::shared_ptr<Frame> frame;
      std::size_t streamIndex;
      int64_t pts;
    };

    //! Comparator for min-heap (smallest PTS has highest priority)
    struct PacketInfoComparator
    {
      bool operator()(const PacketInfo& a, const PacketInfo& b) const
      {
        // Return true if a should come AFTER b (lower priority)
        // For min-heap: larger PTS = lower priority
        return a.pts > b.pts;
      }
    };

    //! Priority queue for presentation ordering (min-heap by PTS)
    std::priority_queue<PacketInfo, std::vector<PacketInfo>, PacketInfoComparator> m_packetQueue;

    //! The minimum buffer size for presentation ordering
    //! Must be large enough to contain all frames needed for temporal reordering (including B-frames)
    static constexpr std::size_t MIN_QUEUE_SIZE = 32;

    //! Maximum number of keyframes we will index on open.
    static constexpr std::size_t MAX_KEYFRAME_INDEX= 10000;

    //! Max frame search when looking for a time code.
    static constexpr int MAX_FRAME_SEARCH = 30;

    //! Flag indicating if frames need to be cloned immediately to free buffers
    //! This is required for capture devices with limited buffer pools (e.g., AVFoundation on macOS)
    bool m_needsFrameClone = false;

#ifdef WITH_GPMF
    //! GPMF parser instance for this iterator
    //! Each iterator needs its own parser to maintain independent state (mNextId)
    std::unique_ptr<GoPro::GpmfParser> m_gpmfParser;
#endif
  };
} // namespace Ravl2::Video
