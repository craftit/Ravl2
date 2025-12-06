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
#include <map>

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
    //! @note This handles both GPS5 (Hero 8) and GPS9 (Hero 13+) formats by detecting the type
    void parseGps(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp);

    //! Parse GPS9 complex type data using TYPE descriptor
    //! @param stream GPMF stream positioned at GPS9 data (type '?')
    //! @param frames Vector to append GPS frames to
    //! @param streamId Stream identifier for creating frames
    //! @param timestamp Base timestamp for this packet
    //! @note GPS9 uses complex type '?' with TYPE descriptor "lllllllSS" (7 longs + 2 shorts)
    void parseGps9Complex(GPMF_stream* stream, std::vector<std::shared_ptr<Video::Frame>>& frames, Video::StreamItemId streamId, Video::MediaTime timestamp);

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

    //! Find and read TYPE descriptor for complex types
    //! @param stream GPMF stream positioned at complex data
    //! @return TYPE descriptor string (e.g., "lllllllSS"), or empty if not found
    [[nodiscard]] std::string getTypeDescriptor(GPMF_stream* stream) const;

    //! Find and read SCAL (scale) values for a stream
    //! @param stream GPMF stream
    //! @param expectedCount Expected number of scale values
    //! @return Vector of scale factors (empty if not found)
    [[nodiscard]] std::vector<int32_t> getScaleFactors(GPMF_stream* stream, uint32_t expectedCount) const;

    //! Find and read STNM (stream name) for a stream
    //! @param stream GPMF stream
    //! @return Stream name (empty if not found)
    [[nodiscard]] std::string getStreamName(GPMF_stream* stream) const;

    //! Find and read SIUN (SI units) for a stream
    //! @param stream GPMF stream
    //! @return SI units string (empty if not found)
    [[nodiscard]] std::string getSiUnits(GPMF_stream* stream) const;

    //! Find and read UNIT (per-field units) for a stream
    //! @param stream GPMF stream
    //! @return Vector of unit strings (empty if not found)
    [[nodiscard]] std::vector<std::string> getUnits(GPMF_stream* stream) const;

    //! Convert GPMF FourCC to string representation
    //! @param fourcc 32-bit FourCC value
    //! @return String representation (4 characters)
    [[nodiscard]] static std::string fourccToString(uint32_t fourcc);


    //! Convert GPMF data to JSON based on type
    //! Handles all GPMF types including numeric, string, and complex types
    //! @param stream GPMF stream positioned at data
    //! @param fourcc The FourCC of the data stream
    //! @param level
    //! @return JSON representation of the data (array, object, or scalar)
    [[nodiscard]] nlohmann::json samplesToJson(GPMF_stream *stream, uint32_t fourcc, int level);

    //! Parse complex type data to JSON using TYPE descriptor
    //! @param stream GPMF stream positioned at complex data
    //! @param typeDesc TYPE descriptor string (e.g., "lllllllSS", "Ff", "bb")
    //! @param scales Scale factors for each field (empty if no SCAL)
    //! @param sampleCount Number of samples
    //! @return JSON array of parsed samples
    [[nodiscard]] nlohmann::json parseComplexTypeToJson(GPMF_stream *stream, const std::string& typeDesc, const std::vector<int32_t>& scales, uint32_t sampleCount) const;

    //! Convert nested object to JSON.
    [[nodiscard]] nlohmann::json nestedToJson(GPMF_stream *stream, uint32_t fourcc, int level);

    //! Internal stream counter for generating unique IDs
    Video::StreamItemId mNextId = 0;

    //! Whether to generate JSON metadata frames
    bool mEnableJson = true;
    bool mVerbose = false;

    //! Track timing information for sample rate calculation
    //! Maps FourCC -> {last_stmp, last_sample_count, calculated_rate}
    struct TimingInfo {
      uint64_t lastStmp = 0;      // Last STMP value (microseconds)
      uint32_t lastSampleCount = 0; // Last TSMP value (sample count)
      float calculatedRate = 0.0f;   // Calculated sample rate (Hz)
      bool hasData = false;          // Whether we have previous data
    };
    mutable std::map<uint32_t, TimingInfo> mTimingInfo;
    bool mHaveReportedGyroFPS = false;
    bool mHaveReportedAccelFPS = false;
  };

} // namespace Ravl2::GoPro
