
#include <unordered_map>
#include <spdlog/spdlog.h>
#include "MediaContainer.hh"
#include "FfmpegMediaContainer.hh"
#include "Ravl2/Pixel/PixelPlane.hh"

namespace Ravl2::Video
{
  namespace
  {
    auto &streamsMap()
    {
      static std::unordered_map<std::type_index, Video::StreamType>  streamTypeMap = {
        {typeid(RGBPlanarImage<uint8_t>), Video::StreamType::Video },
        {typeid(RGBAPlanarImage<uint8_t>), Video::StreamType::Video},
        {typeid(YUV420Image<uint8_t>), Video::StreamType::Video},
        {typeid(YUV420Image<uint16_t>), Video::StreamType::Video},
        {typeid(YUV422Image<uint8_t>), Video::StreamType::Video},
        {typeid(YUV444Image<uint8_t>), Video::StreamType::Video},
        {typeid(Array<PixelYUYV8, 2>), Video::StreamType::Video},
        {typeid(Array<PixelUYVY8, 2>), Video::StreamType::Video},
        {typeid(Array<PixelI8, 2>), Video::StreamType::Video},
        {typeid(AudioChunk<Array<uint8_t, 2>>), Video::StreamType::Audio},
        {typeid(AudioChunk<Array<int16_t, 2>>), Video::StreamType::Audio},
        {typeid(AudioChunk<Array<int32_t, 2>>), Video::StreamType::Audio},
        {typeid(AudioChunk<Array<float, 2>>), Video::StreamType::Audio},
        {typeid(AudioChunk<Array<double, 2>>), Video::StreamType::Audio}
      };
      return streamTypeMap;
    }

  }// namespace

  bool MediaContainer::registerDataType(const std::type_index &type, StreamType streamType)
  {
    auto &typeMap = streamsMap();
    auto it = typeMap.find(type);
    if(it == typeMap.end()) {
      typeMap[type] = streamType;
      return true;
    }
    if(streamType != it->second) {
      SPDLOG_ERROR("Mismatched data registration. {} -> {} ", typeName(type), toString(streamType));
      return false;
    }
    return true;
  }

  VideoResult<std::shared_ptr<MediaContainer>> MediaContainer::openFile(const std::string& filePath)
  {
    return FfmpegMediaContainer::openFile(filePath);
  }

  VideoResult<std::shared_ptr<MediaContainer>> MediaContainer::openDevice(const DeviceParameters& params)
  {
    return FfmpegMediaContainer::openDevice(params);
  }

  //! Create an iterator for a specific stream with a target type
  VideoResult<std::shared_ptr<StreamIterator>> MediaContainer::createIterator(const std::type_info &dataType,std::size_t streamIndex)
  {
    (void) dataType;
    if(streamIndex < streamCount()) {
      return createIterator(streamIndex);
    }
    auto &streamTypes = streamsMap();
    auto it = streamTypes.find(std::type_index(dataType));
    if(it != streamTypes.end()) {
      StreamType targetStreamType = it->second;
      //! Get the number of streams in the container
      std::size_t strmCount = streamCount();
      for(std::size_t i = 0; i < strmCount; i++) {
        if(streamType(i) == targetStreamType) {
          return createIterator(i);
        }
      }
    }
    return VideoResult<std::shared_ptr<StreamIterator>>(VideoErrorCode::UnsupportedFormat);
  }

  VideoResult<std::vector<DeviceInfo>> enumerateDevices()
  {
    return FfmpegMediaContainer::enumerateDevices();
  }


}