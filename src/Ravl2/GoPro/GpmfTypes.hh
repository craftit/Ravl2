//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/GPSCoordinate.hh"
#include <cereal/cereal.hpp>
#include <vector>

namespace Ravl2::GoPro
{
  //! GPS fix data from GoPro GPMF stream
  //! Uses RAVL2's GPSCoordinate which provides:
  //! - GRS84 ellipsoid calculations
  //! - Cartesian coordinate conversion
  //! - Distance calculations (great circle)
  //! - Error bounds tracking
  struct GpsFix
  {
    GPSCoordinate location;   //!< GPS position with latitude, longitude, height
    Point<float, 2> speed{0, 0}; //!< 2D and 3D speed in m/s [speed2d, speed3d]
    int fix = 0;              //!< GPS fix type (0=no lock, 2=2D, 3=3D)
    int satellites = 0;       //!< Number of satellites
    float precision = 0;      //!< Dilution of precision (DOP)

    //! Default constructor
    GpsFix() = default;

    //! Construct with all parameters
    GpsFix(const GPSCoordinate& loc, const Point<float, 2>& spd, int fixType, int sats, float dop)
      : location(loc), speed(spd), fix(fixType), satellites(sats), precision(dop)
    {}

    //! Convenience accessors
    [[nodiscard]] double latitude() const { return location.latitude(); }
    [[nodiscard]] double longitude() const { return location.longitude(); }
    [[nodiscard]] double height() const { return location.height(); }
    [[nodiscard]] float speed2d() const { return speed[0]; }
    [[nodiscard]] float speed3d() const { return speed[1]; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(location, speed, fix, satellites, precision);
    }
  };

  //! Collection of gyroscope samples with sample rate metadata
  //! High-frequency sensors like gyro typically have multiple samples per frame
  //! to match the GPMF packet structure (typically 200+ samples at 200 Hz)
  //! Each sample is a Vector3f representing angular velocity in rad/s [x, y, z]
  struct GyroSamples
  {
    std::vector<Vector3f> samples;  //!< Angular velocity samples in rad/s
    float sampleRate = 0.0F;        //!< Samples per second (0 indicates invalid/unset)

    //! Default constructor
    GyroSamples() = default;

    //! Construct with samples and sample rate
    GyroSamples(const std::vector<Vector3f>& s, float rate)
      : samples(s), sampleRate(rate)
    {
      if (rate <= 0.0F) {
        sampleRate = 0.0F;
      }
    }

    //! Get number of samples
    [[nodiscard]] size_t size() const { return samples.size(); }
    [[nodiscard]] bool empty() const { return samples.empty(); }

    //! Direct access to samples vector
    [[nodiscard]] const std::vector<Vector3f>& data() const { return samples; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(samples, sampleRate);
    }
  };

  //! Collection of accelerometer samples with sample rate metadata
  //! High-frequency sensors like accelerometer typically have multiple samples per frame
  //! to match the GPMF packet structure (typically 200+ samples at 200 Hz)
  //! Each sample is a Vector3f representing acceleration in m/s² [x, y, z]
  struct AccelSamples
  {
    std::vector<Vector3f> samples;  //!< Acceleration samples in m/s²
    float sampleRate = 0.0F;        //!< Samples per second (0 indicates invalid/unset)

    //! Default constructor
    AccelSamples() = default;

    //! Construct with samples and sample rate
    AccelSamples(const std::vector<Vector3f>& s, float rate)
      : samples(s), sampleRate(rate)
    {
      if (rate <= 0.0F) {
        sampleRate = 0.0F;
      }
    }

    //! Get number of samples
    [[nodiscard]] size_t size() const { return samples.size(); }
    [[nodiscard]] bool empty() const { return samples.empty(); }

    //! Direct access to samples vector
    [[nodiscard]] const std::vector<Vector3f>& data() const { return samples; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(samples, sampleRate);
    }
  };

  //! Collection of GPS fix samples with sample rate metadata
  //! Low-frequency sensor (1-18 Hz typically) that may have multiple fixes per GPMF packet
  //! Each sample is a complete GpsFix with location, speed, and quality metrics
  struct GpsSamples
  {
    std::vector<GpsFix> samples;    //!< GPS fix samples
    float sampleRate = 0.0F;        //!< Samples per second (0 indicates invalid/unset)

    //! Default constructor
    GpsSamples() = default;

    //! Construct with samples and sample rate
    GpsSamples(const std::vector<GpsFix>& s, float rate)
      : samples(s), sampleRate(rate)
    {
      if (rate <= 0.0F) {
        sampleRate = 0.0F;
      }
    }

    //! Get number of samples
    [[nodiscard]] size_t size() const { return samples.size(); }
    [[nodiscard]] bool empty() const { return samples.empty(); }

    //! Direct access to samples vector
    [[nodiscard]] const std::vector<GpsFix>& data() const { return samples; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(samples, sampleRate);
    }
  };


} // namespace Ravl2::GoPro
