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

    //! Parse GPS data from GPMF stream and append frames
    //! @param stream GPMF stream positioned at GPS data
    //! @param frames Vector to append GPS frames to
    //! @param streamId Stream identifier for creating frames
    //! @param timestamp Base timestamp for this packet
    void parseGps(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp);

    //! Parse gyroscope data from GPMF stream and append frame
    //! @param stream GPMF stream positioned at gyro data
    //! @param frames Vector to append gyro frame to
    //! @param streamId Stream identifier for creating frames
    //! @param timestamp Base timestamp for this packet
    void parseGyro(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp);

    //! Parse accelerometer data from GPMF stream and append frame
    //! @param stream GPMF stream positioned at accel data
    //! @param frames Vector to append accel frame to
    //! @param streamId Stream identifier for creating frames
    //! @param timestamp Base timestamp for this packet
    void parseAccel(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp);

    //! Set verbose mode
    void setVerbose(bool verbose)
    { mVerbose = verbose; }

  private:
    //! Process a level
    std::vector<std::shared_ptr<Video::Frame>> processLevel(GPMF_stream* levelStream, Video::StreamItemId streamId, Video::MediaTime timestamp, int level);

    //! Get scaling factor for a given FourCC
    //! @param stream GPMF stream
    //! @param fourcc FourCC identifier
    //! @return Scale factor (1.0 if not found)
    [[nodiscard]] float getScaleFactor(GPMF_stream* stream, uint32_t fourcc) const;

    //! Get the sample rate for a stream
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

    //! Convert GPMF data to JSON based on type
    //! Handles all GPMF types including numeric, string, and complex types
    //! @param stream GPMF stream positioned at data
    //! @param fourcc The FourCC of the data stream
    //! @param level
    //! @return JSON representation of the data (array, object, or scalar)
    [[nodiscard]] nlohmann::json samplesToJson(GPMF_stream *stream, uint32_t fourcc, int level);

    //! Convert nested object to JSON.
    [[nodiscard]] nlohmann::json nestedToJson(GPMF_stream *stream, uint32_t fourcc, int level);

    //! Internal stream counter for generating unique IDs
    Video::StreamItemId mNextId = 0;

    //! Whether to generate JSON metadata frames
    bool mEnableJson = true;
    bool mVerbose = false;
  };

} // namespace Ravl2::GoPro
