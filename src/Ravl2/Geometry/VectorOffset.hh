//
// Created by charles on 20/08/24.
//

#pragma once

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Math/LeastSquares.hh"

namespace Ravl2
{

  //! @brief Vector and offset. This defines a line in 2D space, and a plane in 3D space.

  template <typename RealT, size_t N>
  class VectorOffset
  {
  public:
    //! Default constructor.
    VectorOffset() = default;

    //! Constructor from a vector and an offset.
    VectorOffset(const Vector<RealT, N> &norm, RealT p)
        : mNormal(norm), mD(p)
    {}

    //! Constructor from a vector and a point on the plane/line.
    VectorOffset(const Vector<RealT, N> &norm, const Point<RealT, N> &p)
        : mNormal(norm),
          mD(-norm.dot(p))
    {}

    //! Returns the normal of the plane.
    [[nodiscard]] inline const Vector<RealT, N> &normal() const
    {
      return mNormal;
    }

    //! Returns the offset of the plane.
    [[nodiscard]] inline RealT offset() const
    {
      return (mD);
    }

    //! Returns the offset of the plane.
    [[nodiscard]] inline RealT d() const
    {
      return (mD);
    }

    //! Returns the value of the function dot(p,normal) + d often
    //! used in geometrical computations.
    [[nodiscard]] constexpr RealT residuum(const Point<RealT, N> &p) const
    {
      return mNormal.dot(p) + this->mD;
    }

    //! Returns the signed distance of the 'point' from the line.
    //! The return value is greater than 0 if the point is on the left
    //! side of the line. The left side of the line is determined
    //! by the direction of the normal.
    [[nodiscard]] constexpr RealT signedDistance(const Point<RealT, N> &point) const
    {
      return residuum(point) / mNormal.norm();
    }

    //! Returns the distance of the 'point' from this.
    [[nodiscard]] constexpr RealT distance(const Point<RealT, N> &point) const
    {
      return std::abs(signedDistance(point));
    }

    //! @brief Returns the point which is the orthogonal projection of the 'point' to the line.
    //! It is the same as intersection of this line with the perpendicular line passing through the 'point'.
    //! This is the closest point on the line to the 'point'.
    [[nodiscard]] constexpr Point<RealT, N> projection(const Point<RealT, N> &point) const
    {
      return point - mNormal * (residuum(point) / sumOfSqr(mNormal));
    }

    //! Normalizes the normal vector have a length of 1.
    inline auto &makeUnitNormal()
    {
      RealT mag = RealT(mNormal.norm());
      mNormal /= mag;
      mD /= mag;
      return (*this);
    }

    //! Flip the normal vector.
    inline auto &flipNormal()
    {
      mNormal = -mNormal;
      mD = -mD;
      return (*this);
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("normal", mNormal), cereal::make_nvp("d", mD));
    }

  protected:
    Vector<RealT, N> mNormal;
    RealT mD = 0;
  };

  extern template class VectorOffset<float, 2>;
  extern template class VectorOffset<float, 3>;
  
  template <typename RealT,size_t N>
  std::ostream &operator<<(std::ostream &outS, const VectorOffset<RealT,N> &plane)
  {
    outS << plane.normal() << ' ' << plane.offset();
    return (outS);
  }
  
  template <typename RealT,size_t N>
  std::istream &operator>>(std::istream &inS, VectorOffset<RealT,N> &plane)
  {
    Vector<RealT,N> norm;
    RealT d;
    inS >> norm >> d;
    plane = VectorOffset<RealT,N>(norm, d);
    return (inS);
  }
  
}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT, size_t N>
struct fmt::formatter<Ravl2::VectorOffset<RealT, N>> : fmt::ostream_formatter {
};
#endif
