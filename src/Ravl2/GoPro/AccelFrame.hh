//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/GoPro/GpmfFrame.hh"
#include "Ravl2/GoPro/GpmfTypes.hh"
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
    AccelFrame(const std::vector<AccelSample>& data,
               Video::StreamItemId id,
               Video::MediaTime timestamp,
               float sampleRate)
      : Video::MetaDataFrame<std::vector<AccelSample>>(data, id, timestamp)
        , mSampleRate(sampleRate)
    {}

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
    float mSampleRate = 0; //!< Samples per second
  };

} // namespace Ravl2::GoPro
