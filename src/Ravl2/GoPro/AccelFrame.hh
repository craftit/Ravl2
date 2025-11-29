//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/Video/MetaDataFrame.hh"
#include "Ravl2/GoPro/GpmfTypes.hh"
#include "Ravl2/Logging.hh"
#include <vector>

namespace Ravl2::GoPro
{
  //! Frame containing accelerometer telemetry (may contain multiple samples)
  //! High-frequency sensors like accelerometer typically have multiple samples per frame
  //! to match the GPMF packet structure
  class AccelFrame : public Video::MetaDataFrame<std::vector<AccelSample>>
  {
  public:
    //! Constructor with accel data, ID, timestamp, and sample rate
    //! @param data Vector of accelerometer samples
    //! @param id Unique stream item identifier
    //! @param timestamp Media timestamp for this frame
    //! @param sampleRate Samples per second (must be > 0, typical GoPro: 200-400 Hz)
    AccelFrame(const std::vector<AccelSample>& data,
               Video::StreamItemId id,
               Video::MediaTime timestamp,
               float sampleRate)
      : Video::MetaDataFrame<std::vector<AccelSample>>(data, id, timestamp)
        , mSampleRate(sampleRate)
    {
      // Validate sample rate
      if (sampleRate <= 0.0F) {
        SPDLOG_ERROR("Invalid accel sample rate: {} Hz (must be > 0)", sampleRate);
        mSampleRate = 0.0F;
      } else if (sampleRate < 50.0F || sampleRate > 1000.0F) {
        // Warn for unusual rates outside typical GoPro range (200-400 Hz)
        SPDLOG_WARN("Unusual accel sample rate: {} Hz (typical GoPro: 200-400 Hz)", sampleRate);
      }
    }

    //! Get the sample rate
    [[nodiscard]] float sampleRate() const { return mSampleRate; }

    //! Get the data type name
    [[nodiscard]] std::string dataTypeName() const override
    {
      return "GoPro::AccelSample[]";
    }

    //! Convenience: Access raw Vector3f array for processing
    //! This allows direct use with RAVL2 Array or signal processing
    [[nodiscard]] std::vector<Vector3f> asVectorArray() const
    {
      std::vector<Vector3f> result;
      result.reserve(data().size());
      for (const auto& sample : data()) {
        result.push_back(sample.acceleration);
      }
      return result;
    }

  private:
    float mSampleRate = 0.0F; //!< Samples per second (0 indicates invalid/unset)
  };

} // namespace Ravl2::GoPro
