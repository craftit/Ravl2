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
    if (GPMF_Validate(&stream, GPMF_RECURSE_LEVELS) != GPMF_OK) {
      SPDLOG_WARN("GPMF stream validation failed");
      return frames;
    }

    // Search for GPS data (FourCC: GPS5)
    if (GPMF_FindNext(&stream, MAKEID('G', 'P', 'S', '5'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto gpsSamples = parseGps(&stream);
      if (gpsSamples.has_value() && !gpsSamples->empty()) {
        const auto& samples = gpsSamples.value();
        float sampleRate = samples.sampleRate;

        // Calculate time delta between samples for timestamp interpolation
        Video::MediaTime timeDelta(0);
        if (sampleRate > 0.0F && samples.size() > 1) {
          // Convert sample rate to microseconds per sample
          int64_t deltaUs = static_cast<int64_t>((1.0F / sampleRate) * 1000000.0F);
          timeDelta = Video::MediaTime(deltaUs);
        }

        // Create a separate MetaDataFrame<GpsFix> for each GPS fix
        for (size_t i = 0; i < samples.size(); i++) {
          // Interpolate timestamp for this specific fix
          Video::MediaTime fixTimestamp = timestamp;
          if (timeDelta.count() > 0) {
            fixTimestamp = timestamp + Video::MediaTime(timeDelta.count() * static_cast<int64_t>(i));
          }

          frames.push_back(std::make_shared<Video::MetaDataFrame<GpsFix>>(
            samples.data()[i],
            streamId + mNextId++,
            fixTimestamp));
        }

        SPDLOG_DEBUG("Created {} GPS frames from GPMF packet at timestamp {} μs",
                     samples.size(), timestamp.count());
      }
      GPMF_ResetState(&stream); // Reset for next search
    }

    // Search for gyroscope data (FourCC: GYRO)
    if (GPMF_FindNext(&stream, MAKEID('G', 'Y', 'R', 'O'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto gyroSamples = parseGyro(&stream);
      if (gyroSamples.has_value()) {
        frames.push_back(std::make_shared<Video::MetaDataFrame<GyroSamples>>(
          gyroSamples.value(), streamId + mNextId++, timestamp));
      }
      GPMF_ResetState(&stream);
    }

    // Search for accelerometer data (FourCC: ACCL)
    if (GPMF_FindNext(&stream, MAKEID('A', 'C', 'C', 'L'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto accelSamples = parseAccel(&stream);
      if (accelSamples.has_value()) {
        frames.push_back(std::make_shared<Video::MetaDataFrame<AccelSamples>>(
          accelSamples.value(), streamId + mNextId++, timestamp));
      }
    }

    // If JSON generation is enabled, add JSON frames
    if (mEnableJson) {
      auto jsonFrames = parseToJson(data, size, streamId, timestamp);
      for (auto& frame : jsonFrames) {
        frames.push_back(frame);
      }
    }

    return frames;
  }

  std::optional<GpsSamples> GpmfParser::parseGps(GPMF_stream* stream)
  {
    if (stream == nullptr) {
      SPDLOG_WARN("parseGps: null stream pointer");
      return std::nullopt;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      SPDLOG_DEBUG("parseGps: GPS5 stream has 0 samples");
      return std::nullopt;
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
      return std::nullopt;
    }

    // Parse ALL GPS samples (not just the last one!)
    std::vector<GpsFix> samples;
    samples.reserve(sampleCount);
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

      samples.emplace_back(fix);
    }

    // Validate sample rate
    if (sampleRate <= 0.0F) {
      SPDLOG_ERROR("Invalid GPS sample rate: {} Hz (must be > 0)", sampleRate);
      sampleRate = 0.0F;
    } else if (sampleRate < 1.0F || sampleRate > 20.0F) {
      SPDLOG_WARN("Unusual GPS sample rate: {} Hz (typical GoPro: 1-18 Hz)", sampleRate);
    }

    SPDLOG_DEBUG("parseGps: extracted {} GPS samples at {} Hz", sampleCount, sampleRate);

    return GpsSamples(samples, sampleRate);
  }

  std::optional<GyroSamples> GpmfParser::parseGyro(GPMF_stream* stream)
  {
    if (stream == nullptr) {
      SPDLOG_WARN("parseGyro: null stream pointer");
      return std::nullopt;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      SPDLOG_DEBUG("parseGyro: GYRO stream has 0 samples");
      return std::nullopt;
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
      return std::nullopt;
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

    return GyroSamples(samples, sampleRate);
  }

  std::optional<AccelSamples> GpmfParser::parseAccel(GPMF_stream* stream)
  {
    if (stream == nullptr) {
      SPDLOG_WARN("parseAccel: null stream pointer");
      return std::nullopt;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      SPDLOG_DEBUG("parseAccel: ACCL stream has 0 samples");
      return std::nullopt;
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
      return std::nullopt;
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

    return AccelSamples(samples, sampleRate);
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

    // Get sample rate
    float sampleRate = getSampleRate(stream);
    if (sampleRate > 0) {
      metadata["sample_rate_hz"] = sampleRate;
    }

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

  nlohmann::json GpmfParser::samplesToJson(GPMF_stream* stream, [[maybe_unused]] const std::vector<double>& scale)
  {
    nlohmann::json samples = nlohmann::json::array();

    if (stream == nullptr) {
      return samples;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    uint32_t elements = GPMF_ElementsInStruct(stream);

    // Use GPMF_ScaledData for automatic type conversion and scaling
    uint32_t bufferSize = sampleCount * elements * sizeof(double);
    std::vector<double> buffer(sampleCount * elements);

    GPMF_ERR err = GPMF_ScaledData(stream, buffer.data(), bufferSize, 0, sampleCount, GPMF_TYPE_DOUBLE);

    if (err == GPMF_OK) {
      // Convert to JSON array
      for (uint32_t i = 0; i < sampleCount; i++) {
        if (elements == 1) {
          // Scalar value
          samples.push_back(buffer[i]);
        } else {
          // Array of values
          nlohmann::json sample = nlohmann::json::array();
          for (uint32_t j = 0; j < elements; j++) {
            sample.push_back(buffer[i * elements + j]);
          }
          samples.push_back(sample);
        }
      }
    } else {
      SPDLOG_WARN("Failed to extract scaled data for JSON conversion (error={})", static_cast<int>(err));
    }

    return samples;
  }

  nlohmann::json GpmfParser::streamToJson(GPMF_stream* stream, [[maybe_unused]] const nlohmann::json& deviceInfo)
  {
    nlohmann::json streamJson;

    if (stream == nullptr) {
      return streamJson;
    }

    // Get the stream's data FourCC by seeking to samples
    GPMF_stream dataStream = *stream;
    if (GPMF_SeekToSamples(&dataStream) != GPMF_OK) {
      SPDLOG_WARN("Failed to seek to samples in STRM");
      return streamJson;
    }

    uint32_t fourcc = GPMF_Key(&dataStream);
    std::string fourccStr = fourccToString(fourcc);

    streamJson["stream_type"] = fourccStr;

    // Get stream name (STNM) if available
    GPMF_stream tempStream = dataStream;
    if (GPMF_FindPrev(&tempStream, MAKEID('S', 'T', 'N', 'M'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* stnmData = static_cast<char*>(GPMF_RawData(&tempStream));
      uint32_t stnmSize = GPMF_RawDataSize(&tempStream);
      if (stnmData != nullptr && stnmSize > 0) {
        std::string stnm(stnmData, stnmSize);
        // Trim null terminators and whitespace
        stnm.erase(std::find(stnm.begin(), stnm.end(), '\0'), stnm.end());
        if (!stnm.empty()) {
          streamJson["stream_name"] = stnm;
        }
      }
    }

    // Extract metadata
    streamJson["metadata"] = extractStreamMetadata(&dataStream, fourcc);

    // Extract scale factors for data conversion
    std::vector<double> scales;
    if (streamJson["metadata"].contains("units") && streamJson["metadata"]["units"].contains("scale")) {
      for (auto& s : streamJson["metadata"]["units"]["scale"]) {
        scales.push_back(1.0 / s.get<double>());
      }
    } else {
      // Default scale of 1.0
      uint32_t elements = GPMF_ElementsInStruct(&dataStream);
      scales.resize(elements, 1.0);
    }

    // Extract samples
    streamJson["samples"] = samplesToJson(&dataStream, scales);

    return streamJson;
  }

  std::vector<std::shared_ptr<Video::MetaDataFrame<nlohmann::json>>> GpmfParser::parseToJson(
    const uint8_t* data,
    size_t size,
    Video::StreamItemId streamId,
    Video::MediaTime timestamp)
  {
    std::vector<std::shared_ptr<Video::MetaDataFrame<nlohmann::json>>> frames;

    if (data == nullptr || size == 0) {
      return frames;
    }

    // Initialize GPMF stream
    GPMF_stream stream;
    GPMF_ERR initResult = GPMF_Init(&stream, const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(data)), static_cast<uint32_t>(size));
    if (initResult != GPMF_OK) {
      SPDLOG_WARN("Failed to initialize GPMF stream for JSON parsing (error={})", static_cast<int>(initResult));
      return frames;
    }

    // Validate the stream structure
    if (GPMF_Validate(&stream, static_cast<GPMF_LEVELS>(GPMF_RECURSE_LEVELS | GPMF_TOLERANT)) != GPMF_OK) {
      SPDLOG_WARN("GPMF stream validation failed for JSON parsing");
      return frames;
    }

    // Create top-level JSON object for this packet
    nlohmann::json packetJson;
    packetJson["packet_info"]["timestamp_us"] = timestamp.count();
    packetJson["packet_info"]["stream_id"] = streamId;
    packetJson["packet_info"]["packet_size_bytes"] = size;

    // Find all DEVC (device) containers
    GPMF_ResetState(&stream);

    while (GPMF_FindNext(&stream, MAKEID('D', 'E', 'V', 'C'), static_cast<GPMF_LEVELS>(GPMF_RECURSE_LEVELS | GPMF_TOLERANT)) == GPMF_OK) {
      // Extract device info
      nlohmann::json deviceInfo = extractDeviceInfo(&stream);
      packetJson["device"] = deviceInfo;

      // Find all STRM (stream) containers within this DEVC
      GPMF_stream deviceStream = stream;

      nlohmann::json streams = nlohmann::json::array();

      while (GPMF_FindNext(&deviceStream, MAKEID('S', 'T', 'R', 'M'), static_cast<GPMF_LEVELS>(GPMF_CURRENT_LEVEL | GPMF_TOLERANT)) == GPMF_OK) {
        nlohmann::json streamJson = streamToJson(&deviceStream, deviceInfo);
        if (!streamJson.empty()) {
          streams.push_back(streamJson);
        }
      }

      if (!streams.empty()) {
        packetJson["streams"] = streams;
      }

      // For now, we only process the first DEVC
      // In theory, there could be multiple devices, but GoPro cameras typically have one
      break;
    }

    // Create a single frame with all the JSON data for this packet
    if (!packetJson.empty() && packetJson.contains("streams")) {
      auto jsonFrame = std::make_shared<Video::MetaDataFrame<nlohmann::json>>(
        packetJson,
        streamId + mNextId++,
        timestamp
      );
      frames.push_back(jsonFrame);

      SPDLOG_DEBUG("Created JSON frame from GPMF packet at timestamp {} μs with {} streams",
                   timestamp.count(), packetJson["streams"].size());
    }

    return frames;
  }

} // namespace Ravl2::GoPro
