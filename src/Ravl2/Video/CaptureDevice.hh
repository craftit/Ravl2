//
// Capture device URLs and capabilities, independent of the capture backend.
//
// URL forms are documented in CaptureDevices.md (next to this file).
//

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Ravl2/Video/VideoTypes.hh"

namespace Ravl2::Video
{
  //! A capture request parsed from a RavlIO 'camera://' URL.
  struct CaptureRequest
  {
    //! Device and requested mode. devicePath is a platform path ("/dev/video8") with the query removed; a bare
    //! index ("8") becomes "/dev/video8" on Linux and stays an index elsewhere (AVFoundation uses indices).
    DeviceParameters device;
    //! True if the device was given as an index rather than a path.
    bool fromIndex = false;
    //! Deliver only the newest frame, dropping any the reader was too slow to take ('latest=1').
    bool latestOnly = false;
  };

  //! Parse what follows "camera://": "<device>[?key=value&...]".
  //! Keys: size=WxH, fps=N, format=NAME (FFmpeg/V4L2 name, e.g. mjpeg, yuyv422, nv12, h264), latest=0|1.
  //! @throws std::invalid_argument for an unknown key or a malformed value, naming it.
  [[nodiscard]] CaptureRequest parseCaptureUrl(std::string_view url);

  //! One capture mode a device offers: a format at a frame size, with the frame rates it supports there.
  struct CaptureMode
  {
    std::string format;           //!< FFmpeg name, e.g. "mjpeg", "yuyv422" (see captureFormatName())
    std::string description;      //!< Driver's description, e.g. "Motion-JPEG"
    int width = 0;
    int height = 0;
    std::vector<float> frameRates;//!< Supported rates in frames per second; empty if the driver doesn't say
  };

  //! FFmpeg name for a V4L2 fourcc: MJPG -> mjpeg, YUYV -> yuyv422, NV12 -> nv12, H264 -> h264, ...;
  //! unknown fourccs are lower-cased.
  [[nodiscard]] std::string captureFormatName(std::string_view fourcc);

  //! Canonical form of a user-supplied format name ("yuyv" -> "yuyv422", "mjpg"/"jpeg" -> "mjpeg", lower case).
  [[nodiscard]] std::string normaliseCaptureFormat(std::string_view name);

  //! List the modes a V4L2 device offers (VIDIOC_ENUM_FMT / ENUM_FRAMESIZES / ENUM_FRAMEINTERVALS).
  //! Empty if the device can't be opened or queried, and on platforms without V4L2.
  [[nodiscard]] std::vector<CaptureMode> listCaptureModes(const std::string &devicePath);

  //! Check a request against a device's modes.
  //! @return std::nullopt if some mode satisfies every requested (non-zero / non-empty) setting, otherwise a
  //! message naming the first setting no mode satisfies and listing what the device offers.
  [[nodiscard]] std::optional<std::string> checkCaptureRequest(const std::vector<CaptureMode> &modes, const DeviceParameters &request);

  //! One line per format and size, with its frame rates: "mjpeg 1280x720 @ 60, 30 fps".
  [[nodiscard]] std::string describeCaptureModes(const std::vector<CaptureMode> &modes);

}// namespace Ravl2::Video
