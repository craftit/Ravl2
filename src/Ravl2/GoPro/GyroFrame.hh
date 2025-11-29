//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/GoPro/GpmfFrame.hh"
#include "Ravl2/GoPro/GpmfTypes.hh"
#include <vector>

namespace Ravl2::GoPro
{
  //! Frame containing gyroscope telemetry (may contain multiple samples)
  //! High-frequency sensors like gyro typically have multiple samples per frame
  //! to match the GPMF packet structure
  class GyroFrame : public Video::MetaDataFrame<std::vector<GyroSample>>
  {
  public:
    //! Constructor with gyro data, ID, timestamp, and sample rate
    GyroFrame(const std::vector<GyroSample>& data,
              Video::StreamItemId id,
              Video::MediaTime timestamp,
              float sampleRate)
      : Video::MetaDataFrame<std::vector<GyroSample>>(data, id, timestamp)
        , mSampleRate(sampleRate)
    {}

    //! Get the sample rate
    [[nodiscard]] float sampleRate() const { return mSampleRate; }

    //! Get the data type name
    [[nodiscard]] std::string dataTypeName() const override
    {
      return "GoPro::GyroSample[]";
    }

    //! Convenience: Access raw Vector3f array for processing
    //! This allows direct use with RAVL2 Array or signal processing
    [[nodiscard]] std::vector<Vector3f> asVectorArray() const
    {
      std::vector<Vector3f> result;
      result.reserve(data().size());
      for (const auto& sample : data()) {
        result.push_back(sample.angularVelocity);
      }
      return result;
    }

  private:
    float mSampleRate = 0; //!< Samples per second
  };

} // namespace Ravl2::GoPro
