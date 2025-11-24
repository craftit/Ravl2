
#include "MediaContainer.hh"
#include "FfmpegMediaContainer.hh"

namespace Ravl2::Video
{
  VideoResult<std::shared_ptr<MediaContainer>> MediaContainer::openFile(const std::string& filePath)
  {
    return FfmpegMediaContainer::openFile(filePath);
  }

  VideoResult<std::shared_ptr<MediaContainer>> MediaContainer::openDevice(const DeviceParameters& params)
  {
    return FfmpegMediaContainer::openDevice(params);
  }

  VideoResult<std::vector<DeviceInfo>> enumerateDevices()
  {
    return FfmpegMediaContainer::enumerateDevices();
  }
}