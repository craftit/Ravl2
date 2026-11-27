//
// Compressed media output using FFmpeg.
//

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#include <nlohmann/json.hpp>

#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/Video/Frame.hh"
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Pixel/PixelPlane.hh"

namespace Ravl2::Video
{
  //! FFmpeg-backed writer for compressed video with optional audio.
  //! The writer is not thread-safe. It owns all FFmpeg resources and finalizes
  //! the container when close() is called or when the writer is destroyed.
  class FfmpegMediaWriter
  {
  public:
    struct Impl;
    //! Open a media file writer. Video dimensions are determined from the first frame.
    //! @param path Output file path.
    //! @param options JSON options including fps, codec, crf, preset, audio and hwAccel.
    //! @return A writer on success, or a video error code.
    [[nodiscard]] static VideoResult<std::shared_ptr<FfmpegMediaWriter>> open(const std::string &path,
                                                                               const nlohmann::json &options = nlohmann::json::object());

    ~FfmpegMediaWriter();

    FfmpegMediaWriter(const FfmpegMediaWriter &) = delete;
    FfmpegMediaWriter &operator=(const FfmpegMediaWriter &) = delete;

    //! Write an RGB8 image at the supplied media timestamp or configured frame rate.
    [[nodiscard]] bool writeVideo(const Array<PixelRGB8, 2> &image, std::optional<MediaTime> timestamp = std::nullopt);

    //! Write a BGR8 image at the supplied media timestamp or configured frame rate.
    [[nodiscard]] bool writeVideo(const Array<PixelBGR8, 2> &image, std::optional<MediaTime> timestamp = std::nullopt);

    //! Write a planar YUV420 image at the supplied media timestamp or configured frame rate.
    [[nodiscard]] bool writeVideo(const YUV420Image<uint8_t> &image, std::optional<MediaTime> timestamp = std::nullopt);

    //! Write a timestamped frame. Supported image and audio payloads are documented in Video/ReadMe.md.
    [[nodiscard]] bool writeFrame(const Frame &frame);

    //! Write a multi-channel audio chunk. Array dimensions are samples by channels.
    template <typename SampleT>
    [[nodiscard]] bool writeAudio(const AudioChunk<SampleT> &chunk, std::optional<MediaTime> timestamp = std::nullopt)
    {
      if constexpr(std::is_same_v<SampleT, float>)
        return writeAudioFloat(chunk.data(), timestamp);
      else if constexpr(std::is_same_v<SampleT, int16_t>)
        return writeAudioInt16(chunk.data(), timestamp);
      else
        return false;
    }

    //! Flush encoders and write the container trailer. Safe to call more than once.
    [[nodiscard]] bool close() noexcept;

  private:
    explicit FfmpegMediaWriter(std::unique_ptr<Impl> impl);
    [[nodiscard]] bool initializeAudio(int channels);
    [[nodiscard]] bool writeAudioFloat(const Array<float, 2> &audio, std::optional<MediaTime> timestamp);
    [[nodiscard]] bool writeAudioInt16(const Array<int16_t, 2> &audio, std::optional<MediaTime> timestamp);
    [[nodiscard]] bool writePacked(const uint8_t *data, int width, int height, int lineSize, int pixelFormat,
                                  std::optional<MediaTime> timestamp);
    [[nodiscard]] bool writePlanar420(const YUV420Image<uint8_t> &image, std::optional<MediaTime> timestamp);

    std::unique_ptr<Impl> mImpl;
  };

} // namespace Ravl2::Video