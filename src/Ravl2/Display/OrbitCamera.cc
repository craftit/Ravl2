#include <cmath>
#include <numbers>
#include "Ravl2/Types.hh"

#include "Ravl2/Display/OrbitCamera.hh"

namespace Ravl2::DebugDisplay
{

  void OrbitCamera::orbit(float deltaYaw, float deltaPitch) noexcept
  {
    yaw += deltaYaw;
    pitch += deltaPitch;
    if(pitch < minPitch) pitch = minPitch;
    if(pitch > maxPitch) pitch = maxPitch;
  }

  void OrbitCamera::pan(float dx, float dy) noexcept
  {
    const Eigen::Matrix3f R = rotY(yaw) * rotX(pitch);
    const Eigen::Vector3f right = R.col(0);
    const Eigen::Vector3f up = R.col(1);
    target += (-dx) * right + (dy)*up;// screen convention: up is +dy
  }

  void OrbitCamera::dolly(float delta) noexcept
  {
    distance -= delta;
    if(distance < minDistance) distance = minDistance;
    if(distance > maxDistance) distance = maxDistance;
  }

  Eigen::Matrix4f OrbitCamera::buildViewMatrix() const noexcept
  {
    const Eigen::Matrix3f R = rotY(yaw) * rotX(pitch);
    const Eigen::Vector3f camDir = -(R.col(2));// forward is -Z in view space
    const Eigen::Vector3f eye = target - camDir * distance;
    return lookAtRH(eye, target, R.col(1));
  }

  Eigen::Matrix4f OrbitCamera::buildProjMatrix() const noexcept
  {
    return perspectiveRH_ZO(fovY, aspect, nearZ, farZ);
  }

  void OrbitCamera::reset() noexcept
  {
    target = Eigen::Vector3f(0.f, 0.f, 0.f);
    yaw = 0.f;
    pitch = 0.f;
    distance = 3.f;
    fovY = 50.0f * float(M_PI) / 180.0f;
    nearZ = 0.05f;
    farZ = 1000.0f;
  }

  Eigen::Matrix3f OrbitCamera::rotX(float a) noexcept
  {
    const float c = std::cos(a), s = std::sin(a);
    Eigen::Matrix3f m;
    m << 1, 0, 0, 0, c, -s, 0, s, c;
    return m;
  }

  Eigen::Matrix3f OrbitCamera::rotY(float a) noexcept
  {
    const float c = std::cos(a), s = std::sin(a);
    Eigen::Matrix3f m;
    m << c, 0, s, 0, 1, 0, -s, 0, c;
    return m;
  }

  Eigen::Matrix4f OrbitCamera::lookAtRH(const Eigen::Vector3f &eye,
                                        const Eigen::Vector3f &center,
                                        const Eigen::Vector3f &up) noexcept
  {
    const Eigen::Vector3f f = (center - eye).normalized();
    const Eigen::Vector3f s = f.cross(up).normalized();
    const Eigen::Vector3f u = s.cross(f);

    Eigen::Matrix4f m;
    m.setIdentity();
    m(0, 0) = s.x();
    m(0, 1) = s.y();
    m(0, 2) = s.z();
    m(0, 3) = -s.dot(eye);
    m(1, 0) = u.x();
    m(1, 1) = u.y();
    m(1, 2) = u.z();
    m(1, 3) = -u.dot(eye);
    m(2, 0) = -f.x();
    m(2, 1) = -f.y();
    m(2, 2) = -f.z();
    m(2, 3) = f.dot(eye);
    m(3, 0) = 0.0f;
    m(3, 1) = 0.0f;
    m(3, 2) = 0.0f;
    m(3, 3) = 1.0f;
    return m;
  }

  Eigen::Matrix4f OrbitCamera::perspectiveRH_ZO(float fovy, float aspect, float zNear, float zFar) noexcept
  {
    const float f = 1.0f / std::tan(fovy * 0.5f);
    Eigen::Matrix4f m;
    m.setZero();
    m(0, 0) = f / aspect;
    m(1, 1) = f;
    m(2, 2) = zFar / (zNear - zFar);
    m(2, 3) = (zFar * zNear) / (zNear - zFar);
    m(3, 2) = -1.0f;
    return m;
  }

}// namespace Ravl2::DebugDisplay
