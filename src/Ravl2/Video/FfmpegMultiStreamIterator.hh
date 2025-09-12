// filepath: /home/charles/src/Ravl2/src/Ravl2/Video/FfmpegMultiStreamIterator.hh
//
// Created on September 12, 2025
//

#pragma once

#include "Ravl2/Video/StreamIterator.hh"
#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include <memory>
#include <vector>
#include <map>
#include <queue>

// Forward declarations for FFmpeg structures
struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct AVStream;

namespace Ravl2::Video {

//! Implementation of StreamIterator that provides frames for multiple streams in a FFmpeg-based media container
class FfmpegMultiStreamIterator final : public StreamIterator {
public:
  //! Constructor taking a container and a vector of stream indices to include
  //! If no stream indices are provided, all streams will be included
  FfmpegMultiStreamIterator(std::shared_ptr<FfmpegMediaContainer> container,
                           const std::vector<std::size_t>& streamIndices = {});

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
  [[nodiscard]] std::type_info const &dataType() const override;

  //! Get the stream index for the current frame
  [[nodiscard]] std::size_t currentStreamIndex() const;

private:
  //! Decode a packet for a specific stream
  VideoResult<std::shared_ptr<Frame>> decodePacket(AVPacket* packet, std::size_t streamIndex);

  //! Convert an FFmpeg frame to our Frame type
  [[nodiscard]] std::shared_ptr<Frame> convertFrameToFrame(AVFrame* frame, std::size_t streamIndex, StreamItemId id);

  //! Create a video frame from FFmpeg data
  template <typename ImageT>
  [[nodiscard]] std::shared_ptr<VideoFrame<ImageT>> createVideoFrame(AVFrame* frame, std::size_t streamIndex, StreamItemId id);

  //! Create an audio chunk from FFmpeg data
  template <typename SampleT>
  [[nodiscard]] std::shared_ptr<AudioChunk<SampleT>> createAudioChunk(AVFrame* frame, std::size_t streamIndex, StreamItemId id);

  //! Create a metadata frame from FFmpeg data
  template <typename DataT>
  [[nodiscard]] std::shared_ptr<MetaDataFrame<DataT>> createMetadataFrame(AVFrame* frame, std::size_t streamIndex, StreamItemId id);

  //! Make an image from AVFrame data
  template <typename... PlaneTypes>
  bool makeImage(PlanarImage<2,PlaneTypes...>& img, const AVFrame* frame) const;

  //! Get direct access to the FfmpegMediaContainer
  [[nodiscard]] FfmpegMediaContainer& ffmpegContainer() const;

  //! Container for the media file
  std::shared_ptr<FfmpegMediaContainer> m_ffmpegContainer;

  //! List of stream indices we're tracking
  std::vector<std::size_t> m_streamIndices;

  //! FFmpeg codec contexts for each stream
  std::vector<AVCodecContext*> m_codecContexts;

  //! FFmpeg streams
  std::vector<AVStream*> m_streams;

  //! FFmpeg frames - container for decoded data
  std::vector<AVFrame*> m_frames;

  //! FFmpeg packet - container for compressed data
  AVPacket* m_packet = nullptr;

  //! Frame ID counters for each stream
  std::vector<StreamItemId> m_nextFrameIds;

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
  struct PacketInfo {
    std::shared_ptr<Frame> frame;
    std::size_t streamIndex;
    int64_t pts;
    bool operator<(const PacketInfo& other) const {
      return pts > other.pts; // Priority queue is a max-heap, so invert comparison
    }
  };

  //! Priority queue for presentation ordering
  std::priority_queue<PacketInfo> m_packetQueue;

  //! Minimum buffer size for presentation ordering
  static constexpr std::size_t MIN_QUEUE_SIZE = 16;
};

} // namespace Ravl2::Video
