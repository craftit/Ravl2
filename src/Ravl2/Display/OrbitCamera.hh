#pragma once

#include <numbers>
#include "Ravl2/Types.hh"

namespace Ravl2::DebugDisplay
{

  //! Orbit camera for 3D viewport.
  //! @details Arcball/orbit-style camera around a target point. Maintains yaw/pitch in radians and distance.
  //! @threadsafe No. GUI thread only.
  struct OrbitCamera {
    // Target point the camera orbits around (world space)
    Eigen::Vector3f target = Eigen::Vector3f(0.f, 0.f, 0.f);
    // Yaw (rotation around Y axis), radians
    float yaw = 0.30f;// ~17 degrees default to avoid edge-on grid
    // Pitch (rotation around X axis), radians
    float pitch = -0.35f;// ~-20 degrees for initial downward view
    // Distance from target (> 0)
    float distance = 3.0f;

    // Projection parameters
    float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;// radians
    float nearZ = 0.05f;
    float farZ = 1000.0f;
    float aspect = 1.0f;

    // Limits
    float minDistance = 0.05f;
    float maxDistance = 10000.0f;
    float minPitch = -1.55f;// ~ -89 deg
    float maxPitch = 1.55f; // ~  89 deg

    //! Apply orbit deltas (radians). Clamps pitch.
    void orbit(float deltaYaw, float deltaPitch) noexcept;

    //! Pan in camera local X/Y plane (world units)
    void pan(float dx, float dy) noexcept;

    //! Dolly (change distance). Positive delta moves closer (zoom in)
    void dolly(float delta) noexcept;

    //! Build right-handed view matrix (world to view)
    Eigen::Matrix4f buildViewMatrix() const noexcept;

    //! Build perspective projection matrix (right-handed, depth [0,1] for bgfx)
    Eigen::Matrix4f buildProjMatrix() const noexcept;

    //! Reset to defaults keeping aspect
    void reset() noexcept;

    // --- Math helpers (private static)
    static Eigen::Matrix3f rotX(float a) noexcept;
    static Eigen::Matrix3f rotY(float a) noexcept;
    static Eigen::Matrix4f lookAtRH(const Eigen::Vector3f &eye,
                                    const Eigen::Vector3f &center,
                                    const Eigen::Vector3f &up) noexcept;
    static Eigen::Matrix4f perspectiveRH_ZO(float fovy, float aspect, float zNear, float zFar) noexcept;
  };

}// namespace Ravl2::DebugDisplay
