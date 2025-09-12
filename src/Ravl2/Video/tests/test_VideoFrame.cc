// filepath: /Users/charlesgalambos/src/reactai/Ravl2/src/Ravl2/Video/tests/test_VideoFrame.cc
//
// Unit tests for VideoFrame class
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"

namespace Ravl2::Video
{
  // For simplicity, using a simple pixel type
  using TestPixel = uint8_t;

  TEST_CASE("VideoFrameBase properties", "[VideoFrame]")
  {
    // Create a test frame
    Array<TestPixel, 2> frameData({10, 8}, 42); // rows=10 (height), cols=8 (width)
    StreamItemId id = 123;
    MediaTime timestamp = std::chrono::milliseconds(1000);

    VideoFrame<Array<TestPixel, 2>> frame(frameData, id, timestamp);

    SECTION("Basic properties")
    {
      CHECK(frame.id() == id);
      CHECK(frame.timestamp() == timestamp);
      CHECK(frame.streamType() == StreamType::Video);
      CHECK(frame.width() == 8);
      CHECK(frame.height() == 10);
    }

    SECTION("Keyframe flag")
    {
      CHECK_FALSE(frame.isKeyFrame());
      frame.setKeyFrame(true);
      CHECK(frame.isKeyFrame());
      frame.setKeyFrame(false);
      CHECK_FALSE(frame.isKeyFrame());
    }
  }

  TEST_CASE("VideoFrame template class", "[VideoFrame]")
  {
    // Create a test frame
    Array<TestPixel, 2> frameData({5, 5}, 100); // 5x5 frame filled with value 100
    StreamItemId id = 456;
    MediaTime timestamp = std::chrono::milliseconds(2000);

    VideoFrame<Array<TestPixel, 2>> frame(frameData, id, timestamp);

    SECTION("Frame data access")
    {
      const auto&data = frame.image();
      CHECK(data.range().size(0) == 5);
      CHECK(data.range().size(1) == 5);
      CHECK(data[0][0] == 100);
      CHECK(data[4][4] == 100);
    }

    SECTION("Frame validity")
    {
      CHECK(frame.isValid());

      // Create an empty frame
      Array<TestPixel, 2> emptyData({0, 0}, 0);
      VideoFrame<Array<TestPixel, 2>> emptyFrame(emptyData, 789, std::chrono::milliseconds(3000));
      CHECK_FALSE(emptyFrame.isValid());
    }
  }

  TEST_CASE("VideoFrame dimensions", "[VideoFrame]")
  {
    SECTION("Various dimensions")
    {
      // Test different dimensions
      Array<TestPixel, 2> frame1({1, 1}, 1); // rows=1, cols=1
      VideoFrame<Array<TestPixel, 2>> vf1(frame1, 1, std::chrono::milliseconds(100));
      CHECK(vf1.width() == 1);
      CHECK(vf1.height() == 1);

      Array<TestPixel, 2> frame2({9, 16}, 1); // rows=9, cols=16 (16:9)
      VideoFrame<Array<TestPixel, 2>> vf2(frame2, 2, std::chrono::milliseconds(200));
      CHECK(vf2.width() == 16);
      CHECK(vf2.height() == 9);

      Array<TestPixel, 2> frame3({1080, 1920}, 1); // rows=1080, cols=1920 (FullHD 16:9)
      VideoFrame<Array<TestPixel, 2>> vf3(frame3, 3, std::chrono::milliseconds(300));
      CHECK(vf3.width() == 1920);
      CHECK(vf3.height() == 1080);
    }
  }

  // Test with a more complex pixel type
  struct RGBPixel
  {
    uint8_t r, g, b;

    bool operator==(const RGBPixel&other) const
    {
      return r == other.r && g == other.g && b == other.b;
    }
  };

  TEST_CASE("VideoFrame with complex pixel type", "[VideoFrame]")
  {
    Array<RGBPixel, 2> frameData({3, 2}); // rows=3, cols=2

    // Fill with some test data
    for (int y = 0; y < 2; y++)
    {
      for (int x = 0; x < 3; x++)
      {
        frameData[x][y] = {
          static_cast<uint8_t>(x * 50),
          static_cast<uint8_t>(y * 50),
          static_cast<uint8_t>((x + y) * 25)
        };
      }
    }

    VideoFrame<Array<RGBPixel, 2>> frame(frameData, 999, std::chrono::milliseconds(5000));

    SECTION("Frame properties")
    {
      CHECK(frame.width() == 2);
      CHECK(frame.height() == 3);
      CHECK(frame.isValid());
    }

    SECTION("Frame data access")
    {
      const auto&data = frame.image();
      CHECK(data[0][0] == RGBPixel{0, 0, 0});
      CHECK(data[1][0] == RGBPixel{50, 0, 25});
      CHECK(data[2][0] == RGBPixel{100, 0, 50});
      CHECK(data[0][1] == RGBPixel{0, 50, 25});
      CHECK(data[1][1] == RGBPixel{50, 50, 50});
      CHECK(data[2][1] == RGBPixel{100, 50, 75});
    }
  }
} // namespace Ravl2::Video
