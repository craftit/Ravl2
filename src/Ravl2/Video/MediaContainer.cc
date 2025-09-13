
#include "MediaContainer.hh"
#include "FfmpegMediaContainer.hh"

namespace Ravl2::Video
{
  VideoResult<std::shared_ptr<MediaContainer>> MediaContainer::openFile(const std::string& filePath)
  {
    return FfmpegMediaContainer::openFile(filePath);
  }
}