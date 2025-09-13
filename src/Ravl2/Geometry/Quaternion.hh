//
// Created by charles galambos on 22/10/2023.
//

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

  //! @brief Quaternion representation of a rotation in 3d space

  template <typename RealT>
  class Quaternion
  {
  public:
    using value_type = RealT;
    constexpr static unsigned dimension = 3;

    constexpr void normalise()
    {
      // Calculate the magnitude
      auto const mag = RealT(m_vec.norm());
      // If the magnitude is non-zero, then normalise the quaternion
      assert(!isNearZero(mag));
      m_vec /= mag;
    }

    //! Create a zero rotation quaternion
    constexpr Quaternion() = default;

    //! Get an identity quaternion
    [[nodiscard]]
    static constexpr Quaternion identity()
    {
      return Quaternion(1.0, 0, 0, 0);
    }

    //! Check quaternion is normalised
    [[nodiscard]]
    constexpr bool isNormalised() const
    {
      auto const mag = RealT(m_vec.norm());
      return isNearZero(mag - RealT(1.0), RealT(1e-6));
    }

    explicit constexpr Quaternion(const Vector<RealT,4> &vec)
        : m_vec(vec)
    {}

    constexpr Quaternion(float w, const Vector3f &vec)
    {
      m_vec[0] = w;
      m_vec[1] = vec[0];
      m_vec[2] = vec[1];
      m_vec[3] = vec[2];
    }

    //! Create a quaternion from components
    template <typename Data1T, typename Data2T, typename Data3T, typename Data4T>
    constexpr Quaternion(const Data1T &w, const Data2T &x, const Data3T &y, const Data4T &z)
        : m_vec({RealT(w), RealT(x), RealT(y), RealT(z)})
    {}

    //! Create a quaternion from an axis angle representation
    //! @param angle The angle of rotation in radians
    //! @param axis The axis of rotation
    [[nodiscard]] static Quaternion fromAngleAxis(RealT angle, const Vector<RealT,3> &axis)
    {
      auto s = std::sin(angle / 2);
      Vector<RealT,4> vec {std::cos(angle / 2), axis[0] * s, axis[1] * s, axis[2] * s};
      vec /= vec.norm();
      return Quaternion(vec);
    }

    //! Create a quaternion from a rotation matrix
    [[nodiscard]] static Quaternion fromMatrix(const Matrix<RealT, 3, 3> &m)
    {
      RealT trace = m(0, 0) + m(1, 1) + m(2, 2);
      Vector<RealT,4> q;
      if(trace > 0) {
	RealT s = 0.5f / std::sqrt(trace + 1.0f);
	q[0] = 0.25f / s;
	q[1] = (m(2, 1) - m(1, 2)) * s;
	q[2] = (m(0, 2) - m(2, 0)) * s;
	q[3] = (m(1, 0) - m(0, 1)) * s;
      } else {
	if(m(0, 0) > m(1, 1) && m(0, 0) > m(2, 2)) {
	  RealT s = 2.0f * std::sqrt(1.0f + m(0, 0) - m(1, 1) - m(2, 2));
	  q[0] = (m(2, 1) - m(1, 2)) / s;
	  q[1] = 0.25f * s;
	  q[2] = (m(0, 1) + m(1, 0)) / s;
	  q[3] = (m(0, 2) + m(2, 0)) / s;
	} else if(m(1, 1) > m(2, 2)) {
	  RealT s = 2.0f * std::sqrt(1.0f + m(1, 1) - m(0, 0) - m(2, 2));
	  q[0] = (m(0, 2) - m(2, 0)) / s;
	  q[1] = (m(0, 1) + m(1, 0)) / s;
	  q[2] = 0.25f * s;
	  q[3] = (m(1, 2) + m(2, 1)) / s;
	} else {
	  RealT s = 2.0f * std::sqrt(1.0f + m(2, 2) - m(0, 0) - m(1, 1));
	  q[0] = (m(1, 0) - m(0, 1)) / s;
	  q[1] = (m(0, 2) + m(2, 0)) / s;
	  q[2] = (m(1, 2) + m(2, 1)) / s;
	  q[3] = 0.25f * s;
	}
      }
      return Quaternion(q);
    }

    [[nodiscard]] constexpr Vector<RealT,4> &asVector()
    {
      return m_vec;
    }

    //! Get the vector representation
    [[nodiscard]] constexpr const Vector<RealT,4> &asVector() const
    {
      return m_vec;
    }

    //! Rotate a vector
    //! Maybe at a different precision from the quaternion type.
    template <typename Real2T>
    [[nodiscard]] constexpr Vector<Real2T, 3> rotate(const Vector<Real2T, 3> &v) const
    {
      assert(isNormalised());
      Vector<Real2T, 3> const xyz({Real2T(m_vec[1]), Real2T(m_vec[2]), Real2T(m_vec[3])});
      Vector<Real2T, 3> const t = 2.0f * cross(xyz, v);
      return v + m_vec[0] * t + cross(xyz, t);
    }

    //! @brief Transform Vector
    [[nodiscard]] constexpr auto operator()(const Vector<RealT, 3> &pnt) const
    {
      return rotate(pnt);
    }

    //! Compute the inverse rotation
    [[nodiscard]] constexpr Quaternion<RealT> inverse() const
    {
      auto const norm2v = sqr(m_vec[0]) + sqr(m_vec[1]) + sqr(m_vec[2]) + sqr(m_vec[3]);
      return {m_vec[0] / norm2v, -m_vec[1] / norm2v, -m_vec[2] / norm2v, -m_vec[3] / norm2v};
    }

    //! Compute the rotation matrix
    [[nodiscard]] constexpr Matrix<RealT, 3, 3> toMatrix() const
    {
      Matrix<RealT, 3, 3> m({{m_vec[0] * m_vec[0] + m_vec[1] * m_vec[1] - m_vec[2] * m_vec[2] - m_vec[3] * m_vec[3],
                              2.0f * (m_vec[1] * m_vec[2] - m_vec[0] * m_vec[3]),
                              2.0f * (m_vec[1] * m_vec[3] + m_vec[0] * m_vec[2])},
                             {2.0f * (m_vec[1] * m_vec[2] + m_vec[0] * m_vec[3]),
                              m_vec[0] * m_vec[0] - m_vec[1] * m_vec[1] + m_vec[2] * m_vec[2] - m_vec[3] * m_vec[3],
                              2.0f * (m_vec[2] * m_vec[3] - m_vec[0] * m_vec[1])},
                             {2.0f * (m_vec[1] * m_vec[3] - m_vec[0] * m_vec[2]),
                              2.0f * (m_vec[2] * m_vec[3] + m_vec[0] * m_vec[1]),
                              m_vec[0] * m_vec[0] - m_vec[1] * m_vec[1] - m_vec[2] * m_vec[2] + m_vec[3] * m_vec[3]}});
      return m;
    }

    //! Generate a projective matrix.
    [[nodiscard]] constexpr Matrix<RealT, 4, 4> projectiveMatrix() const
    {
      Matrix<RealT, 4, 4> ret;
      ret.template block<3, 3>(0, 0) = toMatrix();
      ret.template block<3, 1>(0, 3) = Vector<RealT, 3>::Zero();
      ret.template block<1, 3>(3, 0) = Vector<RealT, 3>::Zero();
      ret(3, 3) = 1;
      return ret;
    }

    [[nodiscard]] constexpr RealT w() const
    {
      return m_vec[0];
    }

    [[nodiscard]] constexpr RealT x() const
    {
      return m_vec[1];
    }

    [[nodiscard]] constexpr RealT y() const
    {
      return m_vec[2];
    }

    [[nodiscard]] constexpr RealT z() const
    {
      return m_vec[3];
    }

    //! Compute the rotation angle from an axis angle representation
    [[nodiscard]] constexpr RealT angle() const
    {
      auto norm = RealT(m_vec.norm());
      return std::atan2(std::sqrt(sqr(m_vec[1] / norm) + sqr(m_vec[2] / norm) + sqr(m_vec[3] / norm)), m_vec[0] / norm);
    }

    //! Compute a cost that is zero when the quaternion is the identity.
    //! This assumes the quaternion is normalised.
    [[nodiscard]] constexpr RealT zeroCost() const
    {
      auto lx = m_vec[1];
      auto ly = m_vec[2];
      auto lz = m_vec[3];
      return sqr(lx) + sqr(ly) + sqr(lz);
    }

    //! Compute the XYZ euler angles from the quaternion
    [[nodiscard]] constexpr Vector<RealT, 3> eulerAngles() const
    {
      Matrix<RealT, 3, 3> rot = toMatrix();
      Vector<RealT, 3> res;
      const int i = 0;
      const int j = 1;
      const int k = 2;
      {
        res[0] = std::atan2(rot(j, k), rot(k, k));
        auto c2 = std::hypot(rot(i, i), rot(i, j));// Was norm(..)
        res[1] = std::atan2(-rot(i, k), c2);
        auto s1 = std::sin(res[0]);
        auto c1 = std::cos(res[0]);
        res[2] = std::atan2(s1 * rot(k, i) - c1 * rot(j, i), c1 * rot(j, j) - s1 * rot(k, j));
      }
      res = -res;
      return res;
    }

    //! From XYZ euler angles
    template <typename Real2T>
    [[nodiscard]] static constexpr Quaternion<RealT> fromEulerAnglesXYZ(const Vector<Real2T, 3> &angles)
    {
      // We should simplify this out.
      auto ret = fromAngleAxis(static_cast<RealT>(angles[0]), {1.0f, 0.0f, 0.0f}) *
                 fromAngleAxis(static_cast<RealT>(angles[1]), {0.0f, 1.0f, 0.0f}) *
                 fromAngleAxis(static_cast<RealT>(angles[2]), {0.0f, 0.0f, 1.0f});
      ret.normalise();
      return ret;
    }

    //! From XYZ euler angles
    template <typename Real2T>
    [[nodiscard]] static constexpr Quaternion<RealT> fromEulerAnglesXYZ(Real2T angleX, Real2T angleY, Real2T angleZ)
    {
      return fromEulerAnglesXYZ(Vector<Real2T, 3>{angleX, angleY, angleZ});
    }

    //! Check quaternion is real
    [[nodiscard]]
    constexpr bool isReal() const
    {
      return Eigen::isfinite(m_vec.array()).all();
    }

    //! Serialize the quaternion
    template <class Archive>
    void serialize(Archive &archive)
    {
      archive(m_vec[0], m_vec[1], m_vec[2], m_vec[3]);
    }

  private:
    Vector<RealT, 4> m_vec {1.0, 0, 0, 0};
  };

  //! Multiply two quaternions
  template <typename RealT>
  constexpr Quaternion<RealT> operator*(const Quaternion<RealT> &qt1, const Quaternion<RealT> &qt2)
  {
    auto const &q = qt1.asVector();
    auto const &q2 = qt2.asVector();
    return {
      q[0] * q2[0] - q[1] * q2[1] - q[2] * q2[2] - q[3] * q2[3],
      q[0] * q2[1] + q[1] * q2[0] + q[2] * q2[3] - q[3] * q2[2],
      q[0] * q2[2] + q[2] * q2[0] + q[3] * q2[1] - q[1] * q2[3],
      q[0] * q2[3] + q[3] * q2[0] + q[1] * q2[2] - q[2] * q2[1]};
  }

  //! @brief Spherical linear interpolation between two quaternions
  //! @param p1 First quaternion, given at t=0
  //! @param p2 Second quaternion, given at t=1
  //! @param t The interpolation parameter
  //! @return The interpolated quaternion
  template <typename RealT,typename ScaleT>
  [[nodiscard]]
  constexpr Quaternion<RealT> slerp(const Quaternion<RealT> &p1, const Quaternion<RealT> &p2, ScaleT t)
  {
    const RealT it = static_cast<RealT>(t);
    const RealT one = RealT(1.0) - std::numeric_limits<RealT>::epsilon();
    RealT d = p1.asVector().dot(p2.asVector());
    RealT absD = std::fabs(d);

    RealT scale0;
    RealT scale1;

    if(absD >= one) {
      scale0 = RealT(1) - it;
      scale1 = it;
    } else {
      // theta is the angle between the 2 quaternions
      RealT theta = std::acos(absD);
      RealT sinTheta = std::sin(theta);

      scale0 = std::sin((RealT(1) - it) * theta) / sinTheta;
      scale1 = std::sin((it * theta)) / sinTheta;
    }
    if(d < RealT(0)) scale1 = -scale1;

    return Quaternion<RealT>(scale0 * p1.asVector() + scale1 * p2.asVector());
  }

  //! Add two quaternions
  template <typename RealT>
  inline constexpr Quaternion<RealT> operator+(const Quaternion<RealT> &qt1, const Quaternion<RealT> &qt2)
  {
    return Quaternion(qt1.asVector() + qt2.asVector());
  }

  //! Rotate a vector by a quaternion
  template <typename RealT, typename Real2T>
  inline constexpr auto operator*(const Quaternion<RealT> &qt1, const Vector<Real2T, 3> &qt2)
  {
    return qt1.rotate(qt2);
  }

  template <typename RealT>
  inline constexpr Quaternion<RealT> operator*(const Quaternion<RealT> &qt1, const RealT &s2)
  {
    return Quaternion(qt1.asVector() * s2);
  }

  template <typename RealT>
  inline constexpr Quaternion<RealT> operator*(RealT s1, const Quaternion<RealT> &qt2)
  {
    return Quaternion(s1 * qt2.asVector());
  }

  template <typename RealT>
  std::ostream &operator<<(std::ostream &strm, const Quaternion<RealT> &v)
  {
    strm << v.w() << " " << v.x() << " " << v.y() << " " << v.z();
    return strm;
  }

  // Apply an angular velocity to an orientation
  template <typename RealT, typename Real2T>
  [[nodiscard]] constexpr Quaternion<RealT> ApplyAngularVelocity(const Quaternion<RealT> &orientation, const Vector<Real2T, 3> &angularVelocity, Real2T duration)
  {
    auto rot = Quaternion<RealT>::fromEulerAnglesXYZ((angularVelocity * duration).eval());
    return orientation * rot;//add to the starting rotation
  }

  template <typename RealT>
  constexpr std::string toString(const Quaternion<RealT> &val)
  {
    auto angles = val.eulerAngles();
    return fmt::format("Q: {} {} {} deg.", Ravl2::rad2deg(angles[0]), Ravl2::rad2deg(angles[1]), Ravl2::rad2deg(angles[2]));
  }

  extern template class Quaternion<float>;

}// namespace Ravl2

namespace fmt
{
  template <typename DataT>
  struct formatter<Ravl2::Quaternion<DataT>> : ostream_formatter {
  };
}// namespace fmt
