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
  GpmfParser::GpmfParser() = default;

  GpmfParser::~GpmfParser() = default;

  GpmfParser::GpmfParser(GpmfParser&&) noexcept = default;

  GpmfParser& GpmfParser::operator=(GpmfParser&&) noexcept = default;

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
      SPDLOG_DEBUG("Failed to initialize GPMF stream (error={}), data size={} bytes", static_cast<int>(initResult), size);
      return frames;
    }

    // Validate the stream structure
    if (GPMF_Validate(&stream, GPMF_RECURSE_LEVELS) != GPMF_OK) {
      SPDLOG_WARN("GPMF stream validation failed");
      return frames;
    }

    // Search for GPS data (FourCC: GPS5)
    if (GPMF_FindNext(&stream, MAKEID('G', 'P', 'S', '5'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      auto gpsFix = parseGps(&stream);
      if (gpsFix.has_value()) {
        frames.push_back(std::make_shared<Video::MetaDataFrame<GpsFix>>(
          gpsFix.value(), streamId + mNextId++, timestamp));
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

  std::optional<GpsFix> GpmfParser::parseGps(GPMF_stream* stream)
  {
    if (stream == nullptr) {
      SPDLOG_WARN("parseGps: null stream pointer");
      return std::nullopt;
    }

    uint32_t samples = GPMF_Repeat(stream);
    if (samples == 0) {
      SPDLOG_DEBUG("parseGps: GPS5 stream has 0 samples");
      return std::nullopt;
    }

    // GPS5 format: latitude, longitude, altitude, 2D speed, 3D speed
    // All values are scaled integers
    float scale = getScaleFactor(stream, MAKEID('G', 'P', 'S', '5'));
    if (scale == 0) {
      SPDLOG_WARN("parseGps: invalid scale factor, using default 1.0");
      scale = 1.0f; // Default scale
    }

    // Get raw data
    auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
    if (rawData == nullptr) {
      SPDLOG_ERROR("parseGps: failed to get raw data from GPMF stream (samples={})", samples);
      return std::nullopt;
    }

    // Parse the GPS data (use the last sample if multiple)
    size_t offset = (samples - 1) * 5; // 5 values per sample
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

    return fix;
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

    std::vector<GyroSample> samples;
    samples.reserve(sampleCount);
    for (uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * 3;
      Vector3f angularVelocity(
        static_cast<float>(rawData[offset + 0]) * scale,
        static_cast<float>(rawData[offset + 1]) * scale,
        static_cast<float>(rawData[offset + 2]) * scale
      );
      samples.emplace_back(angularVelocity);
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

    std::vector<AccelSample> samples;
    samples.reserve(sampleCount);
    for (uint32_t i = 0; i < sampleCount; i++) {
      size_t offset = i * 3;
      Vector3f acceleration(
        static_cast<float>(rawData[offset + 0]) * scale,
        static_cast<float>(rawData[offset + 1]) * scale,
        static_cast<float>(rawData[offset + 2]) * scale
      );
      samples.emplace_back(acceleration);
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

    // NOTE: Proper sample rate calculation requires analyzing multiple GPMF packets
    // over time using GetGPMFSampleRate() from GPMF_utils, which needs access to
    // the entire MP4 container and all payloads. Since we're parsing packets
    // one-at-a-time in the FFmpeg iterator, we can't perform this calculation here.
    //
    // The GPMF format doesn't embed a simple "sample rate" field in each packet.
    // Fields like ORIN, TSMP, and TIMO have different meanings and can't be
    // directly interpreted as Hz without cross-packet analysis.
    //
    // For now, we return the typical GoPro Hero 8 sensor rate (200 Hz).
    // TODO: Enhance FfmpegMultiStreamIterator to track sample timing across
    // packets and calculate actual rates, or expose GetGPMFSampleRate() via
    // a higher-level API that has access to the full container.

    return 200.0f; // Typical GoPro Hero 8 gyro/accel sample rate
  }

} // namespace Ravl2::GoPro
