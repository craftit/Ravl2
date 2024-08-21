//
// Created by charles galambos on 21/08/2024.
//

#pragma once

#include "Ravl2/Geometry/Quaternion.hh"

namespace Ravl2
{

  template<typename RealT>
  class Isometry3
  {
  public:
    Isometry3() = default;

    Isometry3(const Quaternion<RealT> &rotation,const Vector<RealT,3> &translation)
        : m_rotation(rotation),
          m_translation(translation)
    {}

    //! Transform a point
    [[nodiscard]] Vector<RealT,3> transform(const Vector<RealT,3> &v) const
    { return m_rotation.rotate(v) + m_translation; }

    const Vector<RealT,3> &translation() const
    { return m_translation; }

    const Quaternion<RealT> &rotation() const
    { return m_rotation; }

    //! Add a translation
    void translate(const Vector<RealT,3> &v)
    { m_translation += v; }

    //! Rotate by a quaternion
    //! @param q The quaternion to rotate by
    void rotate(const Quaternion<RealT> &q)
    { m_rotation = q * m_rotation; }

    //! Compute the inverse
    [[nodiscard]] Isometry3<RealT> inverse() const
    {
      Quaternion inv = m_rotation.inverse();
      return Isometry3<RealT>(inv,inv.rotate(-m_translation));
    }

  private:
    Quaternion<RealT> m_rotation = Quaternion<RealT>::identity();
    Vector<RealT,3> m_translation = {0,0,0};
  };

  template<typename RealT>
  inline Vector<RealT,3> operator*(const Isometry3<RealT> &iso,const Vector<RealT,3> &v) {
    return iso.transform(v);
  }

  //! Create a translation isometry
  template<typename RealT>
  Isometry3<RealT> translation(Vector<RealT,3> translation) {
    return Isometry3<RealT>(Quaternion<RealT>::identity(),translation);
  }

  template<typename RealT>
  Isometry3<RealT> operator*(const Isometry3<RealT> &iso1,const Isometry3<RealT> &iso2)
  {
    return Isometry3<RealT>(
      iso1.rotation() * iso2.rotation(),
      iso1.rotation().rotate(iso2.translation()) + iso1.translation()
    );
  }

}
