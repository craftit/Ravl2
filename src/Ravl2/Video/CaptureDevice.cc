//
// Capture device URLs and capabilities, independent of the capture backend.
//

#include "Ravl2/Video/CaptureDevice.hh"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cmath>
#include <map>
#include <stdexcept>
#include <fmt/format.h>

#ifdef __linux__
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace Ravl2::Video
{
  namespace
  {
    std::string lowerCase(std::string_view text)
    {
      std::string result(text);
      std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return char(std::tolower(c)); });
      return result;
    }

    template <typename NumberT>
    NumberT parseNumber(std::string_view text, std::string_view key)
    {
      NumberT value {};
      const auto *end = text.data() + text.size();
      auto [ptr, ec] = std::from_chars(text.data(), end, value);
      if(ec != std::errc() || ptr != end)
        throw std::invalid_argument(fmt::format("Capture URL option '{}': '{}' is not a number", key, text));
      return value;
    }

    bool parseFlag(std::string_view text, std::string_view key)
    {
      const std::string value = lowerCase(text);
      if(value == "1" || value == "true" || value == "yes" || value.empty())
        return true;
      if(value == "0" || value == "false" || value == "no")
        return false;
      throw std::invalid_argument(fmt::format("Capture URL option '{}': '{}' is not 0 or 1", key, text));
    }

    bool isIndex(std::string_view text)
    {
      return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
    }

    //! Frame rates are compared to the nearest 0.1 fps (drivers report e.g. 1/30 s as exactly 30, but some
    //! report 29.97 for "30").
    bool sameRate(float a, float b)
    {
      return std::abs(a - b) < 0.05f * std::max(1.0f, b / 30.0f);
    }
  }// namespace

  CaptureRequest parseCaptureUrl(std::string_view url)
  {
    CaptureRequest request;
    const auto queryAt = url.find('?');
    std::string_view device = url.substr(0, queryAt);
    std::string_view query = queryAt == std::string_view::npos ? std::string_view() : url.substr(queryAt + 1);

    if(isIndex(device)) {
      request.fromIndex = true;
#ifdef __linux__
      request.device.devicePath = fmt::format("/dev/video{}", device);
#else
      request.device.devicePath = std::string(device);
#endif
    } else {
      request.device.devicePath = std::string(device);
    }

    while(!query.empty()) {
      const auto ampAt = query.find('&');
      std::string_view item = query.substr(0, ampAt);
      query = ampAt == std::string_view::npos ? std::string_view() : query.substr(ampAt + 1);
      if(item.empty())
        continue;
      const auto eqAt = item.find('=');
      const std::string key = lowerCase(item.substr(0, eqAt));
      std::string_view value = eqAt == std::string_view::npos ? std::string_view() : item.substr(eqAt + 1);
      if(key == "size") {
        const auto xAt = value.find_first_of("xX");
        if(xAt == std::string_view::npos)
          throw std::invalid_argument(fmt::format("Capture URL option 'size': '{}' is not WIDTHxHEIGHT", value));
        request.device.width = parseNumber<int>(value.substr(0, xAt), "size");
        request.device.height = parseNumber<int>(value.substr(xAt + 1), "size");
        if(request.device.width <= 0 || request.device.height <= 0)
          throw std::invalid_argument(fmt::format("Capture URL option 'size': '{}' must be positive", value));
      } else if(key == "fps") {
        request.device.frameRate = parseNumber<float>(value, "fps");
        if(!(request.device.frameRate > 0))
          throw std::invalid_argument(fmt::format("Capture URL option 'fps': '{}' must be positive", value));
      } else if(key == "format") {
        if(value.empty())
          throw std::invalid_argument("Capture URL option 'format' needs a value, e.g. format=mjpeg");
        request.device.pixelFormat = normaliseCaptureFormat(value);
      } else if(key == "latest") {
        request.latestOnly = parseFlag(value, "latest");
      } else {
        throw std::invalid_argument(fmt::format("Unknown capture URL option '{}' (expected size, fps, format or latest)", key));
      }
    }
    return request;
  }

  std::string captureFormatName(std::string_view fourcc)
  {
    static const std::map<std::string, std::string, std::less<>> names = {
      {"MJPG", "mjpeg"}, {"JPEG", "mjpeg"}, {"YUYV", "yuyv422"}, {"UYVY", "uyvy422"}, {"NV12", "nv12"},
      {"YU12", "yuv420p"}, {"H264", "h264"}, {"HEVC", "hevc"}, {"GREY", "gray"}, {"RGB3", "rgb24"},
      {"BGR3", "bgr24"}};
    std::string key(fourcc);
    while(!key.empty() && key.back() == ' ')
      key.pop_back();
    if(auto it = names.find(key); it != names.end())
      return it->second;
    return lowerCase(key);
  }

  std::string normaliseCaptureFormat(std::string_view name)
  {
    const std::string lower = lowerCase(name);
    if(lower == "yuyv" || lower == "yuy2")
      return "yuyv422";
    if(lower == "uyvy")
      return "uyvy422";
    if(lower == "mjpg" || lower == "jpeg" || lower == "motion-jpeg")
      return "mjpeg";
    if(lower == "grey")
      return "gray";
    return lower;
  }

  std::vector<CaptureMode> listCaptureModes([[maybe_unused]] const std::string &devicePath)
  {
    std::vector<CaptureMode> modes;
#ifdef __linux__
    const int fd = ::open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
    if(fd < 0)
      return modes;
    auto xioctl = [fd](unsigned long request, void *arg) {
      int r;
      do {
        r = ::ioctl(fd, request, arg);
      } while(r < 0 && errno == EINTR);
      return r;
    };
    v4l2_fmtdesc fmtDesc {};
    fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for(fmtDesc.index = 0; xioctl(VIDIOC_ENUM_FMT, &fmtDesc) == 0; ++fmtDesc.index) {
      const char fourcc[5] = {char(fmtDesc.pixelformat & 0xff), char((fmtDesc.pixelformat >> 8) & 0xff),
                              char((fmtDesc.pixelformat >> 16) & 0xff), char((fmtDesc.pixelformat >> 24) & 0xff), 0};
      const std::string format = captureFormatName(fourcc);
      const std::string description = reinterpret_cast<const char *>(fmtDesc.description);

      v4l2_frmsizeenum size {};
      size.pixel_format = fmtDesc.pixelformat;
      for(size.index = 0; xioctl(VIDIOC_ENUM_FRAMESIZES, &size) == 0; ++size.index) {
        // Stepwise/continuous sizes: report the largest and smallest (most drivers for USB cameras are discrete).
        std::vector<std::pair<int, int>> sizes;
        if(size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
          sizes.emplace_back(int(size.discrete.width), int(size.discrete.height));
        } else {
          sizes.emplace_back(int(size.stepwise.min_width), int(size.stepwise.min_height));
          sizes.emplace_back(int(size.stepwise.max_width), int(size.stepwise.max_height));
        }
        for(auto [width, height] : sizes) {
          CaptureMode mode {format, description, width, height, {}};
          v4l2_frmivalenum interval {};
          interval.pixel_format = fmtDesc.pixelformat;
          interval.width = unsigned(width);
          interval.height = unsigned(height);
          for(interval.index = 0; xioctl(VIDIOC_ENUM_FRAMEINTERVALS, &interval) == 0; ++interval.index) {
            if(interval.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
              if(interval.discrete.numerator > 0)
                mode.frameRates.push_back(float(interval.discrete.denominator) / float(interval.discrete.numerator));
            } else {
              // Stepwise: the fastest and slowest rates.
              if(interval.stepwise.min.numerator > 0)
                mode.frameRates.push_back(float(interval.stepwise.min.denominator) / float(interval.stepwise.min.numerator));
              if(interval.stepwise.max.numerator > 0)
                mode.frameRates.push_back(float(interval.stepwise.max.denominator) / float(interval.stepwise.max.numerator));
              break;
            }
          }
          modes.push_back(std::move(mode));
        }
        if(size.type != V4L2_FRMSIZE_TYPE_DISCRETE)
          break;
      }
    }
    ::close(fd);
#endif
    return modes;
  }

  std::optional<std::string> checkCaptureRequest(const std::vector<CaptureMode> &modes, const DeviceParameters &request)
  {
    if(modes.empty())
      return std::nullopt;// Nothing to check against (not V4L2, or the driver won't say): let the backend decide.
    auto formatOk = [&](const CaptureMode &m) { return request.pixelFormat.empty() || m.format == request.pixelFormat; };
    auto sizeOk = [&](const CaptureMode &m) {
      return request.width <= 0 || (m.width == request.width && m.height == request.height);
    };
    auto rateOk = [&](const CaptureMode &m) {
      if(request.frameRate <= 0 || m.frameRates.empty())
        return true;
      return std::any_of(m.frameRates.begin(), m.frameRates.end(), [&](float r) { return sameRate(r, request.frameRate); });
    };
    if(std::any_of(modes.begin(), modes.end(), [&](const CaptureMode &m) { return formatOk(m) && sizeOk(m) && rateOk(m); }))
      return std::nullopt;

    // Name the first setting that can't be met given the ones before it.
    std::string refused;
    if(!std::any_of(modes.begin(), modes.end(), formatOk))
      refused = fmt::format("format={}", request.pixelFormat);
    else if(!std::any_of(modes.begin(), modes.end(), [&](const CaptureMode &m) { return formatOk(m) && sizeOk(m); }))
      refused = request.pixelFormat.empty() ? fmt::format("size={}x{}", request.width, request.height)
                                            : fmt::format("size={}x{} with format={}", request.width, request.height, request.pixelFormat);
    else
      refused = fmt::format("fps={}", request.frameRate)
                + (request.width > 0 ? fmt::format(" at size={}x{}", request.width, request.height) : std::string())
                + (request.pixelFormat.empty() ? std::string() : fmt::format(" with format={}", request.pixelFormat));
    return fmt::format("Capture device '{}' doesn't support {}. It offers:\n{}", request.devicePath, refused, describeCaptureModes(modes));
  }

  std::string describeCaptureModes(const std::vector<CaptureMode> &modes)
  {
    std::string text;
    for(const auto &mode : modes) {
      text += fmt::format("  {} {}x{}", mode.format, mode.width, mode.height);
      if(!mode.frameRates.empty()) {
        text += " @ ";
        for(size_t i = 0; i < mode.frameRates.size(); ++i)
          text += fmt::format("{}{:g}", i == 0 ? "" : ", ", mode.frameRates[i]);
        text += " fps";
      }
      text += "\n";
    }
    return text;
  }

}// namespace Ravl2::Video
