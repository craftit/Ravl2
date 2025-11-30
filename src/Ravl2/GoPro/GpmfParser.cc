//
// Created for RAVL2 GoPro metadata support
//

#include "Ravl2/GoPro/GpmfParser.hh"
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
  GpmfParser::GpmfParser(bool enableJson)
    : mEnableJson(enableJson)
  {
  }

  std::vector<std::shared_ptr<Video::Frame>> GpmfParser::parse(
    const uint8_t* data,
    size_t size,
    Video::StreamItemId streamId,
    Video::MediaTime timestamp)
  {
    std::vector<std::shared_ptr<Video::Frame>> frames;

    if (data == nullptr || size == 0) {
      SPDLOG_DEBUG("parse() called with null/empty data");
      return frames;
    }

    // Initialize GPMF stream
    // Note: GPMF_Init takes buffer size in BYTES (despite the uint32_t* pointer type)
    GPMF_stream stream;
    GPMF_ERR initResult = GPMF_Init(&stream, const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(data)), static_cast<uint32_t>(size));
    if (initResult != GPMF_OK) {
      SPDLOG_WARN("Failed to initialize GPMF stream (error={}), data size={} bytes", static_cast<int>(initResult), size);
      return frames;
    }

    // Validate the stream structure
    GPMF_ERR validateResult = GPMF_Validate(&stream, static_cast<GPMF_LEVELS>(GPMF_RECURSE_LEVELS | GPMF_TOLERANT));
    if (validateResult != GPMF_OK) {
      SPDLOG_WARN("GPMF stream validation failed (error={})", static_cast<int>(validateResult));
      return frames;
    }

    // Single-pass traversal: iterate through all entries using GPMF_Next
    GPMF_ResetState(&stream);

    do {
      uint32_t fourcc = GPMF_Key(&stream);

      // Process known sensor data types
      switch (fourcc) {
        case MAKEID('G', 'P', 'S', '5'):
          parseGps(&stream, frames, streamId, timestamp);
          break;

        case MAKEID('G', 'Y', 'R', 'O'):
          parseGyro(&stream, frames, streamId, timestamp);
          break;

        case MAKEID('A', 'C', 'C', 'L'):
          parseAccel(&stream, frames, streamId, timestamp);
          break;
        case MAKEID('D', 'E', 'V', 'C'): {
          auto metaJson = extractStreamMetadata(&stream,fourcc);
          if(!metaJson.empty()) {
            auto jsonFrame = std::make_shared<Video::MetaDataFrame<nlohmann::json>>(
              metaJson,
              streamId + mNextId++,
              timestamp);
            frames.push_back(jsonFrame);
          }
        } break;

        default:
          // Unknown FourCC - convert to JSON if enabled
          if (mEnableJson) {
            // Check if this is a data entry (has samples)
            uint32_t sampleCount = GPMF_Repeat(&stream);
            if (sampleCount > 0) {
              nlohmann::json unknownJson;
              unknownJson["fourcc"] = fourccToString(fourcc);
              unknownJson["type"] = std::string(1, static_cast<char>(GPMF_Type(&stream)));
              unknownJson["sample_count"] = sampleCount;
              unknownJson["elements"] = GPMF_ElementsInStruct(&stream);

              unknownJson["samples"] = samplesToJson(&stream,fourcc);

              // Create JSON frame
              auto jsonFrame = std::make_shared<Video::MetaDataFrame<nlohmann::json>>(
                unknownJson,
                streamId + mNextId++,
                timestamp);
              frames.push_back(jsonFrame);
            }
          }
          break;
      }
    } while (GPMF_OK == GPMF_Next(&stream, GPMF_RECURSE_LEVELS));

    return frames;
  }

  void GpmfParser::parseGps(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    if (stream == nullptr) {
      SPDLOG_WARN("parseGps: null stream pointer");
      return;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      SPDLOG_DEBUG("parseGps: GPS5 stream has 0 samples");
      return;
    }

    // GPS5 format: latitude, longitude, altitude, 2D speed, 3D speed
    // All values are scaled integers
    float scale = getScaleFactor(stream, MAKEID('G', 'P', 'S', '5'));
    if (scale == 0) {
      SPDLOG_WARN("parseGps: invalid scale factor, using default 1.0");
      scale = 1.0f;
    }

    // Get sample rate
    float sampleRate = getSampleRate(stream);

    // Get raw data
    auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
    if (rawData == nullptr) {
      SPDLOG_ERROR("parseGps: failed to get raw data from GPMF stream (sampleCount={})", sampleCount);
      return;
    }

    // Validate sample rate
    if (sampleRate <= 0.0F) {
      SPDLOG_ERROR("Invalid GPS sample rate: {} Hz (must be > 0)", sampleRate);
      sampleRate = 0.0F;
    } else if (sampleRate < 1.0F || sampleRate > 20.0F) {
      SPDLOG_WARN("Unusual GPS sample rate: {} Hz (typical GoPro: 1-18 Hz)", sampleRate);
    }

    // Calculate time delta between samples for timestamp interpolation
    Video::MediaTime timeDelta(0);
    if (sampleRate > 0.0F && sampleCount > 1) {
      // Convert sample rate to microseconds per sample
      int64_t deltaUs = static_cast<int64_t>((1.0F / sampleRate) * 1000000.0F);
      timeDelta = Video::MediaTime(deltaUs);
    }

    // Parse ALL GPS samples and create individual frames
    for (uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * 5; // 5 values per sample (lat, lon, alt, speed2d, speed3d)

      double latitude = static_cast<double>(rawData[offset + 0]) * static_cast<double>(scale);
      double longitude = static_cast<double>(rawData[offset + 1]) * static_cast<double>(scale);
      double altitude = static_cast<double>(rawData[offset + 2]) * static_cast<double>(scale);
      float speed2d = static_cast<float>(rawData[offset + 3]) * scale;
      float speed3d = static_cast<float>(rawData[offset + 4]) * scale;

      // Create GPSCoordinate
      GPSCoordinate location(latitude, longitude, altitude);

      // Create GpsFix
      GpsFix fix;
      fix.location = location;
      fix.speed = Point<float, 2>(speed2d, speed3d);
      fix.fix = 3; // Assume 3D fix (we have altitude)
      fix.satellites = 0; // Not available in GPS5
      fix.precision = 0; // Not available in GPS5

      // Interpolate timestamp for this specific fix
      Video::MediaTime fixTimestamp = timestamp;
      if (timeDelta.count() > 0) {
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

  void GpmfParser::parseGyro(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    if (stream == nullptr) {
      SPDLOG_WARN("parseGyro: null stream pointer");
      return;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      SPDLOG_DEBUG("parseGyro: GYRO stream has 0 samples");
      return;
    }

    // Get scale factor
    float scale = getScaleFactor(stream, MAKEID('G', 'Y', 'R', 'O'));
    if (scale == 0) {
      SPDLOG_WARN("parseGyro: invalid scale factor, using default 1.0");
      scale = 1.0f;
    }

    // Get sample rate
    float sampleRate = getSampleRate(stream);

    // Get raw data (3 int16 values per sample: x, y, z)
    auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
    if (rawData == nullptr) {
      SPDLOG_ERROR("parseGyro: failed to get raw data from GPMF stream (sampleCount={})", sampleCount);
      return;
    }

    std::vector<Vector3f> samples;
    samples.reserve(sampleCount);
    for (uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * 3;
      samples.emplace_back(
        static_cast<float>(rawData[offset + 0]) * scale,
        static_cast<float>(rawData[offset + 1]) * scale,
        static_cast<float>(rawData[offset + 2]) * scale
      );
    }

    // Validate sample rate
    if (sampleRate <= 0.0F) {
      SPDLOG_ERROR("Invalid gyro sample rate: {} Hz (must be > 0)", sampleRate);
      sampleRate = 0.0F;
    } else if (sampleRate < 50.0F || sampleRate > 1000.0F) {
      SPDLOG_WARN("Unusual gyro sample rate: {} Hz (typical GoPro: 200-400 Hz)", sampleRate);
    }

    // Create and append frame
    frames.push_back(std::make_shared<Video::MetaDataFrame<GyroSamples>>(
      GyroSamples(samples, sampleRate),
      streamId + mNextId++,
      timestamp));
  }

  void GpmfParser::parseAccel(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp)
  {
    if (stream == nullptr) {
      SPDLOG_WARN("parseAccel: null stream pointer");
      return;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      SPDLOG_DEBUG("parseAccel: ACCL stream has 0 samples");
      return;
    }

    // Get scale factor
    float scale = getScaleFactor(stream, MAKEID('A', 'C', 'C', 'L'));
    if (scale == 0) {
      SPDLOG_WARN("parseAccel: invalid scale factor, using default 1.0");
      scale = 1.0f;
    }

    // Get sample rate
    float sampleRate = getSampleRate(stream);

    // Get raw data (3 int16 values per sample: x, y, z)
    auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
    if (rawData == nullptr) {
      SPDLOG_ERROR("parseAccel: failed to get raw data from GPMF stream (sampleCount={})", sampleCount);
      return;
    }

    std::vector<Vector3f> samples;
    samples.reserve(sampleCount);
    for (uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * 3;
      samples.emplace_back(
        static_cast<float>(rawData[offset + 0]) * scale,
        static_cast<float>(rawData[offset + 1]) * scale,
        static_cast<float>(rawData[offset + 2]) * scale
      );
    }

    // Validate sample rate
    if (sampleRate <= 0.0F) {
      SPDLOG_ERROR("Invalid accel sample rate: {} Hz (must be > 0)", sampleRate);
      sampleRate = 0.0F;
    } else if (sampleRate < 50.0F || sampleRate > 1000.0F) {
      SPDLOG_WARN("Unusual accel sample rate: {} Hz (typical GoPro: 200-400 Hz)", sampleRate);
    }

    // Create and append frame
    frames.push_back(std::make_shared<Video::MetaDataFrame<AccelSamples>>(
      AccelSamples(samples, sampleRate),
      streamId + mNextId++,
      timestamp));
  }

  float GpmfParser::getScaleFactor(GPMF_stream* stream, [[maybe_unused]] uint32_t fourcc) const
  {
    if (stream == nullptr) {
      return 1.0f;
    }

    // Save current position
    GPMF_stream tempStream = *stream;

    // Look for SCAL (scale) field at the current level (sibling of current FourCC)
    // GPMF_CURRENT_LEVEL ensures we only look at siblings, not parent/child SCAL tags
    if (GPMF_FindPrev(&tempStream, MAKEID('S', 'C', 'A', 'L'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* scaleData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      if (scaleData != nullptr) {
        uint32_t scaleCount = GPMF_Repeat(&tempStream);
        if (scaleCount > 0) {
          // Scale is typically stored as an integer divisor
          // For multi-component data (e.g., XYZ), SCAL may have multiple values
          // Use the first scale value (they're usually all the same for sensor data)
          float scale = 1.0f / static_cast<float>(scaleData[0]);
          SPDLOG_DEBUG("getScaleFactor: found SCAL with {} values, using first: 1/{} = {}",
                       scaleCount, scaleData[0], scale);
          return scale;
        }
      }
    }

    SPDLOG_DEBUG("getScaleFactor: no SCAL found at current level, using default 1.0");
    return 1.0f;
  }

  float GpmfParser::getSampleRate([[maybe_unused]] GPMF_stream* stream) const
  {
    if (stream == nullptr) {
      return 0.0f;
    }

    // Save current position
    GPMF_stream tempStream = *stream;

    // Strategy 1: Look for TSMP (Time Stamp) field - most accurate
    // TSMP contains the time span for all samples in this packet (in microseconds)
    if (GPMF_FindPrev(&tempStream, MAKEID('T', 'S', 'M', 'P'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* tsmpData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      if (tsmpData != nullptr) {
        uint32_t sampleCount = GPMF_Repeat(&tempStream);
        if (sampleCount > 0) {
          uint32_t tsmpValue = tsmpData[0]; // TSMP in microseconds

          // Get number of samples from the parent data stream
          uint32_t dataRepeat = GPMF_Repeat(stream);
          if (dataRepeat > 1 && tsmpValue > 0) {
            // Calculate rate: (samples - 1) / (time span in seconds)
            // Example: 18 samples over 944444 μs → 17 / 0.944444 = 18.0 Hz
            float rate = (static_cast<float>(dataRepeat - 1) * 1000000.0f) / static_cast<float>(tsmpValue);
            SPDLOG_DEBUG("Calculated sample rate from TSMP: {:.2f} Hz (samples={}, tsmp={}μs)",
                         rate, dataRepeat, tsmpValue);
            return rate;
          }
        }
      }
    }

    // Reset temp stream
    tempStream = *stream;

    // Strategy 2: Look for ORIN (Original Sample Rate) field
    // This is the nominal sample rate from the device
    if (GPMF_FindPrev(&tempStream, MAKEID('O', 'R', 'I', 'N'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* orinData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      if (orinData != nullptr) {
        uint32_t sampleCount = GPMF_Repeat(&tempStream);
        if (sampleCount > 0) {
          uint32_t orinValue = orinData[0]; // ORIN in Hz
          SPDLOG_DEBUG("Found ORIN (original sample rate): {} Hz", orinValue);
          return static_cast<float>(orinValue);
        }
      }
    }

    // Strategy 3: Detect based on FourCC of current stream (fallback)
    uint32_t fourcc = GPMF_Key(stream);
    char fourccStr[5] = {0};
    fourccStr[0] = static_cast<char>((fourcc >> 0) & 0xFF);
    fourccStr[1] = static_cast<char>((fourcc >> 8) & 0xFF);
    fourccStr[2] = static_cast<char>((fourcc >> 16) & 0xFF);
    fourccStr[3] = static_cast<char>((fourcc >> 24) & 0xFF);

    SPDLOG_DEBUG("Detecting sample rate for FourCC: {}", fourccStr);

    // Use typical rates for known sensor types
    if (fourcc == MAKEID('G', 'Y', 'R', 'O')) {
      SPDLOG_DEBUG("Using default gyro rate: 200 Hz");
      return 200.0f; // Typical GoPro gyro rate
    }
    if (fourcc == MAKEID('A', 'C', 'C', 'L')) {
      SPDLOG_DEBUG("Using default accel rate: 200 Hz");
      return 200.0f; // Typical GoPro accel rate
    }
    if (fourcc == MAKEID('G', 'P', 'S', '5')) {
      SPDLOG_DEBUG("Using default GPS rate: 18 Hz (GoPro Hero 8+ typical max)");
      return 18.0f; // Typical GoPro GPS rate (can be 1, 5, 10, or 18 Hz)
    }

    // Fallback: return 0 to indicate unknown
    SPDLOG_WARN("Could not determine sample rate for FourCC: {}, returning 0", fourccStr);
    return 0.0f;
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

  nlohmann::json GpmfParser::extractDeviceInfo(GPMF_stream* stream)
  {
    nlohmann::json deviceInfo;

    if (stream == nullptr) {
      return deviceInfo;
    }

    // Get device ID (DVID)
    uint32_t deviceId = GPMF_DeviceID(stream);
    if (deviceId > 0) {
      deviceInfo["device_id"] = deviceId;
    }

    // Save current position
    GPMF_stream tempStream = *stream;

    // Look for device name (DVNM)
    if (GPMF_FindPrev(&tempStream, MAKEID('D', 'V', 'N', 'M'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* dvnmData = static_cast<char*>(GPMF_RawData(&tempStream));
      uint32_t dvnmSize = GPMF_RawDataSize(&tempStream);
      if (dvnmData != nullptr && dvnmSize > 0) {
        std::string dvnm(dvnmData, dvnmSize);
        // Trim null terminators
        dvnm.erase(std::find(dvnm.begin(), dvnm.end(), '\0'), dvnm.end());
        if (!dvnm.empty()) {
          deviceInfo["device_name"] = dvnm;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Look for version (VERS)
    if (GPMF_FindPrev(&tempStream, MAKEID('V', 'E', 'R', 'S'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* versData = static_cast<char*>(GPMF_RawData(&tempStream));
      uint32_t versSize = GPMF_RawDataSize(&tempStream);
      if (versData != nullptr && versSize > 0) {
        std::string vers(versData, versSize);
        vers.erase(std::find(vers.begin(), vers.end(), '\0'), vers.end());
        if (!vers.empty()) {
          deviceInfo["version"] = vers;
        }
      }
    }

    return deviceInfo;
  }

  nlohmann::json GpmfParser::extractStreamMetadata(GPMF_stream* stream, [[maybe_unused]] uint32_t fourcc)
  {
    SPDLOG_INFO("Extracting stream metadata");
    nlohmann::json metadata;

    if (stream == nullptr) {
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

    // Save current position for metadata searches
    GPMF_stream tempStream = *stream;

    // Get scaling factors (SCAL)
    if (GPMF_FindPrev(&tempStream, MAKEID('S', 'C', 'A', 'L'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* scaleData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      uint32_t scaleCount = GPMF_Repeat(&tempStream);
      if (scaleData != nullptr && scaleCount > 0) {
        std::vector<uint32_t> scales;
        for (uint32_t i = 0; i < scaleCount; i++) {
          scales.push_back(scaleData[i]);
        }
        metadata["units"]["scale"] = scales;
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get SI units (SIUN)
    if (GPMF_FindPrev(&tempStream, MAKEID('S', 'I', 'U', 'N'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* siunData = static_cast<char*>(GPMF_RawData(&tempStream));
      uint32_t siunSize = GPMF_RawDataSize(&tempStream);
      if (siunData != nullptr && siunSize > 0) {
        std::string siun(siunData, siunSize);
        siun.erase(std::find(siun.begin(), siun.end(), '\0'), siun.end());
        if (!siun.empty()) {
          metadata["units"]["siun"] = siun;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get units (UNIT)
    if (GPMF_FindPrev(&tempStream, MAKEID('U', 'N', 'I', 'T'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* unitData = static_cast<char*>(GPMF_RawData(&tempStream));
      uint32_t unitSize = GPMF_RawDataSize(&tempStream);
      if (unitData != nullptr && unitSize > 0) {
        std::string unit(unitData, unitSize);
        unit.erase(std::find(unit.begin(), unit.end(), '\0'), unit.end());
        if (!unit.empty()) {
          metadata["units"]["unit"] = unit;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get timestamp info (TSMP)
    if (GPMF_FindPrev(&tempStream, MAKEID('T', 'S', 'M', 'P'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* tsmpData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      if (tsmpData != nullptr) {
        metadata["timestamp_info"]["tsmp"] = tsmpData[0];
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get ORIN (original sample rate or orientation)
    if (GPMF_FindPrev(&tempStream, MAKEID('O', 'R', 'I', 'N'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      GPMF_SampleType orinType = GPMF_Type(&tempStream);
      auto* orinData = GPMF_RawData(&tempStream);

      if (orinType == GPMF_TYPE_UNSIGNED_LONG || orinType == GPMF_TYPE_SIGNED_LONG) {
        // Numeric: sample rate
        uint32_t orinValue = *static_cast<uint32_t*>(orinData);
        metadata["timestamp_info"]["orin"] = orinValue;
      } else if (orinType == GPMF_TYPE_STRING_ASCII) {
        // String: orientation
        uint32_t orinSize = GPMF_RawDataSize(&tempStream);
        std::string orin(static_cast<char*>(orinData), orinSize);
        orin.erase(std::find(orin.begin(), orin.end(), '\0'), orin.end());
        if (!orin.empty()) {
          metadata["orientation"]["input"] = orin;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get ORIO (output orientation)
    if (GPMF_FindPrev(&tempStream, MAKEID('O', 'R', 'I', 'O'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* orioData = static_cast<char*>(GPMF_RawData(&tempStream));
      uint32_t orioSize = GPMF_RawDataSize(&tempStream);
      if (orioData != nullptr && orioSize > 0) {
        std::string orio(orioData, orioSize);
        orio.erase(std::find(orio.begin(), orio.end(), '\0'), orio.end());
        if (!orio.empty()) {
          metadata["orientation"]["output"] = orio;
        }
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get TICK (start time)
    if (GPMF_FindPrev(&tempStream, MAKEID('T', 'I', 'C', 'K'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* tickData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      if (tickData != nullptr) {
        metadata["timestamp_info"]["tick"] = tickData[0];
      }
    }

    // Reset for next search
    tempStream = *stream;

    // Get TOCK (end time)
    if (GPMF_FindPrev(&tempStream, MAKEID('T', 'O', 'C', 'K'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* tockData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      if (tockData != nullptr) {
        metadata["timestamp_info"]["tock"] = tockData[0];
      }
    }

    return metadata;
  }

  //! Convert nested object to JSON.
  nlohmann::json GpmfParser::nestedToJson(GPMF_stream *stream, [[maybe_unused]] uint32_t fourcc)
  {
    nlohmann::json ret;
    do {
      uint32_t fourXcc = GPMF_Key(stream);
      std::string strFourCC = fourccToString(fourXcc);
      ret[strFourCC] = samplesToJson(stream, fourXcc);
    } while (GPMF_OK == GPMF_Next(stream, GPMF_RECURSE_LEVELS));

    return ret;
  }

  nlohmann::json GpmfParser::samplesToJson(GPMF_stream *stream, [[maybe_unused]] uint32_t fourcc)
  {
    if (stream == nullptr) {
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
    if(elements == 0) {
      return {};
    }

    switch (sampleType) {
      case GPMF_TYPE_STRING_ASCII: {
        std::string_view strView(static_cast<const char *>(GPMF_RawData(stream)), GPMF_RawDataSize(stream));
        return std::string(strView);
      }

      case GPMF_TYPE_SIGNED_BYTE: {
        auto* data = static_cast<const int8_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(static_cast<int>(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(static_cast<int>(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_BYTE: {
        auto* data = static_cast<const uint8_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(static_cast<unsigned>(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(static_cast<unsigned>(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_SIGNED_SHORT: {
        auto* data = static_cast<const int16_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(BYTESWAP16(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP16(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_SHORT: {
        auto* data = static_cast<const uint16_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(BYTESWAP16(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP16(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_SIGNED_LONG: {
        auto* data = static_cast<const int32_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(BYTESWAP32(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP32(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_LONG: {
        auto* data = static_cast<const uint32_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(BYTESWAP32(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP32(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_FLOAT: {
        auto* data = static_cast<const uint32_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            uint32_t swap = BYTESWAP32(data[i]);
            float f = *reinterpret_cast<float*>(&swap);
            samples.push_back(f);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              uint32_t swap = BYTESWAP32(data[i * elements + j]);
              float f = *reinterpret_cast<float*>(&swap);
              sample.push_back(f);
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_DOUBLE: {
        auto* data = static_cast<const uint64_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            uint64_t swap = BYTESWAP64(data[i]);
            double d = *reinterpret_cast<double*>(&swap);
            samples.push_back(d);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              uint64_t swap = BYTESWAP64(data[i * elements + j]);
              double d = *reinterpret_cast<double*>(&swap);
              sample.push_back(d);
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_Q15_16_FIXED_POINT: {
        auto* data = static_cast<const int32_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            double dq = static_cast<double>(BYTESWAP32(data[i])) / 65536.0;
            samples.push_back(dq);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              double dq = static_cast<double>(BYTESWAP32(data[i * elements + j])) / 65536.0;
              sample.push_back(dq);
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_Q31_32_FIXED_POINT: {
        auto* data = static_cast<const int64_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            uint64_t Q64 = BYTESWAP64(static_cast<uint64_t>(data[i]));
            double dq = static_cast<double>(Q64 >> 32);
            dq += static_cast<double>(Q64 & 0xFFFFFFFF) / 4294967296.0;
            samples.push_back(dq);
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
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
        auto* data = static_cast<const int64_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(BYTESWAP64(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP64(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_UNSIGNED_64BIT_INT: {
        auto* data = static_cast<const uint64_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(BYTESWAP64(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(BYTESWAP64(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_FOURCC: {
        auto* data = static_cast<const uint32_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        for (uint32_t i = 0; i < sampleCount; i++) {
          if (elements == 1) {
            samples.push_back(fourccToString(data[i]));
          } else {
            nlohmann::json sample = nlohmann::json::array();
            for (uint32_t j = 0; j < elements; j++) {
              sample.push_back(fourccToString(data[i * elements + j]));
            }
            samples.push_back(sample);
          }
        }
        return samples;
      }

      case GPMF_TYPE_GUID: {
        auto* data = static_cast<const uint8_t*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        uint32_t guidSize = 16; // GUIDs are 128 bits = 16 bytes
        for (uint32_t i = 0; i < sampleCount; i++) {
          std::string guid;
          for (uint32_t j = 0; j < guidSize; j++) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02X", data[i * guidSize + j]);
            guid += hex;
          }
          samples.push_back(guid);
        }
        return samples;
      }

      case GPMF_TYPE_UTC_DATE_TIME: {
        auto* data = static_cast<const char*>(GPMF_RawData(stream));
        nlohmann::json samples = nlohmann::json::array();
        uint32_t dateSize = 16; // UTC dates are 16 bytes: yymmddhhmmss.sss
        for (uint32_t i = 0; i < sampleCount; i++) {
          std::string dateStr(data + i * dateSize, dateSize);
          // Trim null terminators
          dateStr.erase(std::find(dateStr.begin(), dateStr.end(), '\0'), dateStr.end());
          samples.push_back(dateStr);
        }
        return samples;
      }

      case GPMF_TYPE_STRING_UTF8: {
        std::string_view strView(static_cast<const char*>(GPMF_RawData(stream)), GPMF_RawDataSize(stream));
        return std::string(strView);
      }

      case GPMF_TYPE_COMPLEX: {
        // Complex types have opaque data - return as hex string or raw info
        nlohmann::json result;
        result["type"] = "complex";
        result["size_bytes"] = GPMF_RawDataSize(stream);
        result["sample_count"] = sampleCount;
        result["note"] = "Complex type - opaque data structure";
        return result;
      }

      case GPMF_TYPE_COMPRESSED: {
        // Compressed data needs decompression first
        nlohmann::json result;
        result["type"] = "compressed";
        result["size_bytes"] = GPMF_RawDataSize(stream);
        result["sample_count"] = sampleCount;
        result["note"] = "Compressed data - needs decompression";
        return result;
      }

      case GPMF_TYPE_NEST: {
        return nestedToJson(stream, fourcc);
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

} // namespace Ravl2::GoPro
