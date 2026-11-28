//
// Capture URLs, device capability checks, newest-frame reading, and (when a camera is present) live capture.
//

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <thread>

#include "Ravl2/IO/InputSequence.hh"
#include "Ravl2/Video/CaptureDevice.hh"
#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include "Ravl2/Video/FfmpegMediaWriter.hh"
#include "Ravl2/Video/LatestFrameReader.hh"
#include "Ravl2/Video/VideoIO.hh"

namespace Ravl2::Video
{
  namespace
  {
    //! Open a capture URL as RGB (a function, so the template's comma stays out of Catch2's macros).
    auto openRgb(const std::string &url)
    {
      return openInputStream<Array<PixelRGB8, 2>>(url);
    }

    //! Modes like a Logitech StreamCam's.
    std::vector<CaptureMode> streamCamModes()
    {
      return {
        {"mjpeg", "Motion-JPEG", 640, 480, {60, 30}},
        {"mjpeg", "Motion-JPEG", 1280, 720, {60, 30}},
        {"mjpeg", "Motion-JPEG", 1920, 1080, {60, 30}},
        {"yuyv422", "YUYV 4:2:2", 640, 480, {30}},
        {"yuyv422", "YUYV 4:2:2", 1280, 720, {10}},
      };
    }

    //! First capture device on this machine, if any.
    std::optional<std::string> firstCaptureDevice()
    {
      for(int i = 0; i < 64; ++i) {
        const std::string path = "/dev/video" + std::to_string(i);
        if(std::filesystem::exists(path) && !listCaptureModes(path).empty())
          return path;
      }
      return std::nullopt;
    }
  }// namespace

  TEST_CASE("Capture URLs are parsed", "[Video][Capture]")
  {
    auto plain = parseCaptureUrl("/dev/video8");
    CHECK(plain.device.devicePath == "/dev/video8");
    CHECK_FALSE(plain.fromIndex);
    CHECK(plain.device.width == 0);
    CHECK_FALSE(plain.latestOnly);

    auto full = parseCaptureUrl("/dev/video8?size=1280x720&fps=60&format=MJPG&latest=1");
    CHECK(full.device.devicePath == "/dev/video8");// the query is not part of the path
    CHECK(full.device.width == 1280);
    CHECK(full.device.height == 720);
    CHECK(full.device.frameRate == 60.0f);
    CHECK(full.device.pixelFormat == "mjpeg");
    CHECK(full.latestOnly);

    auto index = parseCaptureUrl("8?format=yuyv");
    CHECK(index.fromIndex);
    CHECK(index.device.pixelFormat == "yuyv422");
#ifdef __linux__
    CHECK(index.device.devicePath == "/dev/video8");
#else
    CHECK(index.device.devicePath == "8");
#endif

    CHECK_THROWS_AS(parseCaptureUrl("/dev/video0?sise=640x480"), std::invalid_argument);
    CHECK_THROWS_AS(parseCaptureUrl("/dev/video0?size=640"), std::invalid_argument);
    CHECK_THROWS_AS(parseCaptureUrl("/dev/video0?size=640x0"), std::invalid_argument);
    CHECK_THROWS_AS(parseCaptureUrl("/dev/video0?fps=fast"), std::invalid_argument);
    CHECK_THROWS_AS(parseCaptureUrl("/dev/video0?latest=maybe"), std::invalid_argument);
  }

  TEST_CASE("Capture requests are checked against device modes", "[Video][Capture]")
  {
    const auto modes = streamCamModes();
    auto request = [](int w, int h, float fps, std::string format) {
      DeviceParameters p;
      p.devicePath = "/dev/video8";
      p.width = w;
      p.height = h;
      p.frameRate = fps;
      p.pixelFormat = std::move(format);
      return p;
    };
    CHECK_FALSE(checkCaptureRequest(modes, request(1280, 720, 60, "mjpeg")));
    CHECK_FALSE(checkCaptureRequest(modes, request(1920, 1080, 0, "")));
    CHECK_FALSE(checkCaptureRequest(modes, request(0, 0, 0, "yuyv422")));
    CHECK_FALSE(checkCaptureRequest(modes, request(0, 0, 0, "")));
    CHECK_FALSE(checkCaptureRequest({}, request(9999, 9999, 0, "")));// nothing to check against

    // Each refusal names the setting and lists the modes.
    auto noFormat = checkCaptureRequest(modes, request(0, 0, 0, "h264"));
    REQUIRE(noFormat);
    CHECK(noFormat->find("format=h264") != std::string::npos);
    CHECK(noFormat->find("mjpeg 1280x720 @ 60, 30 fps") != std::string::npos);
    auto noSize = checkCaptureRequest(modes, request(800, 600, 0, ""));
    REQUIRE(noSize);
    CHECK(noSize->find("size=800x600") != std::string::npos);
    auto noRate = checkCaptureRequest(modes, request(1280, 720, 60, "yuyv422"));
    REQUIRE(noRate);
    CHECK(noRate->find("fps=60") != std::string::npos);
    CHECK(noRate->find("format=yuyv422") != std::string::npos);
  }

  TEST_CASE("Capture format names", "[Video][Capture]")
  {
    CHECK(captureFormatName("MJPG") == "mjpeg");
    CHECK(captureFormatName("YUYV") == "yuyv422");
    CHECK(captureFormatName("ABCD") == "abcd");
    CHECK(normaliseCaptureFormat("YUY2") == "yuyv422");
    CHECK(normaliseCaptureFormat("nv12") == "nv12");
  }

  TEST_CASE("Bad capture URLs fail with a message instead of being ignored", "[Video][Capture]")
  {
    initIO();
    // Unknown option: rejected before any device is touched.
    CHECK_THROWS_AS(openRgb("camera:///dev/video0?colour=blue"), std::invalid_argument);
    // Options for a device that isn't there: an error, not a silent fallback to another handler or mode.
    CHECK_THROWS_AS(openRgb("camera:///dev/ravl2-no-such-camera?size=640x480"), std::invalid_argument);
  }

  TEST_CASE("LatestFrameReader keeps only the newest frame", "[Video][Capture]")
  {
    initIO();
    // A 40-frame video stands in for a camera; reading it slowly forces drops.
    const auto path = std::filesystem::temp_directory_path()
                      / ("ravl2-latest-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".mkv");
    {
      auto writer = FfmpegMediaWriter::open(path.string(), nlohmann::json {{"fps", 25}, {"codec", "ffv1"}, {"pixelFormat", "yuv420p"}});
      REQUIRE(writer.isSuccess());
      Array<PixelRGB8, 2> image({16, 16}, PixelRGB8(10, 20, 30));
      for(int i = 0; i < 40; ++i)
        REQUIRE(writer.value()->writeVideo(image));
      REQUIRE(writer.value()->close());
    }
    auto container = FfmpegMediaContainer::openFile(path.string()).value();
    {
      // One iterator at a time per container.
      auto iterator = container->createIterator(std::size_t(0));
      REQUIRE(iterator.isSuccess());
      LatestFrameReader reader(iterator.value());
      std::vector<MediaTime> times;
      while(auto frame = reader.next()) {
        times.push_back(frame->timestamp());
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      CHECK(reader.atEnd());
      REQUIRE_FALSE(times.empty());
      for(size_t i = 1; i < times.size(); ++i)
        CHECK(times[i] > times[i - 1]);
      // Decoding outpaces a 5 ms reader, so frames were dropped, and every frame is accounted for.
      CHECK(reader.droppedFrames() > 0);
      CHECK(times.size() + reader.droppedFrames() == 40);
    }
    {
      // Destroying a reader mid-stream stops its thread.
      auto again = container->createIterator(std::size_t(0));
      REQUIRE(again.isSuccess());
      LatestFrameReader reader(again.value());
      CHECK(reader.next());
    }
    std::filesystem::remove(path);
  }

  TEST_CASE("Live capture with URL options", "[Video][Capture][Device]")
  {
    initIO();
    const auto device = firstCaptureDevice();
    if(!device)
      SKIP("No V4L2 capture device on this machine");
    const auto modes = listCaptureModes(*device);
    REQUIRE_FALSE(modes.empty());
    const auto &mode = modes.front();
    INFO("Device " << *device << " modes:\n" << describeCaptureModes(modes));
    const std::string url = "camera://" + *device + "?size=" + std::to_string(mode.width) + "x" + std::to_string(mode.height)
                            + "&format=" + mode.format + "&latest=1";
    auto stream = openRgb(url);
    REQUIRE(stream.valid());
    auto image = stream.get();
    CHECK(image.range() == IndexRange<2>({mode.height, mode.width}));

    // A size no mode has is refused with the modes listed.
    CHECK_THROWS_AS(openRgb("camera://" + *device + "?size=7x7"), std::invalid_argument);
  }

}// namespace Ravl2::Video
