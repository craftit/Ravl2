//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/GPSCoordinate.hh"
#include "Ravl2/Geometry/Quaternion.hh"
#include <cereal/cereal.hpp>
#include <span>
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

  //! 3-axis gyroscope reading (angular velocity)
  //! Using Vector3f for easy integration with rotation calculations
  struct GyroSample
  {
    Vector3f angularVelocity{0, 0, 0}; //!< Angular velocity in rad/s [x, y, z]

    //! Default constructor
    GyroSample() = default;

    //! Construct from vector
    explicit GyroSample(const Vector3f& av) : angularVelocity(av) {}

    //! Convenience accessors
    [[nodiscard]] float x() const { return angularVelocity[0]; }
    [[nodiscard]] float y() const { return angularVelocity[1]; }
    [[nodiscard]] float z() const { return angularVelocity[2]; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(angularVelocity);
    }
  };

  //! Collection of gyroscope samples with sample rate metadata
  //! High-frequency sensors like gyro typically have multiple samples per frame
  //! to match the GPMF packet structure (typically 200+ samples at 200 Hz)
  struct GyroSamples
  {
    std::vector<GyroSample> samples;  //!< Vector of gyroscope samples
    float sampleRate = 0.0F;          //!< Samples per second (0 indicates invalid/unset)

    //! Default constructor
    GyroSamples() = default;

    //! Construct with samples and sample rate
    GyroSamples(const std::vector<GyroSample>& s, float rate)
      : samples(s), sampleRate(rate)
    {
      if (rate <= 0.0F) {
        sampleRate = 0.0F;
      }
    }

    //! Get number of samples
    [[nodiscard]] size_t size() const { return samples.size(); }
    [[nodiscard]] bool empty() const { return samples.empty(); }

    //! Access samples
    [[nodiscard]] const std::vector<GyroSample>& data() const { return samples; }

    //! Convenience: Get raw Vector3f values for processing
    //! Returns a span over the internal data (zero-copy)
    [[nodiscard]] std::span<const Vector3f> asVectorSpan() const
    {
      // Safe because GyroSample is a trivial wrapper around Vector3f
      // and both have standard layout
      return std::span<const Vector3f>(
        reinterpret_cast<const Vector3f*>(samples.data()),
        samples.size()
      );
    }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(samples, sampleRate);
    }
  };

  //! 3-axis accelerometer reading (linear acceleration)
  //! Using Vector3f for easy integration with physics calculations
  struct AccelSample
  {
    Vector3f acceleration{0, 0, 0};    //!< Acceleration in m/s² [x, y, z]

    //! Default constructor
    AccelSample() = default;

    //! Construct from vector
    explicit AccelSample(const Vector3f& acc) : acceleration(acc) {}

    //! Convenience accessors
    [[nodiscard]] float x() const { return acceleration[0]; }
    [[nodiscard]] float y() const { return acceleration[1]; }
    [[nodiscard]] float z() const { return acceleration[2]; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(acceleration);
    }
  };

  //! Collection of accelerometer samples with sample rate metadata
  //! High-frequency sensors like accelerometer typically have multiple samples per frame
  //! to match the GPMF packet structure (typically 200+ samples at 200 Hz)
  struct AccelSamples
  {
    std::vector<AccelSample> samples;  //!< Vector of accelerometer samples
    float sampleRate = 0.0F;           //!< Samples per second (0 indicates invalid/unset)

    //! Default constructor
    AccelSamples() = default;

    //! Construct with samples and sample rate
    AccelSamples(const std::vector<AccelSample>& s, float rate)
      : samples(s), sampleRate(rate)
    {
      if (rate <= 0.0F) {
        sampleRate = 0.0F;
      }
    }

    //! Get number of samples
    [[nodiscard]] size_t size() const { return samples.size(); }
    [[nodiscard]] bool empty() const { return samples.empty(); }

    //! Access samples
    [[nodiscard]] const std::vector<AccelSample>& data() const { return samples; }

    //! Convenience: Get raw Vector3f values for processing
    //! Returns a span over the internal data (zero-copy)
    [[nodiscard]] std::span<const Vector3f> asVectorSpan() const
    {
      // Safe because AccelSample is a trivial wrapper around Vector3f
      // and both have standard layout
      return std::span<const Vector3f>(
        reinterpret_cast<const Vector3f*>(samples.data()),
        samples.size()
      );
    }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(samples, sampleRate);
    }
  };

  //! Temperature reading
  struct TemperatureSample
  {
    float celsius = 0;        //!< Temperature in Celsius

    //! Default constructor
    TemperatureSample() = default;

    //! Construct from value
    explicit TemperatureSample(float c) : celsius(c) {}

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(celsius);
    }
  };

  //! Magnetometer reading (magnetic field strength)
  //! Using Vector3f for easy integration with orientation calculations
  struct MagnetometerSample
  {
    Vector3f magneticField{0, 0, 0};   //!< Magnetic field in μT [x, y, z]

    //! Default constructor
    MagnetometerSample() = default;

    //! Construct from vector
    explicit MagnetometerSample(const Vector3f& field) : magneticField(field) {}

    //! Convenience accessors
    [[nodiscard]] float x() const { return magneticField[0]; }
    [[nodiscard]] float y() const { return magneticField[1]; }
    [[nodiscard]] float z() const { return magneticField[2]; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(magneticField);
    }
  };

  //! Camera orientation (derived from accelerometer/gyro/magnetometer fusion)
  //! Using RAVL2 Quaternion for rotation representation
  struct OrientationSample
  {
    Quaternion<float> orientation = Quaternion<float>::identity(); //!< Camera orientation as quaternion
    Vector3f eulerAngles{0, 0, 0};          //!< Euler angles (roll, pitch, yaw) in radians

    //! Default constructor
    OrientationSample() = default;

    //! Construct from quaternion
    explicit OrientationSample(const Quaternion<float>& q)
      : orientation(q)
    {
      // Convert quaternion to Euler angles
      eulerAngles = q.eulerAngles();
    }

    //! Convenience accessors
    [[nodiscard]] float roll() const { return eulerAngles[0]; }
    [[nodiscard]] float pitch() const { return eulerAngles[1]; }
    [[nodiscard]] float yaw() const { return eulerAngles[2]; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(orientation, eulerAngles);
    }
  };

} // namespace Ravl2::GoPro
