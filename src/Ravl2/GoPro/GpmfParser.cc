//
// Created for RAVL2 GoPro metadata support
//

#include "Ravl2/GoPro/GpmfParser.hh"

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
    SPDLOG_INFO("Generated {} frames.",frames.size());
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
      frames.push_back(std::make_shared<Video::MetaDataFrame<GpsFix>>(
        fix,
        streamId + mNextId++,
        fixTimestamp));
    }

    SPDLOG_DEBUG("Created {} GPS frames from GPMF packet at timestamp {} μs", sampleCount, timestamp.count());
  }

  void GpmfParser::parseGps9Complex(GPMF_stream *stream, std::vector<std::shared_ptr<Video::Frame>> &frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    SPDLOG_INFO("Extracting GPS9 (complex type with TYPE descriptor).");
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
      frames.push_back(std::make_shared<Video::MetaDataFrame<GpsFix>>(
        fix,
        streamId + mNextId++,
        fixTimestamp));
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
      SPDLOG_WARN("Unusual gyro sample rate: {} Hz (typical GoPro: 200-400 Hz)", sampleRate);
    }

    // Create and append frame
    frames.push_back(std::make_shared<Video::MetaDataFrame<GyroSamples>>(
      GyroSamples(samples, sampleRate),
      streamId + mNextId++,
      timestamp));
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
      SPDLOG_WARN("Unusual accel sample rate: {} Hz (typical GoPro: 200-400 Hz)", sampleRate);
    }

    // Create and append frame
    frames.push_back(std::make_shared<Video::MetaDataFrame<AccelSamples>>(
      AccelSamples(samples, sampleRate),
      streamId + mNextId++,
      timestamp));
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

          unknownJson["samples"] = samplesToJson(levelStream, lastFourcc, level+1);

          // Create JSON frame
          auto jsonFrame = std::make_shared<Video::MetaDataFrame<nlohmann::json>>(
            unknownJson,
            streamId + mNextId++,
            timestamp);
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

  float GpmfParser::getSampleRate([[maybe_unused]] GPMF_stream *stream) const
  {
    if(stream == nullptr) {
      return 0.0f;
    }

    // Save the current position
    GPMF_stream tempStream = *stream;

    // Strategy 1: Look for TSMP (Total SaMPles) field
    // IMPORTANT: TSMP is the NUMBER OF SAMPLES in this packet, NOT a timestamp!
    // It should match the repeat count of the data field. It's a redundant field for validation.
    // DO NOT use TSMP to calculate sample rate - it's just a sample count.
    // For Hero 8: TSMP=19 means 19 GPS samples in packet
    // For Hero 13: TSMP=1 means 1 GPS sample in packet
    if(GPMF_FindPrev(&tempStream, MAKEID('T', 'S', 'M', 'P'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto *tsmpData = static_cast<uint32_t *>(GPMF_RawData(&tempStream));
      if(tsmpData != nullptr) {
        uint32_t sampleCount = GPMF_Repeat(&tempStream);
        if(sampleCount > 0) {
          // IMPORTANT: GPMF data is big-endian, must byte-swap!
          uint32_t tsmpValue = BYTESWAP32(tsmpData[0]);// TSMP is sample count

          // Get number of samples from the parent data stream
          uint32_t dataRepeat = GPMF_Repeat(stream);

          // Validate TSMP matches data repeat count
          if(tsmpValue != dataRepeat) {
            SPDLOG_WARN("TSMP value ({}) does not match data repeat count ({})", tsmpValue, dataRepeat);
          }

          // TSMP doesn't tell us the sample rate - fall through to other strategies
          SPDLOG_DEBUG("Found TSMP: {} samples (matches data repeat: {})", tsmpValue, dataRepeat);
        }
      }
    }

    // Reset temp stream
    tempStream = *stream;

    // Strategy 2: Look for ORIN (Original Sample Rate) field
    // NOTE: ORIN is type 'c' (string) containing axis orientation like "ZXY", NOT a sample rate!
    // This strategy is disabled - ORIN does not contain sample rate information
    // if (GPMF_FindPrev(&tempStream, MAKEID('O', 'R', 'I', 'N'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
    //   // This was incorrectly trying to read ORIN as a numeric sample rate
    // }

    // Strategy 3: Detect based on FourCC of the current stream (fallback)
    uint32_t fourcc = GPMF_Key(stream);
    char fourccStr[5] = {0};
    fourccStr[0] = static_cast<char>((fourcc >> 0) & 0xFF);
    fourccStr[1] = static_cast<char>((fourcc >> 8) & 0xFF);
    fourccStr[2] = static_cast<char>((fourcc >> 16) & 0xFF);
    fourccStr[3] = static_cast<char>((fourcc >> 24) & 0xFF);

    SPDLOG_INFO("Detecting sample rate for FourCC: {}", fourccStr);

    // Use typical rates for known sensor types
    if(fourcc == MAKEID('G', 'Y', 'R', 'O')) {
      SPDLOG_DEBUG("Using default gyro rate: 200 Hz");
      return 200.0f;// Typical GoPro gyro rate
    }
    if(fourcc == MAKEID('A', 'C', 'C', 'L')) {
      SPDLOG_DEBUG("Using default accel rate: 200 Hz");
      return 200.0f;// Typical GoPro accel rate
    }
    if(fourcc == MAKEID('G', 'P', 'S', '5') || fourcc == MAKEID('G', 'P', 'S', '9')) {
      SPDLOG_DEBUG("Using default GPS rate: 18 Hz (GoPro Hero 8+ typical max)");
      return 18.0f;// Typical GoPro GPS rate (can be 1, 5, 10, or 18 Hz)
    }

    // Fallback: return 0 to indicate unknown
    SPDLOG_WARN("Could not determine sample rate for FourCC: {}, returning 0", fourccStr);
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

  std::string GpmfParser::fourccToString(uint32_t fourcc)
  {
    std::string result(4, ' ');
    result[0] = static_cast<char>((fourcc >> 0) & 0xFF);
    result[1] = static_cast<char>((fourcc >> 8) & 0xFF);
    result[2] = static_cast<char>((fourcc >> 16) & 0xFF);
    result[3] = static_cast<char>((fourcc >> 24) & 0xFF);
    return result;
  }

  nlohmann::json GpmfParser::extractDeviceInfo(GPMF_stream *stream)
  {
    nlohmann::json deviceInfo;

    if(stream == nullptr) {
      return deviceInfo;
    }

    // Get device ID (DVID)
    uint32_t deviceId = GPMF_DeviceID(stream);
    if(deviceId > 0) {
      deviceInfo["device_id"] = deviceId;
    }

    // Save the current position
    GPMF_stream tempStream = *stream;

    // Look for the device name (DVNM)
    if(GPMF_FindPrev(&tempStream, MAKEID('D', 'V', 'N', 'M'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *dvnmData = static_cast<char *>(GPMF_RawData(&tempStream));
      uint32_t dvnmSize = GPMF_RawDataSize(&tempStream);
      if(dvnmData != nullptr && dvnmSize > 0) {
        std::string dvnm = gpmfAsciiToUtf8(dvnmData, dvnmSize);
        if(!dvnm.empty()) {
          deviceInfo["device_name"] = dvnm;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Look for version (VERS)
    if(GPMF_FindPrev(&tempStream, MAKEID('V', 'E', 'R', 'S'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *versData = static_cast<char *>(GPMF_RawData(&tempStream));
      uint32_t versSize = GPMF_RawDataSize(&tempStream);
      if(versData != nullptr && versSize > 0) {
        std::string vers = gpmfAsciiToUtf8(versData, versSize);
        if(!vers.empty()) {
          deviceInfo["version"] = vers;
        }
      }
    }

    return deviceInfo;
  }

  nlohmann::json GpmfParser::extractStreamMetadata(GPMF_stream *stream, [[maybe_unused]] uint32_t fourcc)
  {
    SPDLOG_INFO("Extracting stream metadata");
    nlohmann::json metadata;

    if(stream == nullptr) {
      return metadata;
    }

    // Get sample count
    uint32_t sampleCount = GPMF_Repeat(stream);
    metadata["sample_count"] = sampleCount;

    // Get type information
    GPMF_SampleType type = GPMF_Type(stream);
    metadata["type_info"]["gpmf_type"] = std::string(1, static_cast<char>(type));
    metadata["type_info"]["struct_size"] = GPMF_StructSize(stream);
    metadata["type_info"]["elements_per_sample"] = GPMF_ElementsInStruct(stream);

#if 0
    // Get sample rate
    float sampleRate = getSampleRate(stream);
    if (sampleRate > 0) {
      metadata["sample_rate_hz"] = sampleRate;
    }
#endif

    // Save the current position for metadata searches
    GPMF_stream tempStream = *stream;

    // Get scaling factors (SCAL)
    if(GPMF_FindPrev(&tempStream, MAKEID('S', 'C', 'A', 'L'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *scaleData = static_cast<uint32_t *>(GPMF_RawData(&tempStream));
      uint32_t scaleCount = GPMF_Repeat(&tempStream);
      if(scaleData != nullptr && scaleCount > 0) {
        std::vector<uint32_t> scales;
        for(uint32_t i = 0; i < scaleCount; i++) {
          scales.push_back(BYTESWAP32(scaleData[i]));
        }
        metadata["units"]["scale"] = scales;
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get SI units (SIUN)
    if(GPMF_FindPrev(&tempStream, MAKEID('S', 'I', 'U', 'N'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *siunData = static_cast<char *>(GPMF_RawData(&tempStream));
      uint32_t siunSize = GPMF_RawDataSize(&tempStream);
      if(siunData != nullptr && siunSize > 0) {
        // SIUN may contain special characters like °, ², ³, µ
        std::string siun = gpmfAsciiToUtf8(siunData, siunSize);
        if(!siun.empty()) {
          metadata["units"]["siun"] = siun;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get units (UNIT)
    if(GPMF_FindPrev(&tempStream, MAKEID('U', 'N', 'I', 'T'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *unitData = static_cast<char *>(GPMF_RawData(&tempStream));
      uint32_t unitSize = GPMF_RawDataSize(&tempStream);
      if(unitData != nullptr && unitSize > 0) {
        // UNIT may contain special characters like °, ², ³, µ
        std::string unit = gpmfAsciiToUtf8(unitData, unitSize);
        if(!unit.empty()) {
          metadata["units"]["unit"] = unit;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get timestamp info (TSMP)
    if(GPMF_FindPrev(&tempStream, MAKEID('T', 'S', 'M', 'P'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *tsmpData = static_cast<uint32_t *>(GPMF_RawData(&tempStream));
      if(tsmpData != nullptr) {
        metadata["timestamp_info"]["tsmp"] = BYTESWAP32(tsmpData[0]);
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get ORIN (original sample rate or orientation)
    if(GPMF_FindPrev(&tempStream, MAKEID('O', 'R', 'I', 'N'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      GPMF_SampleType orinType = GPMF_Type(&tempStream);
      auto *orinData = GPMF_RawData(&tempStream);

      if(orinType == GPMF_TYPE_UNSIGNED_LONG || orinType == GPMF_TYPE_SIGNED_LONG) {
        // Numeric: sample rate - IMPORTANT: byte swap for big-endian data
        uint32_t orinValue = BYTESWAP32(*static_cast<uint32_t *>(orinData));
        metadata["timestamp_info"]["orin"] = orinValue;
      } else if(orinType == GPMF_TYPE_STRING_ASCII) {
        // String: orientation
        uint32_t orinSize = GPMF_RawDataSize(&tempStream);
        std::string orin = gpmfAsciiToUtf8(static_cast<char *>(orinData), orinSize);
        if(!orin.empty()) {
          metadata["orientation"]["input"] = orin;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get ORIO (output orientation)
    if(GPMF_FindPrev(&tempStream, MAKEID('O', 'R', 'I', 'O'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *orioData = static_cast<char *>(GPMF_RawData(&tempStream));
      uint32_t orioSize = GPMF_RawDataSize(&tempStream);
      if(orioData != nullptr && orioSize > 0) {
        std::string orio = gpmfAsciiToUtf8(orioData, orioSize);
        if(!orio.empty()) {
          metadata["orientation"]["output"] = orio;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get TICK (start time)
    if(GPMF_FindPrev(&tempStream, MAKEID('T', 'I', 'C', 'K'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *tickData = static_cast<uint32_t *>(GPMF_RawData(&tempStream));
      if(tickData != nullptr) {
        metadata["timestamp_info"]["tick"] = BYTESWAP32(tickData[0]);
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get TOCK (end time)
    if(GPMF_FindPrev(&tempStream, MAKEID('T', 'O', 'C', 'K'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto *tockData = static_cast<uint32_t *>(GPMF_RawData(&tempStream));
      if(tockData != nullptr) {
        metadata["timestamp_info"]["tock"] = BYTESWAP32(tockData[0]);
      }
    }

    return metadata;
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
        // Complex types have opaque data - return as hex string or raw info
        nlohmann::json result;
        result["type"] = "complex";
        result["size_bytes"] = GPMF_RawDataSize(stream);
        result["sample_count"] = sampleCount;
        return result;
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
    [[maybe_unused]] bool reg1 = registerTypeName(typeid(Video::MetaDataFrame<nlohmann::json>),"Ravl2::Video::MetaDataFrame<nlohmann::json>");

  }

}// namespace Ravl2::GoPro
