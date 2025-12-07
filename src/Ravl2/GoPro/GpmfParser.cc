//
// Created for RAVL2 GoPro metadata support
//

#include "Ravl2/GoPro/GpmfParser.hh"
#include "Ravl2/Video/VideoTypes.hh"

#include "Ravl2/Assert.hh"
#include "Ravl2/Logging.hh"

// Include GPMF parser C headers
extern "C" {
#include <GPMF_parser.h>
}

#include <nlohmann/json.hpp>
#include <cstring>
#include <algorithm>

namespace Ravl2::GoPro
{
  std::string toString(GPMF_SampleType stype)
  {
    switch(stype) {
      case GPMF_TYPE_NEST: return ">";
      case GPMF_TYPE_EMPTY: return "Empty";
      case GPMF_TYPE_ERROR: return "Error";
      default: {
        std::string ret;
        ret += static_cast<char>(stype);
        return ret;
      }
    }
  }

  //! Convert GPMF ASCII string to UTF-8
  //! GPMF uses extended ASCII with special characters (per spec):
  //! 0xB0 (°), 0xB2 (²), 0xB3 (³), 0xB5 (µ)
  static std::string gpmfAsciiToUtf8(const char *data, size_t size)
  {
    std::string result;
    result.reserve(size * 2);// Reserve extra space for multi-byte UTF-8

    for(size_t i = 0; i < size; i++) {
      auto ch = static_cast<unsigned char>(data[i]);

      if(ch == 0) break;// Null terminator

      switch(ch) {
        case 0xB0:// ° (degree symbol)
          result += "\u00B0";
          break;
        case 0xB2:// ² (superscript 2)
          result += "\u00B2";
          break;
        case 0xB3:// ³ (superscript 3)
          result += "\u00B3";
          break;
        case 0xB5:// µ (micro symbol)
          result += "\u00B5";
          break;
        default:
          if(ch < 128) {
            result += static_cast<char>(ch);
          } else {
            // Unknown extended ASCII - skip or replace with ?
            SPDLOG_WARN("Unknown extended ASCII character: 0x{:02X} in GPMF string", ch);
            result += '?';
          }
          break;
      }
    }
    return result;
  }

  GpmfParser::GpmfParser(bool enableJson)
      : mEnableJson(enableJson)
  {
  }

  std::vector<std::shared_ptr<Video::Frame>> GpmfParser::parse(
    const uint8_t *data,
    size_t size,
    Video::StreamItemId streamId,
    Video::MediaTime timestamp)
  {
    std::vector<std::shared_ptr<Video::Frame>> frames;

    if(data == nullptr || size == 0) {
      SPDLOG_DEBUG("parse() called with null/empty data");
      return frames;
    }

    // Initialize GPMF stream
    // Note: GPMF_Init takes buffer size in BYTES (despite the uint32_t* pointer type)
    GPMF_stream stream;
    GPMF_ERR initResult = GPMF_Init(&stream, const_cast<uint32_t *>(reinterpret_cast<const uint32_t *>(data)), static_cast<uint32_t>(size));
    if(initResult != GPMF_OK) {
      SPDLOG_WARN("Failed to initialize GPMF stream (error={}), data size={} bytes", static_cast<int>(initResult), size);
      return frames;
    }

    // Validate the stream structure
    GPMF_ERR validateResult = GPMF_Validate(&stream, static_cast<GPMF_LEVELS>(GPMF_RECURSE_LEVELS | GPMF_TOLERANT));
    if(validateResult != GPMF_OK) {
      SPDLOG_WARN("GPMF stream validation failed (error={})", static_cast<int>(validateResult));
      return frames;
    }

    // Start processing at the top level
    GPMF_ResetState(&stream);

    frames = processLevel(&stream, streamId, timestamp, 0);
    if(mVerbose) {
      SPDLOG_INFO("Generated {} frames.",frames.size());
    }
    return frames;
  }

  void GpmfParser::parseGps(GPMF_stream *stream, std::vector<std::shared_ptr<Video::Frame>> &frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    SPDLOG_INFO("Extracting GPS.");
    if(stream == nullptr) {
      SPDLOG_WARN("parseGps: null stream pointer");
      return;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if(sampleCount == 0) {
      SPDLOG_DEBUG("parseGps: GPS stream has 0 samples");
      return;
    }

    // Determine GPS format from number of elements
    uint32_t elements = GPMF_ElementsInStruct(stream);
    uint32_t fourcc = GPMF_Key(stream);

    // GPS5 format: latitude, longitude, altitude, 2D speed, 3D speed (5 values)
    // GPS9 format: latitude, longitude, altitude, 2D speed, 3D speed, days, secs, DOP, fix (9 values)
    if(elements != 5 && elements != 9) {
      SPDLOG_WARN("parseGps: unexpected element count {} for GPS data (expected 5 or 9)", elements);
      return;
    }

    // All values are scaled integers
    float scale = getScaleFactor(stream, fourcc);
    if(scale == 0) {
      SPDLOG_WARN("parseGps: invalid scale factor, using default 1.0");
      scale = 1.0f;
    }

    // Get sample rate
    float sampleRate = getSampleRate(stream);

    // Get raw data - GPS5 uses int32_t (GPMF_TYPE_SIGNED_LONG 'l'), not int16_t!
    int32_t *rawData = static_cast<int32_t *>(GPMF_RawData(stream));
    if(rawData == nullptr) {
      SPDLOG_ERROR("parseGps: failed to get raw data from GPMF stream (sampleCount={})", sampleCount);
      return;
    }

    // Validate sample rate
    if(sampleRate <= 0.0F) {
      SPDLOG_ERROR("Invalid GPS sample rate: {} Hz (must be > 0)", sampleRate);
      sampleRate = 0.0F;
    } else if(sampleRate < 1.0F || sampleRate > 20.0F) {
      SPDLOG_WARN("Unusual GPS sample rate: {} Hz (typical GoPro: 1-18 Hz)", sampleRate);
    }

    // Calculate time delta between samples for timestamp interpolation
    Video::MediaTime timeDelta(0);
    if(sampleRate > 0.0F && sampleCount > 1) {
      // Convert sample rate to microseconds per sample
      int64_t deltaUs = static_cast<int64_t>((1.0F / sampleRate) * 1000000.0F);
      timeDelta = Video::MediaTime(deltaUs);
    }

    // Parse ALL GPS samples and create individual frames
    for(uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * elements;// elements = 5 for GPS5, 9 for GPS9

      // IMPORTANT: GPMF data is big-endian, must byte-swap all int32_t values!
      // Common fields (present in both GPS5 and GPS9):
      double latitude = static_cast<double>(BYTESWAP32(rawData[offset + 0])) * static_cast<double>(scale);
      double longitude = static_cast<double>(BYTESWAP32(rawData[offset + 1])) * static_cast<double>(scale);
      double altitude = static_cast<double>(BYTESWAP32(rawData[offset + 2])) * static_cast<double>(scale);
      float speed2d = static_cast<float>(BYTESWAP32(rawData[offset + 3])) * scale;
      float speed3d = static_cast<float>(BYTESWAP32(rawData[offset + 4])) * scale;

      // Create GpsFix
      GpsFix fix;

      // GPS9-specific fields (if available):
      if(elements == 9) {
        // GPS9 additional fields: days, secs, DOP, fix
        fix.days = BYTESWAP32(rawData[offset + 5]);   // Not used yet
        fix.seconds = BYTESWAP32(rawData[offset + 6]);// Not used yet
        int32_t dopRaw = BYTESWAP32(rawData[offset + 7]);
        fix.precision = static_cast<float>(dopRaw) * scale;
        // Fix type is NOT scaled - it's a raw integer (0=no lock, 2=2D, 3=3D)
        fix.fix = BYTESWAP32(rawData[offset + 8]);
      }

      // Create GPSCoordinate

      fix.location = GPSCoordinate(latitude, longitude, altitude);
      fix.speed = Point<float, 2>(speed2d, speed3d);
      fix.satellites = -1;// Not available in either GPS5 or GPS9

      // Interpolate the timestamp for this specific fix
      Video::MediaTime fixTimestamp = timestamp;
      if(timeDelta.count() > 0) {
        fixTimestamp = timestamp + Video::MediaTime(timeDelta.count() * static_cast<int64_t>(i));
      }

      // Create and append frame
      frames.push_back(std::make_shared<Video::FrameData<GpsFix>>(
        fix,
        streamId + mNextId++,
        fixTimestamp,
        Ravl2::Video::StreamType::Data
        ));
    }

    SPDLOG_DEBUG("Created {} GPS frames from GPMF packet at timestamp {} μs", sampleCount, timestamp.count());
  }

  void GpmfParser::parseGps9Complex(GPMF_stream *stream, std::vector<std::shared_ptr<Video::Frame>> &frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    if(mVerbose) {
      SPDLOG_INFO("Extracting GPS9 (complex type with TYPE descriptor).");
    }
    if(stream == nullptr) {
      SPDLOG_WARN("parseGps9Complex: null stream pointer");
      return;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if(sampleCount == 0) {
      SPDLOG_DEBUG("parseGps9Complex: GPS9 stream has 0 samples");
      return;
    }

    // Get TYPE descriptor - should be "lllllllSS" for GPS9
    std::string typeDesc = getTypeDescriptor(stream);
    if(typeDesc.empty()) {
      SPDLOG_ERROR("parseGps9Complex: no TYPE descriptor found for GPS9 complex type");
      return;
    }

    // Validate TYPE descriptor matches expected GPS9 format
    if(typeDesc != "lllllllSS") {
      SPDLOG_ERROR("parseGps9Complex: unexpected TYPE descriptor \"{}\" (expected \"lllllllSS\")", typeDesc);
      return;
    }

    // Get SCAL (scale) values - should be 9 values for GPS9
    std::vector<int32_t> scales = getScaleFactors(stream, 9);
    if(scales.empty()) {
      SPDLOG_WARN("parseGps9Complex: no SCAL found, using default scale of 1 for all fields");
      scales.resize(9, 1);
    } else if(scales.size() != 9) {
      SPDLOG_WARN("parseGps9Complex: expected 9 scale values, found {}, padding with 1", scales.size());
      scales.resize(9, 1);
    }

    // Get sample rate
    float sampleRate = getSampleRate(stream);

    // Calculate time delta between samples for timestamp interpolation
    Video::MediaTime timeDelta(0);
    if(sampleRate > 0.0F && sampleCount > 1) {
      int64_t deltaUs = static_cast<int64_t>((1.0F / sampleRate) * 1000000.0F);
      timeDelta = Video::MediaTime(deltaUs);
    }

    // Get raw data
    auto *rawData = static_cast<uint8_t *>(GPMF_RawData(stream));
    if(rawData == nullptr) {
      SPDLOG_ERROR("parseGps9Complex: failed to get raw data from GPMF stream (sampleCount={})", sampleCount);
      return;
    }

    // Get sample size - should be 32 bytes (7 longs = 28 bytes + 2 shorts = 4 bytes)
    uint32_t sampleSize = GPMF_StructSize(stream);
    if(sampleSize != 32) {
      SPDLOG_ERROR("parseGps9Complex: unexpected sample size {} bytes (expected 32 for \"lllllllSS\")", sampleSize);
      return;
    }

    // Parse ALL GPS9 samples and create individual frames
    for(uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * sampleSize;

      // Parse according to TYPE descriptor "lllllllSS":
      // 7 signed 32-bit longs (l) = 28 bytes
      auto *longPtr = reinterpret_cast<const int32_t *>(rawData + offset);

      // Field 0: Latitude (scale by SCAL[0])
      int32_t latRaw = BYTESWAP32(longPtr[0]);
      double latitude = static_cast<double>(latRaw) / static_cast<double>(scales[0]);

      // Field 1: Longitude (scale by SCAL[1])
      int32_t lonRaw = BYTESWAP32(longPtr[1]);
      double longitude = static_cast<double>(lonRaw) / static_cast<double>(scales[1]);

      // Field 2: Altitude (scale by SCAL[2])
      int32_t altRaw = BYTESWAP32(longPtr[2]);
      double altitude = static_cast<double>(altRaw) / static_cast<double>(scales[2]);

      // Field 3: 2D Speed (scale by SCAL[3])
      int32_t speed2dRaw = BYTESWAP32(longPtr[3]);
      float speed2d = static_cast<float>(speed2dRaw) / static_cast<float>(scales[3]);

      // Field 4: 3D Speed (scale by SCAL[4])
      int32_t speed3dRaw = BYTESWAP32(longPtr[4]);
      float speed3d = static_cast<float>(speed3dRaw) / static_cast<float>(scales[4]);

      // Field 5: Days since 2000-01-01 (scale by SCAL[5])
      int32_t daysRaw = BYTESWAP32(longPtr[5]);
      int32_t days = daysRaw / scales[5];

      // Field 6: Seconds (scale by SCAL[6])
      int32_t secsRaw = BYTESWAP32(longPtr[6]);
      int32_t seconds = secsRaw / scales[6];

      // 2 unsigned 16-bit shorts (S) = 4 bytes
      offset += 7 * 4;// Advance past 7 longs
      auto *shortPtr = reinterpret_cast<const uint16_t *>(rawData + offset);

      // Field 7: DOP (Dilution of Precision) (scale by SCAL[7])
      uint16_t dopRaw = BYTESWAP16(shortPtr[0]);
      float dop = static_cast<float>(dopRaw) / static_cast<float>(scales[7]);

      // Field 8: Fix type (0=no lock, 2=2D, 3=3D) (scale by SCAL[8], but typically 1)
      uint16_t fixRaw = BYTESWAP16(shortPtr[1]);
      uint16_t fixType = fixRaw / static_cast<uint16_t>(scales[8]);

      // Create GpsFix
      GpsFix fix;
      fix.location = GPSCoordinate(latitude, longitude, altitude);
      fix.speed = Point<float, 2>(speed2d, speed3d);
      fix.days = days;
      fix.seconds = seconds;
      fix.precision = dop;
      fix.fix = fixType;
      fix.satellites = -1;// Not available in GPS9

      // Interpolate the timestamp for this specific fix
      Video::MediaTime fixTimestamp = timestamp;
      if(timeDelta.count() > 0) {
        fixTimestamp = timestamp + Video::MediaTime(timeDelta.count() * static_cast<int64_t>(i));
      }

      // Create and append frame
      frames.push_back(std::make_shared<Video::FrameData<GpsFix>>(
        fix,
        streamId + mNextId++,
        fixTimestamp,
        Video::StreamType::Data));
    }

    SPDLOG_DEBUG("Created {} GPS9 frames from GPMF packet at timestamp {} μs", sampleCount, timestamp.count());
  }

  void GpmfParser::parseGyro(GPMF_stream *stream, std::vector<std::shared_ptr<Video::Frame>> &frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    if(stream == nullptr) {
      SPDLOG_WARN("parseGyro: null stream pointer");
      return;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if(sampleCount == 0) {
      SPDLOG_DEBUG("parseGyro: GYRO stream has 0 samples");
      return;
    }

    // Get scale factor
    float scale = getScaleFactor(stream, MAKEID('G', 'Y', 'R', 'O'));
    if(scale == 0) {
      SPDLOG_WARN("parseGyro: invalid scale factor, using default 1.0");
      scale = 1.0f;
    }

    // Get sample rate
    float sampleRate = getSampleRate(stream);

    // Get raw data (3 int16 values per sample: x, y, z)
    auto *rawData = static_cast<int16_t *>(GPMF_RawData(stream));
    if(rawData == nullptr) {
      SPDLOG_ERROR("parseGyro: failed to get raw data from GPMF stream (sampleCount={})", sampleCount);
      return;
    }

    std::vector<Vector3f> samples;
    samples.reserve(sampleCount);
    for(uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * 3;
      // IMPORTANT: GPMF data is big-endian, must byte-swap all int16_t values!
      samples.emplace_back(
        static_cast<float>(BYTESWAP16(rawData[offset + 0])) * scale,
        static_cast<float>(BYTESWAP16(rawData[offset + 1])) * scale,
        static_cast<float>(BYTESWAP16(rawData[offset + 2])) * scale);
    }

    // Validate sample rate
    if(sampleRate <= 0.0F) {
      SPDLOG_ERROR("Invalid gyro sample rate: {} Hz (must be > 0)", sampleRate);
      sampleRate = 0.0F;
    } else if(sampleRate < 50.0F || sampleRate > 1000.0F) {
      if(!mHaveReportedGyroFPS) {
        mHaveReportedGyroFPS = true;
        SPDLOG_WARN("Unusual gyro sample rate: {} Hz (typical GoPro: 200-400 Hz)", sampleRate);
      }
    }

    // Create and append frame
    frames.push_back(std::make_shared<Video::FrameData<GyroSamples>>(
      GyroSamples(samples, sampleRate),
      streamId + mNextId++,
      timestamp,
      Video::StreamType::Data));
  }

  void GpmfParser::parseAccel(GPMF_stream *stream, std::vector<std::shared_ptr<Video::Frame>> &frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    if(stream == nullptr) {
      SPDLOG_WARN("parseAccel: null stream pointer");
      return;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if(sampleCount == 0) {
      SPDLOG_DEBUG("parseAccel: ACCL stream has 0 samples");
      return;
    }

    // Get scale factor
    float scale = getScaleFactor(stream, MAKEID('A', 'C', 'C', 'L'));
    if(scale == 0) {
      SPDLOG_WARN("parseAccel: invalid scale factor, using default 1.0");
      scale = 1.0f;
    }

    // Get sample rate
    float sampleRate = getSampleRate(stream);

    // Get raw data (3 int16 values per sample: x, y, z)
    auto *rawData = static_cast<int16_t *>(GPMF_RawData(stream));
    if(rawData == nullptr) {
      SPDLOG_ERROR("parseAccel: failed to get raw data from GPMF stream (sampleCount={})", sampleCount);
      return;
    }

    std::vector<Vector3f> samples;
    samples.reserve(sampleCount);
    for(uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * 3;
      // IMPORTANT: GPMF data is big-endian, must byte-swap all int16_t values!
      samples.emplace_back(
        static_cast<float>(BYTESWAP16(rawData[offset + 0])) * scale,
        static_cast<float>(BYTESWAP16(rawData[offset + 1])) * scale,
        static_cast<float>(BYTESWAP16(rawData[offset + 2])) * scale);
    }

    // Validate sample rate
    if(sampleRate <= 0.0F) {
      SPDLOG_ERROR("Invalid accel sample rate: {} Hz (must be > 0)", sampleRate);
      sampleRate = 0.0F;
    } else if(sampleRate < 50.0F || sampleRate > 1000.0F) {
      if(!mHaveReportedAccelFPS) {
        SPDLOG_WARN("Unusual accel sample rate: {} Hz (typical GoPro: 200-400 Hz)", sampleRate);
        mHaveReportedAccelFPS = true;
      }
    }

    // Create and append frame
    frames.push_back(std::make_shared<Video::FrameData<AccelSamples>>(
      AccelSamples(samples, sampleRate),
      streamId + mNextId++,
      timestamp,
      Video::StreamType::Data));
  }

  std::vector<std::shared_ptr<Video::Frame>> GpmfParser::processLevel(GPMF_stream *levelStream, Video::StreamItemId streamId, Video::MediaTime timestamp,  int level)
  {
    std::vector<std::shared_ptr<Video::Frame>> frames;
    if(mVerbose) {
      SPDLOG_INFO("Processing level {}  Type:{}  NestLevel:{} Type:{} ",level, level, GPMF_NestLevel(levelStream), toString(GPMF_Type(levelStream)));
    }

    uint32_t lastFourcc = GPMF_Key(levelStream);

    // Typically, each 'STRM' section ends with the type.
    bool processed = false;
    do {
      uint32_t fourcc = GPMF_Key(levelStream);
      lastFourcc = fourcc;
      if(mVerbose) {
        SPDLOG_INFO("fourcc: {}  Type:{} ", fourccToString(fourcc),toString(GPMF_Type(levelStream)));
      }
      // Process known sensor data types
      switch(fourcc) {
        case MAKEID('G', 'P', 'S', '5'):
        case MAKEID('G', 'P', 'S', '9'):
          {
            // Detect GPS format by checking the sample type
            // Hero 8: GPS5/GPS9 are type 'l' (signed long) - simple arrays
            // Hero 13: GPS9 is type '?' (complex) with TYPE descriptor "lllllllSS"
            GPMF_SampleType sampleType = GPMF_Type(levelStream);

            if(sampleType == GPMF_TYPE_COMPLEX) {
              // Hero 13 format: GPS9 as complex type with TYPE descriptor
              SPDLOG_DEBUG("Detected GPS complex type (Hero 13+ format)");
              parseGps9Complex(levelStream, frames, streamId, timestamp);
            } else {
              // Hero 8 format: GPS5/GPS9 as simple 'l' (int32) arrays
              SPDLOG_DEBUG("Detected GPS simple type (Hero 8 format)");
              parseGps(levelStream, frames, streamId, timestamp);
            }
            processed = true;
          }
          break;

        case MAKEID('G', 'Y', 'R', 'O'):
          parseGyro(levelStream, frames, streamId, timestamp);
          processed = true;
          break;

        case MAKEID('A', 'C', 'C', 'L'):
          parseAccel(levelStream, frames, streamId, timestamp);
          processed = true;
          break;
        case MAKEID('L', 'O', 'G', 'S'):
          // These are 'health logs', GoPro internal.
          // we don't need them for the moment.
          processed = true;
          break;

        // DEVC: enter and process children
        case MAKEID('D', 'E', 'V', 'C'): {
          // Process DEVC children (STRM containers, etc.)
          GPMF_stream devcStream = *levelStream;
          if(GPMF_OK == GPMF_Next(&devcStream, GPMF_RECURSE_LEVELS)) {
            auto subFrames = processLevel(&devcStream, streamId, timestamp, level+1);
            frames.insert(frames.end(), subFrames.begin(), subFrames.end());
          }
          processed = true;
        } break;

#if 1
        // STRM: enter and process children
        case MAKEID('S', 'T', 'R', 'M'): {
          // Process STRM children (GPS9, GYRO, ACCL, etc.)
          GPMF_stream strmStream = *levelStream;
          if(GPMF_OK == GPMF_Next(&strmStream, GPMF_RECURSE_LEVELS)) {
            auto subFrames = processLevel(&strmStream, streamId, timestamp, level+1);
            frames.insert(frames.end(), subFrames.begin(), subFrames.end());
          }
          processed = true;
        } break;
#endif

        default:
          // Unknown FourCC - convert to JSON if enabled
          break;
      }
    } while(GPMF_OK == GPMF_Next(levelStream, GPMF_CURRENT_LEVEL));

    if(!processed) {
      if(mEnableJson) {

        // Check if this is a data entry (has samples)
        uint32_t sampleCount = GPMF_Repeat(levelStream);
        if(sampleCount > 0) {
          nlohmann::json unknownJson;
          unknownJson["fourcc"] = fourccToString(lastFourcc);
          unknownJson["type"] = std::string(1, static_cast<char>(GPMF_Type(levelStream)));
          unknownJson["sample_count"] = sampleCount;
          unknownJson["elements"] = GPMF_ElementsInStruct(levelStream);

          // Add metadata if available
          std::string streamName = getStreamName(levelStream);
          if(!streamName.empty()) {
            unknownJson["stream_name"] = streamName;
          }

          std::string siUnits = getSiUnits(levelStream);
          if(!siUnits.empty()) {
            unknownJson["units"] = siUnits;
          }

          std::vector<std::string> units = getUnits(levelStream);
          if(!units.empty()) {
            unknownJson["field_units"] = units;
          }

          unknownJson["samples"] = samplesToJson(levelStream, lastFourcc, level+1);

          // Create JSON frame
          auto jsonFrame = std::make_shared<Video::FrameData<nlohmann::json>>(
            unknownJson,
            streamId + mNextId++,
            timestamp,
            Video::StreamType::Data);
          frames.push_back(jsonFrame);
        }
      }
    }

    if(mVerbose) {
      SPDLOG_INFO("Finished level {} ",level);
    }
    return frames;
  }

  float GpmfParser::getScaleFactor(GPMF_stream *stream, [[maybe_unused]] uint32_t fourcc) const
  {
    if(stream == nullptr) {
      return 1.0f;
    }

    // Save the current position
    GPMF_stream tempStream = *stream;

    // Look for SCAL (scale) field at the current level (sibling of current FourCC)
    // GPMF_RECURSE_LEVELS allows searching within the current container
    if(GPMF_FindPrev(&tempStream, MAKEID('S', 'C', 'A', 'L'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *scaleData = static_cast<uint32_t *>(GPMF_RawData(&tempStream));
      if(scaleData != nullptr) {
        uint32_t scaleCount = GPMF_Repeat(&tempStream);
        if(scaleCount > 0) {
          // Scale is typically stored as an integer divisor
          // For multi-component data (e.g., XYZ), SCAL may have multiple values
          // Use the first scale value (they're usually all the same for sensor data)
          // IMPORTANT: GPMF data is big-endian, must byte-swap!
          uint32_t scaleDivisor = BYTESWAP32(scaleData[0]);
          float scale = 1.0f / static_cast<float>(scaleDivisor);
          SPDLOG_DEBUG("getScaleFactor: found SCAL with {} values, using first: 1/{} = {}",
                       scaleCount, scaleDivisor, scale);
          return scale;
        }
      }
    }

    SPDLOG_DEBUG("getScaleFactor: no SCAL found at current level, using default 1.0");
    return 1.0f;
  }

  float GpmfParser::getSampleRate(GPMF_stream *stream) const
  {
    if(stream == nullptr) {
      return 0.0f;
    }

    uint32_t fourcc = GPMF_Key(stream);

    // Save the current position
    GPMF_stream tempStream = *stream;

    // Strategy 1: Calculate from STMP (start time) differences between packets
    // STMP is the arrival time in microseconds for the first sample
    // TSMP is the number of samples in this packet
    uint64_t currentStmp = 0;
    uint32_t currentTsmp = 0;
    bool hasStmp = false;
    bool hasTsmp = false;

    // Look for STMP (start time in microseconds)
    if(GPMF_FindPrev(&tempStream, MAKEID('S', 'T', 'M', 'P'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *stmpData = static_cast<uint64_t *>(GPMF_RawData(&tempStream));
      if(stmpData != nullptr) {
        // STMP is type 'J' (64-bit unsigned), big-endian
        currentStmp = BYTESWAP64(stmpData[0]);
        hasStmp = true;
        if(mVerbose) {
          SPDLOG_INFO("Found STMP: {} μs for FourCC: {}", currentStmp, fourccToString(fourcc));
        }
      }
    }

    // Reset temp stream
    tempStream = *stream;

    // Get sample count from the data repeat field (not TSMP, which may be cumulative)
    currentTsmp = GPMF_Repeat(stream);
    hasTsmp = (currentTsmp > 0);

    // Also log TSMP for comparison (for debugging)
    tempStream = *stream;
    if(GPMF_FindPrev(&tempStream, MAKEID('T', 'S', 'M', 'P'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *tsmpData = static_cast<uint32_t *>(GPMF_RawData(&tempStream));
      if(tsmpData != nullptr) {
        uint32_t tsmpValue = BYTESWAP32(tsmpData[0]);
        if(mVerbose) {
          SPDLOG_INFO("Found TSMP: {} (repeat count: {}) for FourCC: {}", tsmpValue, currentTsmp, fourccToString(fourcc));
        }
      }
    } else {
      if(mVerbose) {
        SPDLOG_INFO("Using repeat count: {} samples for FourCC: {}", currentTsmp, fourccToString(fourcc));
      }
    }

    // If we have both STMP and TSMP, calculate rate from timing deltas
    if(hasStmp && hasTsmp) {
      auto& timingInfo = mTimingInfo[fourcc];

      if(timingInfo.hasData && currentStmp > timingInfo.lastStmp) {
        // Calculate time delta in seconds
        double timeDelta = static_cast<double>(currentStmp - timingInfo.lastStmp) / 1000000.0;

        // Calculate sample rate: samples / time
        // TSMP tells us how many samples are in THIS packet (currentTsmp)
        // BUT those samples were collected between lastStmp and currentStmp
        // So we use lastSampleCount (samples from the previous packet that filled the time gap)
        if(timeDelta > 0.0 && timingInfo.lastSampleCount > 0) {
          float calculatedRate = static_cast<float>(timingInfo.lastSampleCount) / static_cast<float>(timeDelta);
          timingInfo.calculatedRate = calculatedRate;
          if(mVerbose) {
            SPDLOG_INFO("Calculated sample rate for {}: {:.2f} Hz (Δt={:.6f}s, samples={})",
                        fourccToString(fourcc), calculatedRate, timeDelta, timingInfo.lastSampleCount);
          }
        }
      }

      // Update timing info for next packet
      timingInfo.lastStmp = currentStmp;
      timingInfo.lastSampleCount = currentTsmp;
      timingInfo.hasData = true;

      // Return calculated rate if we have one
      if(timingInfo.calculatedRate > 0.0f) {
        return timingInfo.calculatedRate;
      }
    }

    // Strategy 2: Fallback to typical rates for known sensor types
    // This is used for the first packet before we can calculate from deltas
    if(fourcc == MAKEID('G', 'Y', 'R', 'O')) {
      SPDLOG_DEBUG("Using default gyro rate: 200 Hz (will calculate from STMP on next packet)");
      return 200.0f;
    }
    if(fourcc == MAKEID('A', 'C', 'C', 'L')) {
      SPDLOG_DEBUG("Using default accel rate: 200 Hz (will calculate from STMP on next packet)");
      return 200.0f;
    }
    if(fourcc == MAKEID('G', 'P', 'S', '5') || fourcc == MAKEID('G', 'P', 'S', '9')) {
      SPDLOG_DEBUG("Using default GPS rate: 18 Hz (will calculate from STMP on next packet)");
      return 18.0f;
    }

    // Fallback: return 0 to indicate unknown
    SPDLOG_WARN("Could not determine sample rate for FourCC: {}, returning 0", fourccToString(fourcc));
    return 0.0f;
  }

  std::string GpmfParser::getTypeDescriptor(GPMF_stream *stream) const
  {
    if(stream == nullptr) {
      return "";
    }

    // Save the current position
    GPMF_stream tempStream = *stream;

    // Look for TYPE field at the current level (sibling of current FourCC)
    if(GPMF_FindPrev(&tempStream, MAKEID('T', 'Y', 'P', 'E'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *typeData = static_cast<char *>(GPMF_RawData(&tempStream));
      uint32_t typeSize = GPMF_RawDataSize(&tempStream);
      if(typeData != nullptr && typeSize > 0) {
        // TYPE is stored as ASCII string (e.g., "lllllllSS")
        std::string typeDesc(typeData, typeSize);
        // Remove any null terminators or padding
        typeDesc.erase(std::find(typeDesc.begin(), typeDesc.end(), '\0'), typeDesc.end());
        SPDLOG_DEBUG("getTypeDescriptor: found TYPE: \"{}\"", typeDesc);
        return typeDesc;
      }
    }

    SPDLOG_DEBUG("getTypeDescriptor: no TYPE found at current level");
    return "";
  }

  std::vector<int32_t> GpmfParser::getScaleFactors(GPMF_stream *stream, uint32_t expectedCount) const
  {
    std::vector<int32_t> scales;

    if(stream == nullptr) {
      return scales;
    }

    // Save the current position
    GPMF_stream tempStream = *stream;

    // Look for SCAL (scale) field at the current level
    if(GPMF_FindPrev(&tempStream, MAKEID('S', 'C', 'A', 'L'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *scaleData = static_cast<int32_t *>(GPMF_RawData(&tempStream));
      if(scaleData != nullptr) {
        uint32_t scaleCount = GPMF_Repeat(&tempStream);
        if(scaleCount > 0) {
          scales.reserve(scaleCount);
          for(uint32_t i = 0; i < scaleCount; i++) {
            // IMPORTANT: GPMF data is big-endian, must byte-swap!
            scales.push_back(BYTESWAP32(scaleData[i]));
          }
          SPDLOG_DEBUG("getScaleFactors: found {} scale values", scaleCount);

          // Warn if count doesn't match expected
          if(expectedCount > 0 && scaleCount != expectedCount) {
            SPDLOG_WARN("getScaleFactors: expected {} scale values, found {}", expectedCount, scaleCount);
          }
        }
      }
    } else {
      SPDLOG_DEBUG("getScaleFactors: no SCAL found at current level");
    }

    return scales;
  }

  std::string GpmfParser::getStreamName(GPMF_stream *stream) const
  {
    if(stream == nullptr) {
      return "";
    }

    GPMF_stream tempStream = *stream;

    // Look for STNM (stream name) field at the current level
    if(GPMF_FindPrev(&tempStream, MAKEID('S', 'T', 'N', 'M'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *nameData = static_cast<const char *>(GPMF_RawData(&tempStream));
      uint32_t nameSize = GPMF_RawDataSize(&tempStream);
      if(nameData != nullptr && nameSize > 0) {
        std::string name = gpmfAsciiToUtf8(nameData, nameSize);
        // Remove null terminators
        name.erase(std::find(name.begin(), name.end(), '\0'), name.end());
        return name;
      }
    }

    return "";
  }

  std::string GpmfParser::getSiUnits(GPMF_stream *stream) const
  {
    if(stream == nullptr) {
      return "";
    }

    GPMF_stream tempStream = *stream;

    // Look for SIUN (SI units) field at the current level
    if(GPMF_FindPrev(&tempStream, MAKEID('S', 'I', 'U', 'N'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *unitData = static_cast<const char *>(GPMF_RawData(&tempStream));
      uint32_t unitSize = GPMF_RawDataSize(&tempStream);
      if(unitData != nullptr && unitSize > 0) {
        std::string units = gpmfAsciiToUtf8(unitData, unitSize);
        // Remove null terminators
        units.erase(std::find(units.begin(), units.end(), '\0'), units.end());
        return units;
      }
    }

    return "";
  }

  std::vector<std::string> GpmfParser::getUnits(GPMF_stream *stream) const
  {
    std::vector<std::string> units;

    if(stream == nullptr) {
      return units;
    }

    GPMF_stream tempStream = *stream;

    // Look for UNIT field at the current level
    if(GPMF_FindPrev(&tempStream, MAKEID('U', 'N', 'I', 'T'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *unitData = static_cast<const char *>(GPMF_RawData(&tempStream));
      uint32_t unitCount = GPMF_Repeat(&tempStream);
      uint32_t unitSize = GPMF_StructSize(&tempStream);

      if(unitData != nullptr && unitCount > 0) {
        for(uint32_t i = 0; i < unitCount; i++) {
          std::string unit = gpmfAsciiToUtf8(unitData + i * unitSize, unitSize);
          // Remove null terminators
          unit.erase(std::find(unit.begin(), unit.end(), '\0'), unit.end());
          units.push_back(unit);
        }
      }
    }

    return units;
  }

  std::string GpmfParser::fourccToString(uint32_t fourcc)
  {
    std::string result(4, ' ');
    result[0] = static_cast<char>((fourcc >> 0) & 0xFF);
    result[1] = static_cast<char>((fourcc >> 8) & 0xFF);
    result[2] = static_cast<char>((fourcc >> 16) & 0xFF);
    result[3] = static_cast<char>((fourcc >> 24) & 0xFF);
    return result;
  }

  //! Convert nested object to JSON.
  nlohmann::json GpmfParser::nestedToJson(GPMF_stream *stream, [[maybe_unused]] uint32_t fourcc, int level)
  {
    nlohmann::json ret;

    // Copy the stream to avoid modifying the original
    GPMF_stream tempStream = *stream;
    SPDLOG_INFO("Nested toJson level {} ",level);

    // First, we need to "enter" the nest by calling GPMF_Next to descend into children
    GPMF_ERR err = GPMF_Next(&tempStream, GPMF_RECURSE_LEVELS);
    if(err != GPMF_OK) {
      // Empty nest or error
      return ret;
    }

    // Now iterate through children at the current level
    // Handle duplicate keys (like multiple STRM entries) by using arrays
    do {
      uint32_t fourXcc = GPMF_Key(&tempStream);
      std::string strFourCC = fourccToString(fourXcc);
      nlohmann::json childJson = samplesToJson(&tempStream, fourXcc, level);

      // If this key already exists, convert to array or append
      if(ret.contains(strFourCC)) {
        if(ret[strFourCC].is_array()) {
          ret[strFourCC].push_back(childJson);
        } else {
          // Convert single value to array with both old and new
          nlohmann::json oldValue = ret[strFourCC];
          ret[strFourCC] = nlohmann::json::array({oldValue, childJson});
        }
      } else {
        ret[strFourCC] = childJson;
      }
    } while(GPMF_OK == GPMF_Next(&tempStream, GPMF_CURRENT_LEVEL));

    return ret;
  }

  nlohmann::json GpmfParser::parseComplexTypeToJson(GPMF_stream *stream, const std::string& typeDesc, const std::vector<int32_t>& scales, uint32_t sampleCount) const
  {
    nlohmann::json samples = nlohmann::json::array();

    auto *rawData = static_cast<const uint8_t *>(GPMF_RawData(stream));
    if(rawData == nullptr) {
      return samples;
    }

    // Calculate sample size from TYPE descriptor
    size_t sampleSize = 0;
    for(char typeChar : typeDesc) {
      switch(typeChar) {
        case 'b': case 'B': sampleSize += 1; break;  // signed/unsigned byte
        case 's': case 'S': sampleSize += 2; break;  // signed/unsigned short
        case 'l': case 'L': sampleSize += 4; break;  // signed/unsigned long
        case 'f': sampleSize += 4; break;            // 32-bit float
        case 'F': sampleSize += 4; break;            // FourCC (4-char code)
        case 'd': sampleSize += 8; break;            // 64-bit double
        case 'j': case 'J': sampleSize += 8; break;  // signed/unsigned 64-bit
        case 'q': case 'Q': sampleSize += 4; break;  // Q15.16 and Q31.32
        default:
          SPDLOG_WARN("Unknown TYPE descriptor character: '{}'", typeChar);
          return samples;
      }
    }

    // Parse each sample
    for(uint32_t i = 0; i < sampleCount; i++) {
      nlohmann::json sample = nlohmann::json::array();
      size_t offset = i * sampleSize;
      size_t fieldOffset = 0;

      for(size_t fieldIdx = 0; fieldIdx < typeDesc.size(); fieldIdx++) {
        char typeChar = typeDesc[fieldIdx];
        int32_t scale = (fieldIdx < scales.size()) ? scales[fieldIdx] : 1;

        switch(typeChar) {
          case 'b': {  // signed byte
            int8_t val = static_cast<int8_t>(rawData[offset + fieldOffset]);
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : val);
            fieldOffset += 1;
            break;
          }
          case 'B': {  // unsigned byte
            uint8_t val = rawData[offset + fieldOffset];
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : val);
            fieldOffset += 1;
            break;
          }
          case 's': {  // signed short
            int16_t val = BYTESWAP16(*reinterpret_cast<const int16_t*>(rawData + offset + fieldOffset));
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : val);
            fieldOffset += 2;
            break;
          }
          case 'S': {  // unsigned short
            uint16_t val = BYTESWAP16(*reinterpret_cast<const uint16_t*>(rawData + offset + fieldOffset));
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : val);
            fieldOffset += 2;
            break;
          }
          case 'l': {  // signed long
            int32_t val = BYTESWAP32(*reinterpret_cast<const int32_t*>(rawData + offset + fieldOffset));
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : val);
            fieldOffset += 4;
            break;
          }
          case 'L': {  // unsigned long
            uint32_t val = BYTESWAP32(*reinterpret_cast<const uint32_t*>(rawData + offset + fieldOffset));
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : val);
            fieldOffset += 4;
            break;
          }
          case 'f': {  // 32-bit float
            uint32_t rawVal = BYTESWAP32(*reinterpret_cast<const uint32_t*>(rawData + offset + fieldOffset));
            float val = std::bit_cast<float>(rawVal);
            sample.push_back(scale != 1 ? val / static_cast<float>(scale) : val);
            fieldOffset += 4;
            break;
          }
          case 'F': {  // FourCC (4-character code)
            // FourCC is stored as readable ASCII in big-endian, don't byte-swap
            uint32_t fourcc = *reinterpret_cast<const uint32_t*>(rawData + offset + fieldOffset);
            sample.push_back(fourccToString(fourcc));
            fieldOffset += 4;
            break;
          }
          case 'd': {  // 64-bit double
            uint64_t rawVal = BYTESWAP64(*reinterpret_cast<const uint64_t*>(rawData + offset + fieldOffset));
            double val = std::bit_cast<double>(rawVal);
            sample.push_back(scale != 1 ? val / scale : val);
            fieldOffset += 8;
            break;
          }
          case 'j': {  // signed 64-bit int
            int64_t val = BYTESWAP64(*reinterpret_cast<const int64_t*>(rawData + offset + fieldOffset));
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : static_cast<double>(val));
            fieldOffset += 8;
            break;
          }
          case 'J': {  // unsigned 64-bit int
            uint64_t val = BYTESWAP64(*reinterpret_cast<const uint64_t*>(rawData + offset + fieldOffset));
            sample.push_back(scale != 1 ? static_cast<double>(val) / scale : static_cast<double>(val));
            fieldOffset += 8;
            break;
          }
          case 'q': {  // Q15.16 fixed point
            int32_t rawVal = BYTESWAP32(*reinterpret_cast<const int32_t*>(rawData + offset + fieldOffset));
            double val = static_cast<double>(rawVal) / 65536.0;
            sample.push_back(scale != 1 ? val / scale : val);
            fieldOffset += 4;
            break;
          }
          case 'Q': {  // Q31.32 fixed point
            int64_t rawVal = BYTESWAP64(*reinterpret_cast<const int64_t*>(rawData + offset + fieldOffset));
            double val = static_cast<double>(rawVal) / 4294967296.0;
            sample.push_back(scale != 1 ? val / scale : val);
            fieldOffset += 8;
            break;
          }
          default:
            // Unknown type - skip
            break;
        }
      }

      // If only one field, don't wrap in array
      if(sample.size() == 1) {
        samples.push_back(sample[0]);
      } else {
        samples.push_back(sample);
      }
    }

    return samples;
  }

  nlohmann::json GpmfParser::samplesToJson(GPMF_stream *stream, [[maybe_unused]] uint32_t fourcc, int level)
  {
    if(stream == nullptr) {
      return {};
    }

    // Extract samples using GPMF_ScaledData
#if 0
    std::vector<double> scales;
    float scaleFactor = getScaleFactor(stream, fourcc);
    uint32_t elements = GPMF_ElementsInStruct(stream);
    scales.resize(elements, scaleFactor);
#endif

    GPMF_SampleType sampleType = GPMF_Type(stream);
    uint32_t sampleCount = GPMF_Repeat(stream);
    uint32_t elements = GPMF_ElementsInStruct(stream);

    // Handle NEST types specially - they don't have samples, just child KLVs
    // We need to recursively parse the children and build a nested JSON object
    if(sampleType == GPMF_TYPE_NEST) {
      SPDLOG_INFO("Sample nest -> '{}'[{}]  Elements:{} ", fourccToString(fourcc), sampleCount,elements);
      return nestedToJson(stream, fourcc, level+1);
    }
    if(elements == 0) {
      return {};
    }
    if(mVerbose) {
      SPDLOG_INFO("Sample type {} -> '{}'[{}]  Elements:{} ", fourccToString(fourcc), static_cast<char>(sampleType),sampleCount,elements);
    }

    switch(sampleType) {
      case GPMF_TYPE_STRING_ASCII: {
        // GPMF ASCII strings may contain extended ASCII characters (°, ², ³, µ)
        // Convert to proper UTF-8 for JSON compatibility
        return gpmfAsciiToUtf8(static_cast<const char *>(GPMF_RawData(stream)), GPMF_RawDataSize(stream));
      }

      case GPMF_TYPE_SIGNED_BYTE: {
        auto *data = static_cast<const int8_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(static_cast<int>(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(static_cast<int>(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_BYTE: {
        auto *data = static_cast<const uint8_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(static_cast<unsigned>(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(static_cast<unsigned>(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_SIGNED_SHORT: {
        auto *data = static_cast<const int16_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(BYTESWAP16(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP16(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_SHORT: {
        auto *data = static_cast<const uint16_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(BYTESWAP16(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP16(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_SIGNED_LONG: {
        auto *data = static_cast<const int32_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(BYTESWAP32(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP32(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_LONG: {
        auto *data = static_cast<const uint32_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(BYTESWAP32(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP32(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_FLOAT: {
        auto *data = static_cast<const uint32_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            uint32_t swap = BYTESWAP32(data[i]);
            float f = std::bit_cast<float>(swap);
            samples.push_back(f);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              uint32_t swap = BYTESWAP32(data[i * elements + j]);
              float f = std::bit_cast<float>(swap);
              sample.push_back(f);
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_DOUBLE: {
        auto *data = static_cast<const uint64_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            uint64_t swap = BYTESWAP64(data[i]);
            double d = std::bit_cast<double>(swap);
            samples.push_back(d);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              uint64_t swap = BYTESWAP64(data[i * elements + j]);
              double d = std::bit_cast<double>(swap);
              sample.push_back(d);
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_Q15_16_FIXED_POINT: {
        auto *data = static_cast<const int32_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            double dq = static_cast<double>(BYTESWAP32(data[i])) / 65536.0;
            samples.push_back(dq);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              double dq = static_cast<double>(BYTESWAP32(data[i * elements + j])) / 65536.0;
              sample.push_back(dq);
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_Q31_32_FIXED_POINT: {
        auto *data = static_cast<const int64_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            uint64_t Q64 = BYTESWAP64(static_cast<uint64_t>(data[i]));
            double dq = static_cast<double>(Q64 >> 32);
            dq += static_cast<double>(Q64 & 0xFFFFFFFF) / 4294967296.0;
            samples.push_back(dq);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              uint64_t Q64 = BYTESWAP64(static_cast<uint64_t>(data[i * elements + j]));
              double dq = static_cast<double>(Q64 >> 32);
              dq += static_cast<double>(Q64 & 0xFFFFFFFF) / 4294967296.0;
              sample.push_back(dq);
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_SIGNED_64BIT_INT: {
        auto *data = static_cast<const int64_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(BYTESWAP64(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP64(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_64BIT_INT: {
        auto *data = static_cast<const uint64_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(BYTESWAP64(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP64(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_FOURCC: {
        auto *data = static_cast<const uint32_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for(uint32_t i = 0; i < sampleCount; i++) {
          if(elements == 1) {
            samples.push_back(fourccToString(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for(uint32_t j = 0; j < elements; j++) {
              sample.push_back(fourccToString(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_GUID: {
        auto *data = static_cast<const uint8_t *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        uint32_t guidSize = 16;// GUIDs are 128 bits = 16 bytes
        for(uint32_t i = 0; i < sampleCount; i++) {
          std::string guid;
          for(uint32_t j = 0; j < guidSize; j++) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02X", data[i * guidSize + j]);
            guid += hex;
          }
          samples.push_back(guid);
        }
        return samples;
      }

      case GPMF_TYPE_UTC_DATE_TIME: {
        auto *data = static_cast<const char *>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        uint32_t dateSize = 16;// UTC dates are 16 bytes: yymmddhhmmss.sss
        for(uint32_t i = 0; i < sampleCount; i++) {
          std::string dateStr(data + i * dateSize, dateSize);
          // Trim null terminators
          dateStr.erase(std::find(dateStr.begin(), dateStr.end(), '\0'), dateStr.end());
          samples.push_back(dateStr);
        }
        return samples;
      }

      case GPMF_TYPE_STRING_UTF8: {
        // UTF-8 strings should already be valid, just need to handle null terminators
        const char *data = static_cast<const char *>(GPMF_RawData(stream));
        size_t size = GPMF_RawDataSize(stream);
        std::string str(data, size);
        // Remove null terminators
        str.erase(std::find(str.begin(), str.end(), '\0'), str.end());
        return str;
      }

      case GPMF_TYPE_COMPLEX: {
        // Complex types use TYPE descriptor to define their structure
        // Try to parse using TYPE descriptor if available
        std::string typeDesc = getTypeDescriptor(stream);

        if(typeDesc.empty()) {
          // No TYPE descriptor - return basic info
          nlohmann::json result;
          result["type"] = "complex";
          result["size_bytes"] = GPMF_RawDataSize(stream);
          result["sample_count"] = sampleCount;
          result["note"] = "No TYPE descriptor found";
          return result;
        }

        // Get SCAL values if present
        std::vector<int32_t> scales = getScaleFactors(stream, static_cast<uint32_t>(typeDesc.size()));

        // Parse complex data using TYPE descriptor
        return parseComplexTypeToJson(stream, typeDesc, scales, sampleCount);
      }

      case GPMF_TYPE_COMPRESSED: {
        // Compressed data needs decompression first
        nlohmann::json result;
        result["type"] = "compressed";
        result["size_bytes"] = GPMF_RawDataSize(stream);
        result["sample_count"] = sampleCount;
        return result;
      }

      case GPMF_TYPE_NEST: {
        // Should never reach here - NEST is handled before the switch
        SPDLOG_WARN("GPMF_TYPE_NEST reached in switch - this is a bug");
        RavlAlwaysAssert(false);
        return nestedToJson(stream, fourcc, level+1);
      }

      case GPMF_TYPE_EMPTY: {
        nlohmann::json result;
        result["type"] = "empty";
        result["note"] = "Empty payload";
        return result;
      }

      case GPMF_TYPE_ERROR: {
        nlohmann::json result;
        result["type"] = "error";
        result["note"] = "Error type";
        return result;
      }
    }

    return {};
  }

  namespace
  {
    [[maybe_unused]] bool reg1 = registerTypeName(typeid(Video::FrameData<nlohmann::json>),"Ravl2::Video::MetaDataFrame<nlohmann::json>");

  }

}// namespace Ravl2::GoPro
