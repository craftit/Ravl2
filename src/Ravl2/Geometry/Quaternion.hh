//
// Created by charles galambos on 22/10/2023.
//

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2 {

	//! @brief Quaternion representation of a rotation in 3d space

        template<typename RealT>
	class Quaternion
	{
	public:
		void normalise()
		{
			// Calculate the magnitude
			auto const mag = float(xt::norm_l2(m_vec)());
			// If the magnitude is non-zero, then normalise the quaternion
			assert(!isNearZero(mag));
			m_vec /= mag;
		}

		//! Create a zero rotation quaternion
		Quaternion() = default;

		//! Get an identity quaternion
		[[nodiscard]]
		static Quaternion identity()
		{ return Quaternion(1.0,0,0,0); }

		//! Check quaternion is normalised
		[[nodiscard]]
		bool isNormalised() const
		{
			auto const mag = float(xt::norm_l2(m_vec)());
			return isNearZero(mag - 1.0f,1e-6f);
		}

		explicit Quaternion(const Vector4f &vec)
				: m_vec(vec)
		{}

		Quaternion(float w,const Vector3f &vec)
		{
			m_vec[0] = w;
			m_vec[1] = vec[0];
			m_vec[2] = vec[1];
			m_vec[3] = vec[2];
		}

		//! Create a quaternion from components
		template<typename Data1T,typename Data2T,typename Data3T,typename Data4T>
		Quaternion(const Data1T &w,const Data2T &x,const Data3T &y,const Data4T &z)
		 : m_vec({RealT(w),RealT(x),RealT(y),RealT(z)})
		{}

		//! Create a quaternion from an axis angle representation
		//! @param angle The angle of rotation in radians
		//! @param axis The axis of rotation
		[[nodiscard]] static Quaternion fromAngleAxis(RealT angle, const Vector3f &axis)
		{
			auto s = std::sin(angle / 2);
			Vector4f vec {std::cos(angle / 2),axis[0] * s,axis[1] * s,axis[2] * s};
			vec /= float(xt::norm_l2(vec)());
			return Quaternion(vec);
		}

		[[nodiscard]] Vector4f &asVector()
		{ return m_vec; }

		//! Get the vector representation
		[[nodiscard]] const Vector4f &asVector() const
		{ return m_vec; }

		//! Rotate vector
		//! Maybe a different precision from the quaternion type.
		template<typename Real2T>
		[[nodiscard]] Vector<Real2T,3> rotate(const Vector<Real2T,3> &v) const	{
                  assert(isNormalised());
                  Vector<Real2T,3> const xyz({Real2T(m_vec[1]),Real2T(m_vec[2]),Real2T(m_vec[3])});
                  Vector<Real2T,3> const t = 2.0f * xt::linalg::cross(xyz, v);
                  return v + m_vec[0] * t + xt::linalg::cross(xyz, t);
                }

		//! Compute the inverse rotation
		[[nodiscard]] Quaternion<RealT> inverse() const
                {
                  auto const norm2v = sqr(m_vec[0]) + sqr(m_vec[1])+ sqr(m_vec[2])+ sqr(m_vec[3]);
                  return {m_vec[0]/norm2v, -m_vec[1]/norm2v, -m_vec[2]/norm2v, -m_vec[3]/norm2v};
                }

		//! Compute the rotation matrix
		[[nodiscard]] Matrix<RealT,3,3> rotationMatrix() const
                {
                  Matrix<RealT,3,3> m({
                    {
                      m_vec[0] * m_vec[0] + m_vec[1] * m_vec[1] - m_vec[2] * m_vec[2] - m_vec[3] * m_vec[3],
                      2.0f * (m_vec[1] * m_vec[2] - m_vec[0] * m_vec[3]),
                      2.0f * (m_vec[1] * m_vec[3] + m_vec[0] * m_vec[2])
                    },
                    {
                      2.0f * (m_vec[1] * m_vec[2] + m_vec[0] * m_vec[3]),
                      m_vec[0] * m_vec[0] - m_vec[1] * m_vec[1] + m_vec[2] * m_vec[2] - m_vec[3] * m_vec[3],
                      2.0f * (m_vec[2] * m_vec[3] - m_vec[0] * m_vec[1])
                    },
                    {
                      2.0f * (m_vec[1] * m_vec[3] - m_vec[0] * m_vec[2]),
                      2.0f * (m_vec[2] * m_vec[3] + m_vec[0] * m_vec[1]),
                      m_vec[0] * m_vec[0] - m_vec[1] * m_vec[1] - m_vec[2] * m_vec[2] + m_vec[3] * m_vec[3]
                    }});
                  return m;
                }

		[[nodiscard]] RealT w() const
		{ return m_vec[0]; }

		[[nodiscard]] RealT x() const
		{ return m_vec[1]; }

		[[nodiscard]] RealT y() const
		{ return m_vec[2]; }

		[[nodiscard]] RealT z() const
		{ return m_vec[3]; }

		//! Compute the rotation angle from an axis angle representation
		[[nodiscard]] RealT angle() const
		{
			float norm = float(xt::norm_l2(m_vec)());
			return std::atan2(std::sqrt(sqr(m_vec[1]/norm) + sqr(m_vec[2]/norm) + sqr(m_vec[3]/norm)),m_vec[0]/norm);
		}

		// This assumes the quaternion is normalised.
		[[nodiscard]] RealT zeroCost() const
		{
			auto lx = m_vec[1];
			auto ly = m_vec[2];
			auto lz = m_vec[3];
			return sqr(lx) + sqr(ly) + sqr(lz);
		}

		//! Compute the XYZ euler angles from the quaternion
		[[nodiscard]] Vector<RealT,3> eulerAngles() const
                {
                  Matrix<RealT,3,3> rot = rotationMatrix();
                  Vector<RealT,3> res;
                  const int i = 0;
                  const int j = 1;
                  const int k = 2;
                  {
                    res[0] = std::atan2(rot(j,k), rot(k,k));
                    auto c2 = std::hypot(rot(i,i), rot(i,j)); // Was norm(..)
                    res[1] = std::atan2(-rot(i,k), c2);
                    auto s1 = std::sin(res[0]);
                    auto c1 = std::cos(res[0]);
                    res[2] = std::atan2(s1*rot(k,i)-c1*rot(j,i), c1*rot(j,j) - s1 * rot(k,j));
                  }
                  res = -res;
                  return res;
                }


                //! From XYZ euler angles
		[[nodiscard]] static Quaternion fromEulerAngles(const Vector3f &angles)
                {
                  // We should simplify this out.
                  auto ret = fromAngleAxis(angles[0],{1.0f, 0.0f, 0.0f}) *
                    fromAngleAxis(angles[1],{0.0f, 1.0f, 0.0f}) *
                    fromAngleAxis(angles[2],{0.0f, 0.0f, 1.0f});
                  ret.normalise();
                  return ret;
                }


		//! Spherical linear interpolation between two quaternions
		//! @param t The interpolation parameter
		//! @param other The other quaternion
		//! @return The interpolated quaternion
		[[nodiscard]]
		Quaternion slerp(float t,const Quaternion &other) const
                {
                  const RealT one = RealT(1.0) - std::numeric_limits<RealT>::epsilon();
                  RealT d = xt::linalg::dot(m_vec, other.m_vec)();
                  RealT absD = std::fabs(d);

                  RealT scale0;
                  RealT scale1;

                  if(absD >= one) {
                    scale0 = RealT(1) - t;
                    scale1 = t;
                  } else {
                    // theta is the angle between the 2 quaternions
                    RealT theta = std::acos(absD);
                    RealT sinTheta = std::sin(theta);

                    scale0 = std::sin((RealT(1) - t) * theta) / sinTheta;
                    scale1 = std::sin((t * theta)) / sinTheta;
                  }
                  if(d < RealT(0)) scale1 = -scale1;

                  return Quaternion(scale0 * m_vec + scale1 * other.m_vec);
                }

          private:
            Vector<RealT,4> m_vec {1.0,0,0,0} ;
	};

	//! Multiply two quaternions
        template<typename RealT>
	Quaternion<RealT> operator*(const Quaternion<RealT> &qt1,const Quaternion<RealT> &qt2)
        {
          auto const &q = qt1.asVector();
          auto const &q2 = qt2.asVector();
          return {
            q[0]*q2[0] - q[1]*q2[1] - q[2]*q2[2] - q[3]*q2[3],
            q[0]*q2[1] + q[1]*q2[0] + q[2]*q2[3] - q[3]*q2[2],
            q[0]*q2[2] + q[2]*q2[0] + q[3]*q2[1] - q[1]*q2[3],
            q[0]*q2[3] + q[3]*q2[0] + q[1]*q2[2] - q[2]*q2[1]
          };
        }


	//! Add two quaternions
        template<typename RealT>
	inline Quaternion<RealT> operator+(const Quaternion<RealT> &qt1,const Quaternion<RealT> &qt2)
	{ return Quaternion(qt1.asVector() + qt2.asVector()); }

	//! Rotate a vector by a quaternion
        template<typename RealT,typename Real2T>
	inline auto operator*(const Quaternion<RealT> &qt1,const Vector<Real2T,3> &qt2)
	{ return qt1.rotate(qt2); }

        template<typename RealT>
	inline Quaternion<RealT> operator*(const Quaternion<RealT> &qt1,const RealT &s2)
	{ return Quaternion(qt1.asVector() * s2); }

        template<typename RealT>
	inline Quaternion<RealT> operator*(RealT s1,const Quaternion<RealT> &qt2)
	{ return Quaternion(s1 * qt2.asVector());}

        template<typename RealT>
        std::ostream &operator<<(std::ostream &strm,const Quaternion<RealT> &v)
        {
          strm << v.w() << " " << v.x() << " " << v.y()  << " " << v.z();
          return strm;
        }

	// Apply an angular velocity to an orientation
        template<typename RealT,typename Real2T>
	[[nodiscard]] Quaternion<RealT> ApplyAngularVelocity(const Quaternion<RealT> &orientation,const Vector<Real2T,3> &angularVelocity,Real2T duration)
        {
          auto rot = Quaternion<RealT>::fromEulerAngles(angularVelocity * duration);
          return orientation * rot;  //add to the starting rotation
        }

        template<typename RealT>
	std::string toString(const Quaternion<RealT> &val)
        {
          auto angles = val.eulerAngles();
          return fmt::format("Q: {} {} {}",angles[0],angles[1],angles[2]);
        }

}