//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/GoPro/GpmfTypes.hh"
#include "Ravl2/Video/MetaDataFrame.hh"
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include <memory>

// Forward declare GPMF_stream to avoid including C header in our header
struct GPMF_stream;

namespace Ravl2::GoPro
{
  //! C++ wrapper for the C-based gpmf-parser library
  //! Handles parsing of GoPro GPMF metadata from video streams
  class GpmfParser
  {
  public:
    //! Constructor
    //! @param enableJson If true, generate JSON frames for all GPMF data alongside typed frames
    explicit GpmfParser(bool enableJson = true);

    //! Destructor
    ~GpmfParser() = default;

    // Disable copy (contains pointer to C struct)
    GpmfParser(const GpmfParser&) = delete;
    GpmfParser& operator=(const GpmfParser&) = delete;

    // Enable move
    GpmfParser(GpmfParser&&) noexcept  = default;
    GpmfParser& operator=(GpmfParser&&) noexcept  = default;

    //! Parse GPMF data from a packet and extract frames
    //! @param data Raw GPMF data
    //! @param size Size of data in bytes
    //! @param streamId Stream identifier for creating frames
    //! @param timestamp Base timestamp for this packet
    //! @return Vector of Frame pointers (may be GPS, Gyro, Accel, etc.)
    [[nodiscard]] std::vector<std::shared_ptr<Video::Frame>> parse(
      const uint8_t* data,
      size_t size,
      Video::StreamItemId streamId,
      Video::MediaTime timestamp);

    //! Parse GPS data from GPMF stream
    //! @param stream GPMF stream positioned at GPS data
    //! @return GPS samples with sample rate if found and valid
    [[nodiscard]] std::optional<GpsSamples> parseGps(GPMF_stream* stream);

    //! Parse gyroscope data from GPMF stream
    //! @param stream GPMF stream positioned at gyro data
    //! @return Gyro samples with sample rate (empty if parsing failed)
    [[nodiscard]] std::optional<GyroSamples> parseGyro(GPMF_stream* stream);

    //! Parse accelerometer data from GPMF stream
    //! @param stream GPMF stream positioned at accel data
    //! @return Accel samples with sample rate (empty if parsing failed)
    [[nodiscard]] std::optional<AccelSamples> parseAccel(GPMF_stream* stream);

    //! Parse GPMF data to comprehensive JSON representation
    //! Captures all GPMF streams (GPS, GYRO, ACCL, and any others) with complete metadata
    //! @param data Raw GPMF data
    //! @param size Size of data in bytes
    //! @param streamId Stream identifier for creating frames
    //! @param timestamp Base timestamp for this packet
    //! @return Vector of JSON frames, one per GPMF packet with all streams
    [[nodiscard]] std::vector<std::shared_ptr<Video::MetaDataFrame<nlohmann::json>>> parseToJson(
      const uint8_t* data,
      size_t size,
      Video::StreamItemId streamId,
      Video::MediaTime timestamp);

  private:
    //! Get scaling factor for a given FourCC
    //! @param stream GPMF stream
    //! @param fourcc FourCC identifier
    //! @return Scale factor (1.0 if not found)
    [[nodiscard]] float getScaleFactor(GPMF_stream* stream, uint32_t fourcc) const;

    //! Get sample rate for a stream
    //! @param stream GPMF stream
    //! @return Sample rate in Hz (0 if not found)
    [[nodiscard]] float getSampleRate(GPMF_stream* stream) const;

    //! Convert GPMF FourCC to string representation
    //! @param fourcc 32-bit FourCC value
    //! @return String representation (4 characters)
    [[nodiscard]] static std::string fourccToString(uint32_t fourcc);

    //! Extract device-level metadata (DVID, DVNM, VERS) from GPMF stream
    //! @param stream GPMF stream positioned at DEVC level
    //! @return JSON object with device info
    [[nodiscard]] nlohmann::json extractDeviceInfo(GPMF_stream* stream);

    //! Extract stream metadata (SCAL, SIUN, TSMP, ORIN, etc.)
    //! @param stream GPMF stream positioned at a data FourCC
    //! @param fourcc The FourCC of the data stream
    //! @return JSON object with stream metadata
    [[nodiscard]] nlohmann::json extractStreamMetadata(GPMF_stream* stream, uint32_t fourcc);

    //! Convert GPMF FourCC to string
    //! @param fourcc 32-bit FourCC value
    //! @return String representation (4 characters)
    [[nodiscard]] nlohmann::json samplesToJson(GPMF_stream* stream, const std::vector<double>& scale);

    //! Convert GPMF stream to JSON object
    //! @param stream GPMF stream positioned at a STRM element
    //! @param deviceInfo JSON object with DEVC-level info
    //! @return JSON object representing the stream, or empty if parsing failed
    [[nodiscard]] nlohmann::json streamToJson(GPMF_stream* stream, const nlohmann::json& deviceInfo);

    //! Internal stream counter for generating unique IDs
    Video::StreamItemId mNextId = 0;

    //! Whether to generate JSON metadata frames
    bool mEnableJson = false;
  };

} // namespace Ravl2::GoPro
