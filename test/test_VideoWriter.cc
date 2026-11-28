//
// Tests for FFmpeg video and audio output.
//

#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/IO/OutputSequence.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include "Ravl2/Video/FfmpegMediaWriter.hh"
#include "Ravl2/Video/Frame.hh"
#include "Ravl2/Video/VideoIO.hh"

namespace Ravl2::Video
{
  namespace
  {
    std::filesystem::path temporaryVideoPath(const std::string &extension)
    {
      const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
      return std::filesystem::temp_directory_path() / ("ravl2-video-writer-" + std::to_string(unique) + extension);
    }

    Array<PixelRGB8, 2> makeImage(int width, int height, int frameIndex)
    {
      Array<PixelRGB8, 2> image({height, width});
      for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
          image(y, x) = PixelRGB8(static_cast<uint8_t>((x + frameIndex) % 256),
                                  static_cast<uint8_t>((y + frameIndex * 3) % 256),
                                  static_cast<uint8_t>((x + y + frameIndex * 7) % 256));
        }
      }
      return image;
    }

    std::size_t countVideoFrames(const std::shared_ptr<MediaContainer> &container, std::size_t streamIndex)
    {
      auto iteratorResult = container->createIterator(streamIndex);
      if(!iteratorResult.isSuccess())
        return 0;
      auto iterator = iteratorResult.value();
      std::size_t count = 0;
      if(iterator->currentFrame())
        ++count;
      for(;;) {
        const auto result = iterator->next();
        if(result.isSuccess()) {
          if(iterator->currentFrame())
            ++count;
          continue;
        }
        if(result.error() == VideoErrorCode::NeedMoreData)
          continue;
        if(result.error() == VideoErrorCode::EndOfStream)
          break;
        break;
      }
      return count;
    }
  }

  TEST_CASE("VideoWriter RGB frames", "[Video][IO]")
  {
    initIO();
    const auto mp4Path = temporaryVideoPath(".mp4");
    {
      auto output = openOutputStream<Array<PixelRGB8, 2>>(mp4Path.string(), {{"fps", 30}});
      REQUIRE(output.valid());
      for(int frameIndex = 0; frameIndex < 30; ++frameIndex)
        output.put(makeImage(64, 48, frameIndex));
      REQUIRE(output.close());
    }

    auto mp4Result = FfmpegMediaContainer::openFile(mp4Path.string());
    REQUIRE(mp4Result.isSuccess());
    auto mp4 = mp4Result.value();
    std::size_t videoIndex = mp4->streamCount();
    for(std::size_t i = 0; i < mp4->streamCount(); ++i) {
      if(mp4->streamType(i) == StreamType::Video) {
        videoIndex = i;
        break;
      }
    }
    REQUIRE(videoIndex < mp4->streamCount());
    auto properties = mp4->videoProperties(videoIndex);
    REQUIRE(properties.isSuccess());
    CHECK(properties.value().width == 64);
    CHECK(properties.value().height == 48);
    CHECK(properties.value().totalFrames == 30);
    CHECK(countVideoFrames(mp4, videoIndex) == 30);
    mp4.reset();
    std::filesystem::remove(mp4Path);

    const auto aviPath = temporaryVideoPath(".avi");
    {
      auto output = openOutputStream<Array<PixelRGB8, 2>>(aviPath.string(), {{"fps", 25}, {"codec", "mpeg4"}});
      REQUIRE(output.valid());
      for(int frameIndex = 0; frameIndex < 6; ++frameIndex)
        output.put(makeImage(64, 48, frameIndex));
      REQUIRE(output.close());
    }
    auto aviResult = FfmpegMediaContainer::openFile(aviPath.string());
    REQUIRE(aviResult.isSuccess());
    auto avi = aviResult.value();
    videoIndex = avi->streamCount();
    for(std::size_t i = 0; i < avi->streamCount(); ++i) {
      if(avi->streamType(i) == StreamType::Video) {
        videoIndex = i;
        break;
      }
    }
    REQUIRE(videoIndex < avi->streamCount());
    CHECK(countVideoFrames(avi, videoIndex) == 6);
    avi.reset();
    std::filesystem::remove(aviPath);
  }

  TEST_CASE("VideoWriter timestamped video and audio", "[Video][IO]")
  {
    initIO();
    const auto path = temporaryVideoPath(".mp4");
    const nlohmann::json options {
      {"fps", 25},
      {"audio", {{"codec", "aac"}, {"sampleRate", 48000}, {"channels", 2}}}
    };
    {
      auto output = openOutputStream<std::shared_ptr<Frame>>(path.string(), options);
      REQUIRE(output.valid());
      constexpr int kFrames = 25;
      constexpr int kSamplesPerChunk = 48000 / kFrames;
      for(int frameIndex = 0; frameIndex < kFrames; ++frameIndex) {
        const MediaTime timestamp(frameIndex * 40000);
        auto image = makeImage(64, 48, frameIndex);
        output.put(std::make_shared<FrameData<Array<PixelRGB8, 2>>>(
          std::move(image), frameIndex, timestamp, StreamType::Video));

        Array<float, 2> samples({kSamplesPerChunk, 2});
        for(int sampleIndex = 0; sampleIndex < kSamplesPerChunk; ++sampleIndex) {
          const double time = static_cast<double>(frameIndex * kSamplesPerChunk + sampleIndex) / 48000.0;
          const float value = static_cast<float>(0.1 * std::sin(2.0 * 3.141592653589793 * 440.0 * time));
          samples(sampleIndex, 0) = value;
          samples(sampleIndex, 1) = value;
        }
        auto audio = AudioChunk<float>(std::move(samples));
        output.put(std::make_shared<FrameData<AudioChunk<float>>>(
          std::move(audio), frameIndex, timestamp, StreamType::Audio));
      }
      REQUIRE(output.close());
    }

    auto containerResult = FfmpegMediaContainer::openFile(path.string());
    REQUIRE(containerResult.isSuccess());
    auto container = containerResult.value();
    std::size_t videoIndex = container->streamCount();
    std::size_t audioIndex = container->streamCount();
    for(std::size_t i = 0; i < container->streamCount(); ++i) {
      if(container->streamType(i) == StreamType::Video)
        videoIndex = i;
      else if(container->streamType(i) == StreamType::Audio)
        audioIndex = i;
    }
    REQUIRE(videoIndex < container->streamCount());
    REQUIRE(audioIndex < container->streamCount());
    CHECK(countVideoFrames(container, videoIndex) == 25);
    CHECK(container->duration() >= MediaTime(900000));
    CHECK(container->duration() <= MediaTime(1200000));
    auto audioIterator = container->createIterator(audioIndex);
    REQUIRE(audioIterator.isSuccess());
    // Audio frames hold AudioChunk<SampleT>; dataType() used to report Array<SampleT, 2>, so typed access failed.
    CHECK(audioIterator.value()->dataType() == typeid(AudioChunk<float>));
    audioIterator = VideoResult<std::shared_ptr<StreamIterator>>(VideoErrorCode::NotFound);
    {
      auto typed = container->createIterator<AudioChunk<float>>(audioIndex);
      CHECK(typed.isValid());
      CHECK(typed.currentFrame().data().data().range().size(0) > 0);
    }
    CHECK(container->createIterator(typeid(AudioChunk<float>), audioIndex).isSuccess());
    container.reset();
    std::filesystem::remove(path);
  }

  TEST_CASE("VideoWriter rejects bad input and recovers", "[Video][IO]")
  {
    const auto path = temporaryVideoPath(".mp4");
    {
      auto writerResult = FfmpegMediaWriter::open(path.string(), nlohmann::json {{"fps", 25}, {"audio", {{"channels", 2}}}});
      REQUIRE(writerResult.isSuccess());
      auto writer = writerResult.value();

      // Odd dimensions can't be encoded as yuv420p; a later valid frame must still work (the failed attempt used
      // to leak its encoder and leave state behind).
      CHECK_FALSE(writer->writeVideo(makeImage(63, 47, 0)));
      CHECK(writer->writeVideo(makeImage(64, 48, 0)));

      // A mono chunk for a stereo encoder is rejected (it used to be accepted and read past the buffer).
      Array<float, 2> mono({100, 1}, 0.0f);
      CHECK_FALSE(writer->writeAudio(AudioChunk<float>(std::move(mono))));
      Array<float, 2> stereo({100, 2}, 0.0f);
      CHECK(writer->writeAudio(AudioChunk<float>(std::move(stereo))));
      CHECK(writer->close());
    }
    std::filesystem::remove(path);
  }

} // namespace Ravl2::Video