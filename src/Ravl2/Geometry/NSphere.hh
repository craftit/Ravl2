//
// Created by charles galambos on 08/02/2025.
//

#pragma once

#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2 {

  template<typename RealT, IndexSizeT N>
  class NSphere
  {
  public:
    NSphere() = default;

    //! Construct a sphere with a given center and radius.
    NSphere(const Point<RealT, N> &center, RealT radius)
      : mCentre(center),
        mRadius(radius)
    {}

    //! Access the center of the sphere.
    [[nodiscard]] const Point<RealT, N> &center() const
    {
      return mCentre;
    }

    //! Access the radius of the sphere.
    [[nodiscard]] RealT radius() const
    {
      return mRadius;
    }

    //! Set the center of the sphere.
    void setCentre(const Point<RealT, N> &center)
    {
      mCentre = center;
    }

    //! Set the radius of the sphere.
    void setRadius(RealT radius)
    {
      mRadius = radius;
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("centre", mCentre), cereal::make_nvp("radius", mRadius));
    }

    //! Test if a point is inside the sphere.
    //! @param point - The point to test.
    //! @return True if the point is inside the sphere, false otherwise.
    [[nodiscard]] bool contains(const Point<RealT, N> &point) const
    {
      return squaredEuclidDistance(mCentre, point) < sqr(mRadius);
    }

    //! Test if this sphere completely contains another sphere.
    //! @param other - The other sphere.
    //! @return True if this sphere contains the other sphere, false otherwise.
    [[nodiscard]] bool contains(const NSphere<RealT, N> &other) const
    {
      return (squaredEuclidDistance(mCentre, other.mCentre) + sqr(other.mRadius)) < sqr(mRadius);
    }
  protected:
    Point<RealT, N> mCentre {};
    RealT mRadius = 0;
  };

  template<typename RealT, IndexSizeT N>
  std::ostream &operator<<(std::ostream &os, const NSphere<RealT,N> &sphere)
  {
    os << "C:" << Eigen::WithFormat(sphere.center(),defaultEigenFormat()) << " R:" << sphere.radius();
    return os;
  }

  // Let everyone know there's an implementation already generated for common cases
  extern template class NSphere<float,2>;
  extern template class NSphere<float,3>;


} // Ravl2

#if FMT_VERSION >= 90000
template <typename RealT, Ravl2::IndexSizeT N>
struct fmt::formatter<Ravl2::NSphere<RealT,N>> : fmt::ostream_formatter {
};
#endif

