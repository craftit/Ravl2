//
// Created for RAVL2 GoPro metadata support
//

#pragma once

#include "Ravl2/GoPro/GpmfTypes.hh"
#include "Ravl2/Geometry/GPSCoordinate.hh"
#include "Ravl2/Geometry/Quaternion.hh"
#include <vector>

namespace Ravl2::GoPro
{
  //! Convert GPS track to local coordinate system for visualization
  //! Returns points as Vector3f for easy use with 3D rendering
  //! Uses the first point as origin
  std::vector<Vector3f> gpsTrackToLocal(const std::vector<GpsFix>& track);

  //! Convert GPS track to local coordinate system with specified origin
  std::vector<Vector3f> gpsTrackToLocal(
    const std::vector<GpsFix>& track,
    const GPSCoordinate& origin);

  //! Calculate distance between two GPS coordinates (meters)
  //! Uses GPSCoordinate's built-in great circle distance calculation
  //! which accounts for Earth's ellipsoid shape (GRS84)
  inline double gpsDistance(
    const GPSCoordinate& gps1,
    const GPSCoordinate& gps2)
  {
    // GPSCoordinate provides cartesian() method for accurate distance
    return euclidDistance(gps1.cartesian(), gps2.cartesian());
  }

  //! Integrate gyroscope data to estimate orientation changes
  //! Returns a vector of Quaternions representing orientation over time
  std::vector<Quaternion<float>> integrateGyroToOrientation(
    const std::vector<GyroSample>& gyroSamples,
    float sampleRate,
    const Quaternion<float>& initialOrientation = Quaternion<float>::identity());

  //! Remove gravity from accelerometer data using orientation
  //! Useful for extracting linear acceleration (motion) from total acceleration
  Vector3f removeGravity(
    const Vector3f& acceleration,
    const Quaternion<float>& orientation);

  //! Apply low-pass filter to vector data (for smoothing)
  template<typename VectorT>
  std::vector<VectorT> lowPassFilter(
    const std::vector<VectorT>& data,
    float alpha)
  {
    if (data.empty()) {
      return {};
    }

    std::vector<VectorT> filtered;
    filtered.reserve(data.size());

    filtered.push_back(data[0]);
    for (size_t i = 1; i < data.size(); i++) {
      filtered.push_back(alpha * data[i] + (1.0f - alpha) * filtered[i - 1]);
    }

    return filtered;
  }

} // namespace Ravl2::GoPro
