//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/GoPro/GpmfTypes.hh"
#include "Ravl2/Video/MetaDataFrame.hh"
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
    GpmfParser();

    //! Destructor
    ~GpmfParser();

    // Disable copy (contains pointer to C struct)
    GpmfParser(const GpmfParser&) = delete;
    GpmfParser& operator=(const GpmfParser&) = delete;

    // Enable move
    GpmfParser(GpmfParser&&) noexcept;
    GpmfParser& operator=(GpmfParser&&) noexcept;

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
    //! @return GPS fix if found and valid
    [[nodiscard]] std::optional<GpsFix> parseGps(GPMF_stream* stream);

    //! Parse gyroscope data from GPMF stream
    //! @param stream GPMF stream positioned at gyro data
    //! @return Gyro samples with sample rate (empty if parsing failed)
    [[nodiscard]] std::optional<GyroSamples> parseGyro(GPMF_stream* stream);

    //! Parse accelerometer data from GPMF stream
    //! @param stream GPMF stream positioned at accel data
    //! @return Accel samples with sample rate (empty if parsing failed)
    [[nodiscard]] std::optional<AccelSamples> parseAccel(GPMF_stream* stream);

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

    //! Internal stream counter for generating unique IDs
    Video::StreamItemId mNextId = 0;
  };

} // namespace Ravl2::GoPro
