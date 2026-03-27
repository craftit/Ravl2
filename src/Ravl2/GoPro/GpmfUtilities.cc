//
// Created for RAVL2 GoPro metadata support
//

#include "Ravl2/GoPro/GpmfUtilities.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2::GoPro
{
  std::vector<Vector3f> gpsTrackToLocal(const std::vector<GpsFix>& track)
  {
    if (track.empty()) {
      return {};
    }

    // Use first point as origin
    return gpsTrackToLocal(track, track[0].location);
  }

  std::vector<Vector3f> gpsTrackToLocal(
    const std::vector<GpsFix>& track,
    const GPSCoordinate& origin)
  {
    std::vector<Vector3f> localPoints;
    localPoints.reserve(track.size());

    // Get origin in Cartesian coordinates
    Point<double, 3> originCart = origin.cartesian();

    for (const auto& fix : track) {
      // Convert to Cartesian
      Point<double, 3> pointCart = fix.location.cartesian();

      // Compute offset from origin
      Vector<double, 3> offset = pointCart - originCart;

      // Convert to float Vector3f
      localPoints.emplace_back(
        static_cast<float>(offset[0]),
        static_cast<float>(offset[1]),
        static_cast<float>(offset[2])
      );
    }

    return localPoints;
  }

  std::vector<Quaternion<float>> integrateGyroToOrientation(
    const GyroSamples& gyroData,
    const Quaternion<float>& initialOrientation)
  {
    std::vector<Quaternion<float>> orientations;
    orientations.reserve(gyroData.samples.size());

    if (gyroData.samples.empty() || gyroData.sampleRate <= 0) {
      return orientations;
    }

    float deltaTime = 1.0f / gyroData.sampleRate;
    Quaternion<float> currentOrientation = initialOrientation;

    for (const auto& angularVelocity : gyroData.samples) {
      // Create a quaternion from angular velocity
      // For small rotations: q ≈ [1, ω*dt/2]
      Vector3f angularVelocityDt = angularVelocity * (deltaTime * 0.5f);
      float angle = angularVelocityDt.norm();

      Quaternion<float> deltaQ;
      if (angle > 1e-6f) {
        // Proper quaternion from axis-angle
        Vector3f axis = angularVelocityDt / angle;
        deltaQ = Quaternion<float>::fromAngleAxis(2.0f * angle, axis);
      } else {
        // Small angle approximation: q ≈ [1, ω*dt/2]
        // Even for very small rotations, we should apply the linearized update
        // rather than treating them as zero rotation
        deltaQ = Quaternion<float>(1.0f, angularVelocityDt[0], angularVelocityDt[1], angularVelocityDt[2]);
        deltaQ.normalise();
      }

      // Integrate
      currentOrientation = currentOrientation * deltaQ;
      currentOrientation.normalise();

      orientations.push_back(currentOrientation);
    }

    return orientations;
  }

  Vector3f removeGravity(
    const Vector3f& acceleration,
    const Quaternion<float>& orientation)
  {
    // Gravity vector in world frame (pointing down)
    Vector3f gravityWorld(0, 0, -9.81f);

    // Transform gravity to sensor frame
    Vector3f gravitySensor = orientation.inverse()(gravityWorld);

    // Remove gravity
    return acceleration - gravitySensor;
  }

} // namespace Ravl2::GoPro
