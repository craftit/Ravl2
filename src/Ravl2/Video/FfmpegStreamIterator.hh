//
// Created on September 6, 2025
//

#pragma once

#include "Ravl2/Video/StreamIterator.hh"
#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include <memory>

// Forward declarations for FFmpeg structures
struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace Ravl2::Video {

//! Implementation of StreamIterator for FFmpeg-based media containers
class FfmpegStreamIterator : public StreamIterator {
public:
  //! Constructor taking a container and stream index
  FfmpegStreamIterator(std::shared_ptr<FfmpegMediaContainer> container, std::size_t streamIndex);

  //! Destructor - ensures proper cleanup of FFmpeg resources
  ~FfmpegStreamIterator() override;

  //! Check if the iterator is at the end of the stream
  bool isAtEnd() const override;

  //! Move to the next frame/chunk
  VideoResult<void> next() override;

  //! Move to the previous frame/chunk
  VideoResult<void> previous() override;

  //! Seek to a specific timestamp
  VideoResult<void> seek(MediaTime timestamp, SeekFlags flags = SeekFlags::Precise) override;

  //! Seek to a specific frame/chunk index
  VideoResult<void> seekToIndex(int64_t index) override;

  //! Get a specific frame by its unique ID
  VideoResult<std::shared_ptr<Frame>> getFrameById(StreamItemId id) const override;

  //! Reset the iterator to the beginning of the stream
  VideoResult<void> reset() override;

  //! Get the total duration of the stream
  MediaTime duration() const override;

  //! Check if the iterator can seek
  bool canSeek() const override;

  //! Get the current position as a frame/chunk index (override for more accurate frame counting)
  int64_t positionIndex() const override;

  //! Get the data element type held in the frames.
  [[nodiscard]] std::type_info const &dataType() const override;

protected:
  template<typename PixelT>
  bool makeImage(Array<PixelT, 2>&img, const AVFrame* frame) const;

  template <typename... PlaneTypes>
  bool makeImage(PlanarImage<2,PlaneTypes... > &img, const AVFrame* frame) const;


  // Helper method to get or create SwsContext for conversion
  SwsContext* getSwsContext(int srcWidth, int srcHeight, AVPixelFormat srcFormat, 
                           int dstWidth, int dstHeight, AVPixelFormat dstFormat) const;

  // SwScale-based implementation for image conversion
  template<typename PixelT>
  bool makeImageWithSwScale(Array<PixelT, 2>&img, const AVFrame* frame) const;

private:
  //! Decode the current packet into a frame
  VideoResult<void> decodeCurrentPacket();

  //! Read the next packet from the stream
  VideoResult<void> readNextPacket();

  //! Convert FFmpeg packet to our Frame type
  std::shared_ptr<Frame> convertPacketToFrame();

  //! Create a video frame from FFmpeg data
  template <typename ImageT>
  std::shared_ptr<VideoFrame<ImageT>> createVideoFrame();

  //! Create an audio chunk from FFmpeg data
  template <typename SampleT>
  std::shared_ptr<AudioChunk<SampleT>> createAudioChunk();

  //! Create a metadata frame from FFmpeg data
  template <typename DataT>
  std::shared_ptr<MetaDataFrame<DataT>> createMetadataFrame();

  //! Get direct access to the FfmpegMediaContainer
  FfmpegMediaContainer& ffmpegContainer() const;

  //! FFmpeg packet - container for compressed data
  AVPacket* m_packet = nullptr;

  //! FFmpeg frame - container for decoded data
  AVFrame* m_frame = nullptr;

  //! Frame ID counter for this stream
  StreamItemId m_nextFrameId = 0;

  //! Indicates if we've reached the end of the stream
  bool m_isAtEnd = false;

  //! Last decoded frame position index
  int64_t m_currentPositionIndex = 0;

  //! Flag indicating if the last operation was a seek
  bool m_wasSeekOperation = false;

  //! FFmpeg stream pointer (from the container)
  AVStream* m_stream = nullptr;

  //! FFmpeg codec context (from the container)
  AVCodecContext* m_codecContext = nullptr;
};

} // namespace Ravl2::Video
