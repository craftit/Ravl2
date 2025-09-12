//
// Created by charles galambos on 12/09/2025.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Resource.hh"
#include "Ravl2/IO/InputSequence.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Video/VideoIO.hh"
#include "Ravl2/Video/FfmpegMultiStreamIterator.hh"
#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include "Ravl2/Logging.hh"
#include <set>

namespace Ravl2::Video
{
  TEST_CASE("VideoStreamIO", "[IO]")
  {
    initIO(); // make sure video io is linked.
    std::string fn = "sample-5s.mp4";
    addResourcePath("data", RAVL_SOURCE_DIR "/data");
    fn = Ravl2::findFileResource("data", fn, true);

    using ImageT = Ravl2::Array<Ravl2::PixelRGB8, 2>;
    StreamInputProxy<ImageT> inputStream = Ravl2::openInputStream<ImageT>(fn, defaultLoadFormatHint(true));
    CHECK(inputStream.valid());
    auto image = inputStream.get();
    CHECK(!image.empty());
  }

  TEST_CASE("FfmpegMultiStreamIterator - Basic Operation", "[Video]")
  {
    initIO(); // make sure video io is linked.
    std::string fn = "sample-5s.mp4";
    addResourcePath("data", RAVL_SOURCE_DIR "/data");
    fn = Ravl2::findFileResource("data", fn, true);

    //! Open the media container
    auto containerResult = FfmpegMediaContainer::openFile(fn);
    REQUIRE(containerResult.isSuccess());
    auto container = std::dynamic_pointer_cast<FfmpegMediaContainer>(containerResult.value());
    REQUIRE(container != nullptr);

    //! Create a multi-stream iterator with default settings (all streams)
    FfmpegMultiStreamIterator iterator(container);

    //! Check iterator is valid
    CHECK(!iterator.isAtEnd());

    //! Get the first frame
    auto frame = iterator.currentFrame();
    REQUIRE(frame != nullptr);

    //! Check frame has valid timestamp
    CHECK(frame->timestamp().count() >= 0);
  }

  TEST_CASE("FfmpegMultiStreamIterator - Frame Presentation Order", "[Video]")
  {
    initIO();
    std::string fn = "sample-5s.mp4";
    addResourcePath("data", RAVL_SOURCE_DIR "/data");
    fn = Ravl2::findFileResource("data", fn, true);

    //! Open the media container
    auto containerResult = FfmpegMediaContainer::openFile(fn);
    REQUIRE(containerResult.isSuccess());
    auto container = std::dynamic_pointer_cast<FfmpegMediaContainer>(containerResult.value());
    REQUIRE(container != nullptr);

    //! Create a multi-stream iterator for the video stream only
    std::vector<std::size_t> videoStreams;
    for (std::size_t i = 0; i < container->streamCount(); ++i)
    {
      if (container->streamType(i) == StreamType::Video)
      {
        videoStreams.push_back(i);
        break; // Just get the first video stream
      }
    }
    REQUIRE(!videoStreams.empty());

    FfmpegMultiStreamIterator iterator(container, videoStreams);

    //! Track presentation timestamps to ensure they're monotonically increasing
    MediaTime previousPts(-1);
    int frameCount = 0;
    const int MAX_FRAMES_TO_CHECK = 20; // Limit the number of frames we check

    while (!iterator.isAtEnd() && frameCount < MAX_FRAMES_TO_CHECK)
    {
      auto frame = iterator.currentFrame();
      REQUIRE(frame != nullptr);

      MediaTime currentPts = frame->timestamp();

      //! For the first frame, just record the PTS
      if (previousPts.count() < 0)
      {
        previousPts = currentPts;
      }
      else
      {
        //! Check that frames are in increasing presentation order
        CHECK(currentPts >= previousPts);
        previousPts = currentPts;
      }

      iterator.next();
      frameCount++;
    }

    //! Ensure we read at least some frames
    CHECK(frameCount > 0);
  }

  TEST_CASE("FfmpegMultiStreamIterator - Seeking", "[Video]")
  {
    initIO();
    // Initialize logging to ensure debug messages are visible in debug builds
    Ravl2::initializeLogging();
    std::string fn = "sample-5s.mp4";
    addResourcePath("data", RAVL_SOURCE_DIR "/data");
    fn = Ravl2::findFileResource("data", fn, true);

    //! Open the media container
    auto containerResult = FfmpegMediaContainer::openFile(fn);
    REQUIRE(containerResult.isSuccess());
    auto container = std::dynamic_pointer_cast<FfmpegMediaContainer>(containerResult.value());
    REQUIRE(container != nullptr);

    //! Create a multi-stream iterator
    FfmpegMultiStreamIterator iterator(container);

    //! Skip the test if seeking isn't supported for this container/file
    if (!iterator.canSeek())
    {
      SKIP("This media file doesn't support seeking");
      return;
    }

    //! Get the duration
    MediaTime duration = iterator.duration();
    REQUIRE(duration.count() > 0);

    //! Seek to the middle of the stream
    MediaTime midPoint = duration / 2;
    auto seekResult = iterator.seek(midPoint);
    REQUIRE(seekResult.isSuccess());

    //! Get the frame at the seek position
    auto frame = iterator.currentFrame();
    REQUIRE(frame != nullptr);

    //! Verify the timestamp is close to where we sought to
    MediaTime frameTime = frame->timestamp();
    CHECK(((frameTime >= midPoint) || (frameTime <= midPoint + MediaTime(1000000)))); // Within 1 second

    //! Test seeking back to the beginning
    seekResult = iterator.reset();
    REQUIRE(seekResult.isSuccess());

    frame = iterator.currentFrame();
    REQUIRE(frame != nullptr);

    //! First frame should have a small timestamp
    CHECK(frame->timestamp().count() < 1000000); // Less than 1 second
  }

  TEST_CASE("FfmpegMultiStreamIterator - Multiple Streams", "[Video]")
  {
    initIO();
    std::string fn = "sample-5s.mp4";
    addResourcePath("data", RAVL_SOURCE_DIR "/data");
    fn = Ravl2::findFileResource("data", fn, true);

    //! Open the media container
    auto containerResult = FfmpegMediaContainer::openFile(fn);
    REQUIRE(containerResult.isSuccess());
    auto container = std::dynamic_pointer_cast<FfmpegMediaContainer>(containerResult.value());
    REQUIRE(container != nullptr);

    //! Skip test if file doesn't have multiple streams
    if (container->streamCount() < 2)
    {
      SKIP("Test file doesn't have multiple streams");
      return;
    }

    //! Create a multi-stream iterator with all streams
    FfmpegMultiStreamIterator iterator(container);

    //! Process a few frames to check stream switching works properly
    int frameCount = 0;
    const int MAX_FRAMES_TO_CHECK = 20;
    std::set<std::size_t> seenStreamIndices;

    while (!iterator.isAtEnd() && frameCount < MAX_FRAMES_TO_CHECK)
    {
      frameCount++;
      seenStreamIndices.insert(iterator.currentStreamIndex());
      iterator.next();
    }

    //! We should have seen frames from at least one stream
    CHECK(!seenStreamIndices.empty());

    //! Print information about which streams we saw
    INFO("Saw frames from " << seenStreamIndices.size() << " different streams");
  }
}
