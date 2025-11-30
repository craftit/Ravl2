//
// Created for RAVL2 GoPro metadata support
//

#include "Ravl2/GoPro/GpmfParser.hh"
#include "Ravl2/Logging.hh"

// Include GPMF parser C headers
extern "C" {
#include <GPMF_parser.h>
}

#include <cstring>

namespace Ravl2::GoPro
{

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

} // namespace Ravl2::GoPro
