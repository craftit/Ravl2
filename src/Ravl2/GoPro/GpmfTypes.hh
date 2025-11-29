//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/GPSCoordinate.hh"
#include "Ravl2/Geometry/Quaternion.hh"
#include <cereal/cereal.hpp>

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
