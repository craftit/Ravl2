//
// Created on September 6, 2025
//

#pragma once

#include "Ravl2/Video/MediaContainer.hh"
#include <map>
#include <string>
#include <memory>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"
#endif
#pragma GCC diagnostic ignored "-Wold-style-cast"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#pragma GCC diagnostic pop

namespace Ravl2::Video {

// Forward declaration
class FfmpegStreamIterator;

//! Implementation of MediaContainer using FFmpeg for media file access
class FfmpegMediaContainer final : public MediaContainer  {
public:
  //! Constructor - use the factory method for normal creation
  explicit FfmpegMediaContainer();

  //! Destructor - ensures proper cleanup of FFmpeg resources
  ~FfmpegMediaContainer() override;

  //! Factory method to create an FFmpeg-based media container from a file
  [[nodiscard]] static VideoResult<std::shared_ptr<MediaContainer>> openFile(const std::string& filePath);

  //! Check if the container is open
  [[nodiscard]] bool isOpen() const override;

  //! Close the container and release resources
  VideoResult<void> close() override;

  //! Get the number of streams in the container
  [[nodiscard]] std::size_t streamCount() const override;

  //! Get the type of stream at the specified index
  [[nodiscard]] StreamType streamType(std::size_t streamIndex) const override;

  //! Get properties for a video stream
  [[nodiscard]] VideoResult<VideoProperties> videoProperties(std::size_t streamIndex) const override;

  //! Get properties for an audio stream
  [[nodiscard]] VideoResult<AudioProperties> audioProperties(std::size_t streamIndex) const override;

  //! Get properties for a data stream
  [[nodiscard]] VideoResult<DataProperties> dataProperties(std::size_t streamIndex) const override;

  //! Get the total duration of the container (the longest stream)
  [[nodiscard]] MediaTime duration() const override;

  //! Create an iterator for a specific stream
  [[nodiscard]] VideoResult<std::shared_ptr<StreamIterator>> createIterator(std::size_t streamIndex) override;

  //! Get global container metadata
  [[nodiscard]] std::map<std::string, std::string> metadata() const override;

  //! Get specific metadata value
  [[nodiscard]] std::string metadata(const std::string& key) const override;

  //! Check if a specific metadata key exists
  [[nodiscard]] bool hasMetadata(const std::string& key) const override;

private:
  //! Initialize FFmpeg library (called once)
  static void initializeFfmpeg();

  //! Convert FFmpeg error code to VideoErrorCode
  static VideoErrorCode convertFfmpegError(int ffmpegError);

  //! Extract metadata from FFmpeg context
  void extractMetadata();

  //! Map FFmpeg stream types to our StreamType enum
  static StreamType mapFfmpegStreamType(int ffmpegStreamType);

  //! Get codec information from an AVCodecContext
  static CodecInfo getCodecInfo(const AVCodecContext* codecContext);

  //! FFmpeg format context - main container for file information
  AVFormatContext* m_formatContext = nullptr;

  //! FFmpeg codec contexts for each stream
  std::vector<AVCodecContext*> m_codecContexts;

  //! Stream types, cached for faster access
  std::vector<StreamType> m_streamTypes;

  //! Metadata dictionary
  std::map<std::string, std::string> m_metadata;

  //! Flag indicating if FFmpeg has been initialized
  static bool s_ffmpegInitialized;

  //! Friend class declaration to allow the iterator to access private members
  friend class FfmpegStreamIterator;
};

} // namespace Ravl2::Video
