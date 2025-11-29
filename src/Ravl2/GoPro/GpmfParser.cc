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
        frames.push_back(std::make_shared<GpsFrame>(gpsFix.value(), streamId + mNextId++, timestamp));
      }
      GPMF_ResetState(&stream); // Reset for next search
    }

    // Search for gyroscope data (FourCC: GYRO)
    if (GPMF_FindNext(&stream, MAKEID('G', 'Y', 'R', 'O'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      float sampleRate = 0;
      auto gyroSamples = parseGyro(&stream, sampleRate);
      if (!gyroSamples.empty()) {
        frames.push_back(std::make_shared<GyroFrame>(gyroSamples, streamId + mNextId++, timestamp, sampleRate));
      }
      GPMF_ResetState(&stream);
    }

    // Search for accelerometer data (FourCC: ACCL)
    if (GPMF_FindNext(&stream, MAKEID('A', 'C', 'C', 'L'), GPMF_RECURSE_LEVELS) == GPMF_OK) {
      float sampleRate = 0;
      auto accelSamples = parseAccel(&stream, sampleRate);
      if (!accelSamples.empty()) {
        frames.push_back(std::make_shared<AccelFrame>(accelSamples, streamId + mNextId++, timestamp, sampleRate));
      }
    }

    return frames;
  }

  std::optional<GpsFix> GpmfParser::parseGps(GPMF_stream* stream)
  {
    if (stream == nullptr) {
      return std::nullopt;
    }

    uint32_t samples = GPMF_Repeat(stream);
    if (samples == 0) {
      return std::nullopt;
    }

    // GPS5 format: latitude, longitude, altitude, 2D speed, 3D speed
    // All values are scaled integers
    float scale = getScaleFactor(stream, MAKEID('G', 'P', 'S', '5'));
    if (scale == 0) {
      scale = 1.0f; // Default scale
    }

    // Get raw data
    auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
    if (rawData == nullptr) {
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

  std::vector<GyroSample> GpmfParser::parseGyro(GPMF_stream* stream, float& sampleRate)
  {
    std::vector<GyroSample> samples;

    if (stream == nullptr) {
      return samples;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      return samples;
    }

    // Get scale factor
    float scale = getScaleFactor(stream, MAKEID('G', 'Y', 'R', 'O'));
    if (scale == 0) {
      scale = 1.0f;
    }

    // Get sample rate
    sampleRate = getSampleRate(stream);

    // Get raw data (3 int16 values per sample: x, y, z)
    auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
    if (rawData == nullptr) {
      return samples;
    }

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

    return samples;
  }

  std::vector<AccelSample> GpmfParser::parseAccel(GPMF_stream* stream, float& sampleRate)
  {
    std::vector<AccelSample> samples;

    if (stream == nullptr) {
      return samples;
    }

    uint32_t sampleCount = GPMF_Repeat(stream);
    if (sampleCount == 0) {
      return samples;
    }

    // Get scale factor
    float scale = getScaleFactor(stream, MAKEID('A', 'C', 'C', 'L'));
    if (scale == 0) {
      scale = 1.0f;
    }

    // Get sample rate
    sampleRate = getSampleRate(stream);

    // Get raw data (3 int16 values per sample: x, y, z)
    auto* rawData = static_cast<int16_t*>(GPMF_RawData(stream));
    if (rawData == nullptr) {
      return samples;
    }

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

    return samples;
  }

  float GpmfParser::getScaleFactor(GPMF_stream* stream, [[maybe_unused]] uint32_t fourcc) const
  {
    if (stream == nullptr) {
      return 1.0f;
    }

    // Save current position
    GPMF_stream tempStream = *stream;

    // Look for SCAL (scale) field
    if (GPMF_FindPrev(&tempStream, MAKEID('S', 'C', 'A', 'L'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
      auto* scaleData = static_cast<uint32_t*>(GPMF_RawData(&tempStream));
      if (scaleData != nullptr) {
        // Scale is typically stored as an integer divisor
        return 1.0f / static_cast<float>(*scaleData);
      }
    }

    return 1.0f;
  }

  float GpmfParser::getSampleRate([[maybe_unused]] GPMF_stream* stream) const
  {
    if (stream == nullptr) {
      return 0.0f;
    }

    // Look for ORIN (original sample rate) or similar
    // This is a simplified version - actual implementation may vary
    // For now, return a default value
    return 200.0f; // Typical GoPro gyro/accel rate
  }

} // namespace Ravl2::GoPro
