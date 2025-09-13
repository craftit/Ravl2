#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Ravl2/Video/MemoryMediaContainer.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/AudioChunk.hh"
#include "Ravl2/Video/MetaDataFrame.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"

namespace Ravl2::Video
{
  // For testing purposes, we'll use a simple pixel type
  using TestPixel = uint8_t;
  using AudioSampleT = float;

  std::shared_ptr<VideoFrameBase> createVideoFrame(MediaTime timestamp, StreamItemId id) {
    // Create a test frame with simple pixel data
    Array<TestPixel, 2> frameData({640, 480}, 128); // 640x480 frame filled with value 128

    // Create and return the video frame
    auto frame = std::make_shared<VideoFrame<Array<TestPixel,2> > >(frameData, id, timestamp);
    return frame;
  }

  // Helper function to create an audio chunk
  std::shared_ptr<AudioChunk<AudioSampleT> > createAudioChunk(MediaTime timestamp, StreamItemId id) {
    Ravl2::Array<AudioSampleT, 2> audioData({4410, 2}, 0.0f); // 4410 samples (0.1s at 44.1kHz), 2 channels

    auto chunk = std::make_shared<AudioChunk<AudioSampleT>>(audioData, id, timestamp);
    return chunk;
  }

  // Helper function to create a metadata frame
  std::shared_ptr<MetaDataFrameBase> createMetaDataFrame(MediaTime timestamp, StreamItemId id) {
    // Create metadata with key-value pairs
    std::map<std::string, std::string> data = {
      {"key1", "value1"},
      {"key2", "value2"},
      {"timestamp", std::to_string(timestamp.count())},
      {"frame_id", std::to_string(id)}
    };

    // Create and return the metadata frame
    auto frame = std::make_shared<MetaDataFrame<std::map<std::string, std::string>>>(data, id, timestamp);
    return frame;
  }

  TEST_CASE("MemoryMediaContainer basic functionality", "[video][memory-container]") {
    // Create video frames for video stream
    std::vector<std::shared_ptr<Frame>> videoFrames;
    for (int i = 0; i < 10; ++i)
    {
      videoFrames.push_back(createVideoFrame(MediaTime(i * 33333333), i + 1)); // ~30fps timing
    }

    std::vector<std::shared_ptr<Frame>> audioChunks;
    for (int i = 0; i < 20; ++i) {
      audioChunks.push_back(createAudioChunk(MediaTime(i * 20000000), i + 100)); // ~50 chunks per second
    }

    // Create metadata frames for data stream
    std::vector<std::shared_ptr<Frame>> metadataFrames;
    for (int i = 0; i < 5; ++i) {
      metadataFrames.push_back(createMetaDataFrame(MediaTime(i * 100000000), i + 200));
    }

    // Create video properties
    VideoProperties videoProps;
    videoProps.width = 640;
    videoProps.height = 480;
    videoProps.pixelFormat = "RGB24";
    videoProps.frameRate = 30.0;
    videoProps.duration = MediaTime(9 * 33333333); // Duration of last frame

    // Create audio properties
    AudioProperties audioProps;
    audioProps.sampleRate = 44100;
    audioProps.channels = 2;
    audioProps.duration = MediaTime(19 * 20000000); // Duration of last chunk

    // Create data properties
    DataProperties dataProps;
    dataProps.format = "JSON";
    dataProps.duration = MediaTime(4 * 100000000); // Duration of last metadata frame

    // Create streams
    std::vector<StreamData> streams = {
      StreamData(StreamType::Video, videoFrames, videoProps),
      StreamData(StreamType::Audio, audioChunks, audioProps),
      StreamData(StreamType::Data, metadataFrames, dataProps)
  };

    // Create container metadata
    std::map<std::string, std::string> metadata = {
      {"title", "Test Video"},
      {"author", "Test Author"},
      {"created", "2025-09-06"}
    };

    // Create the container
    auto container = MemoryMediaContainer::create(streams, metadata);

    SECTION("Container basic properties") {
      REQUIRE(container->isOpen());
      REQUIRE(container->streamCount() == 3);
      REQUIRE(container->streamType(0) == StreamType::Video);
      REQUIRE(container->streamType(1) == StreamType::Audio);
      REQUIRE(container->streamType(2) == StreamType::Data);

      // Test duration (should be the longest stream duration)
      // The duration is 20 chunks * 20000000 (timestamp of the last chunk + duration of one chunk)
      REQUIRE(container->duration() == MediaTime(19 * 20000000 + 20000000)); // Audio is longest (20 chunks)
    }

    SECTION("Stream properties") {
      // Test video properties
      auto videoPropsResult = container->videoProperties(0);
      REQUIRE(videoPropsResult.isSuccess());
      REQUIRE(videoPropsResult.value().width == 640);
      REQUIRE(videoPropsResult.value().height == 480);
      REQUIRE(videoPropsResult.value().pixelFormat == "RGB24");
      REQUIRE(videoPropsResult.value().frameRate == Catch::Approx(30.0));

      // Test audio properties
      auto audioPropsResult = container->audioProperties(1);
      REQUIRE(audioPropsResult.isSuccess());
      REQUIRE(audioPropsResult.value().sampleRate == 44100);
      REQUIRE(audioPropsResult.value().channels == 2);

      // Test data properties
      auto dataPropsResult = container->dataProperties(2);
      REQUIRE(dataPropsResult.isSuccess());
      REQUIRE(dataPropsResult.value().format == "JSON");

      // Test invalid stream types
      REQUIRE_FALSE(container->videoProperties(1).isSuccess()); // Audio stream, not video
      REQUIRE_FALSE(container->audioProperties(0).isSuccess()); // Video stream, not audio
      REQUIRE_FALSE(container->dataProperties(0).isSuccess());  // Video stream, not data
    }

    SECTION("Metadata access") {
      REQUIRE(container->hasMetadata("title"));
      REQUIRE(container->hasMetadata("author"));
      REQUIRE(container->hasMetadata("created"));
      REQUIRE_FALSE(container->hasMetadata("nonexistent"));

      REQUIRE(container->metadata("title") == "Test Video");
      REQUIRE(container->metadata("author") == "Test Author");
      REQUIRE(container->metadata("created") == "2025-09-06");
      REQUIRE(container->metadata("nonexistent") == "");

      auto allMetadata = container->metadata();
      REQUIRE(allMetadata.size() == 3);
      REQUIRE(allMetadata["title"] == "Test Video");
    }

    SECTION("Close container") {
      REQUIRE(container->isOpen());
      auto result = container->close();
      REQUIRE(result.isSuccess());
      REQUIRE_FALSE(container->isOpen());

      // Test that operations fail after closing
      auto iterResult = container->createIterator(0);
      REQUIRE_FALSE(iterResult.isSuccess());

      // Test that closing again fails
      auto closeResult = container->close();
      REQUIRE_FALSE(closeResult.isSuccess());
    }

    SECTION("Invalid stream indices") {
      REQUIRE(container->streamType(3) == StreamType::Unknown);
      REQUIRE_FALSE(container->videoProperties(3).isSuccess());
      REQUIRE_FALSE(container->audioProperties(3).isSuccess());
      REQUIRE_FALSE(container->dataProperties(3).isSuccess());
      REQUIRE_FALSE(container->createIterator(3).isSuccess());
    }
  }

  TEST_CASE("MemoryStreamIterator basic functionality", "[video][memory-iterator]") {
    // Create video frames for a single stream
    std::vector<std::shared_ptr<Frame>> videoFrames;
    for (int i = 0; i < 10; ++i) {
      videoFrames.push_back(createVideoFrame(MediaTime(i * 33333333), i + 1)); // ~30fps timing
    }

    // Create stream and container
    VideoProperties videoProps;
    videoProps.width = 640;
    videoProps.height = 480;
    videoProps.pixelFormat = "RGB24";
    videoProps.frameRate = 30.0;
    videoProps.duration = MediaTime(9 * 33333333); // Duration of last frame

    std::vector<StreamData> streams = {
      StreamData(StreamType::Video, videoFrames, videoProps)
  };

    auto container = MemoryMediaContainer::create(streams);

    // Create iterator
    auto iterResult = container->createIterator(0);
    REQUIRE(iterResult.isSuccess());
    auto iterator = iterResult.value();

    SECTION("Iterator basics") {
      REQUIRE(iterator->streamIndex() == 0);
      REQUIRE(iterator->streamType() == StreamType::Video);
      REQUIRE(iterator->position() == MediaTime(0)); // Initial position is first frame
      REQUIRE(iterator->positionIndex() == 0);
      REQUIRE_FALSE(iterator->isAtEnd());
      REQUIRE(iterator->canSeek());
      REQUIRE(iterator->duration() == MediaTime(9 * 33333333));

      // Test current frame
      auto frameResult = iterator->currentFrame();
      REQUIRE(frameResult);
      REQUIRE(frameResult->timestamp() == MediaTime(0));
      REQUIRE(frameResult->id() == 1);
    }

    SECTION("Iterator navigation") {
      // Move forward
      for (int i = 0; i < 9; ++i) {
        REQUIRE_FALSE(iterator->isAtEnd());
        auto result = iterator->next();
        REQUIRE(result.isSuccess());
        REQUIRE(iterator->positionIndex() == i + 1);
        REQUIRE(iterator->position() == MediaTime((i + 1) * 33333333));
      }

      // At last frame, not at end yet
      REQUIRE_FALSE(iterator->isAtEnd());

      // Move past end
      auto result = iterator->next();
      REQUIRE(result.isSuccess());
      REQUIRE(iterator->isAtEnd());

      // Can't get current frame at end
      auto frameResult = iterator->currentFrame();
      REQUIRE_FALSE(frameResult);

      // Try to move past end
      result = iterator->next();
      REQUIRE_FALSE(result.isSuccess());

      // Move backward
      result = iterator->previous();
      REQUIRE(result.isSuccess());
      REQUIRE_FALSE(iterator->isAtEnd());
      REQUIRE(iterator->positionIndex() == 9);
      REQUIRE(iterator->position() == MediaTime(9 * 33333333));

      // Get current frame after moving back
      frameResult = iterator->currentFrame();
      REQUIRE(frameResult);
      REQUIRE(frameResult->timestamp() == MediaTime(9 * 33333333));

      // Reset to beginning
      result = iterator->reset();
      REQUIRE(result.isSuccess());
      REQUIRE(iterator->positionIndex() == 0);
      REQUIRE(iterator->position() == MediaTime(0));
    }

    SECTION("Iterator seeking") {
      // Seek to timestamp (exact match)
      auto result = iterator->seek(MediaTime(5 * 33333333));
      REQUIRE(result.isSuccess());
      REQUIRE(iterator->positionIndex() == 5);
      REQUIRE(iterator->position() == MediaTime(5 * 33333333));

      // Seek to approximate timestamp
      result = iterator->seek(MediaTime(5 * 33333333 + 10000000));
      REQUIRE(result.isSuccess());
      // Should find closest frame
      REQUIRE(iterator->positionIndex() == 5);
      REQUIRE(iterator->position() == MediaTime(5 * 33333333));

      // Seek with Previous flag
      result = iterator->seek(MediaTime(5 * 33333333 + 10000000), SeekFlags::Previous);
      REQUIRE(result.isSuccess());
      REQUIRE(iterator->positionIndex() == 5);

      // Seek with Next flag
      result = iterator->seek(MediaTime(5 * 33333333 - 10000000), SeekFlags::Next);
      REQUIRE(result.isSuccess());
      REQUIRE(iterator->positionIndex() == 5);

      // Seek to index
      result = iterator->seekToIndex(8);
      REQUIRE(result.isSuccess());
      REQUIRE(iterator->positionIndex() == 8);
      REQUIRE(iterator->position() == MediaTime(8 * 33333333));

      // Seek to invalid index
      result = iterator->seekToIndex(20);
      REQUIRE_FALSE(result.isSuccess());
      // Position should be unchanged
      REQUIRE(iterator->positionIndex() == 8);

      // Seek to negative index
      result = iterator->seekToIndex(-1);
      REQUIRE_FALSE(result.isSuccess());
    }

    SECTION("Frame access by ID") {
      // Get frame by valid ID
      auto frameResult = iterator->getFrameById(5);
      REQUIRE(frameResult.isSuccess());
      REQUIRE(frameResult.value()->id() == 5);
      REQUIRE(frameResult.value()->timestamp() == MediaTime(4 * 33333333));

      // Get frame by invalid ID
      frameResult = iterator->getFrameById(100);
      REQUIRE_FALSE(frameResult.isSuccess());
    }
  }

  TEST_CASE("MemoryMediaContainer edge cases", "[video][memory-container]") {
    SECTION("Empty container") {
      std::vector<StreamData> emptyStreams;
      auto container = MemoryMediaContainer::create(emptyStreams);

      REQUIRE(container->isOpen());
      REQUIRE(container->streamCount() == 0);
      REQUIRE(container->duration() == MediaTime(0));

      // Test that operations with invalid indices fail properly
      REQUIRE_FALSE(container->createIterator(0).isSuccess());
      REQUIRE(container->streamType(0) == StreamType::Unknown);
    }

    SECTION("Empty streams") {
      // Create an empty video stream
      std::vector<std::shared_ptr<Frame>> emptyFrames;
      VideoProperties videoProps;
      videoProps.width = 640;
      videoProps.height = 480;
      videoProps.duration = MediaTime(0);

      std::vector<StreamData> streams = {
        StreamData(StreamType::Video, emptyFrames, videoProps)
    };

      auto container = MemoryMediaContainer::create(streams);
      REQUIRE(container->streamCount() == 1);
      REQUIRE(container->duration() == MediaTime(0));

      // Create iterator for empty stream
      auto iterResult = container->createIterator(0);
      REQUIRE(iterResult.isSuccess());
      auto iterator = iterResult.value();

      // Should be at end immediately
      REQUIRE(iterator->isAtEnd());
      REQUIRE_FALSE(iterator->currentFrame());
      REQUIRE_FALSE(iterator->next().isSuccess());
      REQUIRE_FALSE(iterator->previous().isSuccess());
    }
  }
}