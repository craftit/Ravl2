// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  //! General affine transformation.

  template <typename DataT, unsigned N>
  class Affine
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimension = N;
    using PointT = Point<DataT, N>;

    //! Construct an identity transform.
    inline constexpr Affine() = default;

    //! Copy constructor.
    inline constexpr Affine(const Affine &Oth) noexcept = default;

    //! Construct from Scale/Rotate matrix and a translation vector.
    inline constexpr Affine(const Matrix<DataT, N, N> &SR, const Vector<DataT, N> &T) noexcept;

    //! Access the translation component of the transformation.
    [[nodiscard]] inline constexpr Vector<DataT, N> &Translation() { return mT; }

    //! Constant access to the translation component of the transformation.
    [[nodiscard]] inline constexpr const Vector<DataT, N> &Translation() const { return mT; }

    //! In place Scaling along the X & Y axis by value given in the vector.
    //! If all values 1, then no effect.
    inline constexpr void scale(const Vector<DataT, N> &xy);

    //! Add a translation in direction T.
    inline constexpr void translate(const Vector<DataT, N> &T);

    //! Generate an inverse transformation.
    [[nodiscard]] constexpr Affine<DataT, N> inverse() const;

    //! Get Scale/Rotate matrix.
    [[nodiscard]] constexpr Matrix<DataT, N, N> &SRMatrix() { return mSR; }

    //! Get Scale/Rotate matrix.
    [[nodiscard]] constexpr const Matrix<DataT, N, N> &SRMatrix() const { return mSR; }

    //! Assignment.
    inline constexpr Affine<DataT, N> &operator=(const Affine &oth) = default;

    //! Check all components of transform are real.
    [[nodiscard]] constexpr bool isReal() const;

    //! Transform Vector,  scale, Rotate, translate.
    // Take a vector and put it though the transformation.
    [[nodiscard]] constexpr auto operator()(const PointT &pnt) const
    {
      return PointT(xt::linalg::dot(mSR, pnt) + mT);
    }

    //! Compose this transform with 'In'
    [[nodiscard]] inline constexpr auto operator()(const Affine &in) const
    {
      return Affine(xt::linalg::dot(mSR, in.SRMatrix()), xt::linalg::dot(mSR, in.Translation()) + mT);
    }

    //! @brief Divide this transform by 'in'
    [[nodiscard]] inline constexpr auto divideBy(const Affine &in) const
    {
      Matrix<DataT, N, N> invSr = xt::linalg::inv(in.SRMatrix());
      return Affine(mSR * invSr, invSr * (mT - in.Translation()));
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("SR", mSR), cereal::make_nvp("T", mT));
    }

  protected:
    Matrix<DataT, N, N> mSR = xt::eye<DataT>(N);//!< Scale/rotate.
    Vector<DataT, N> mT = xt::zeros<DataT>({N});//!< Translate.
  };

  /////////////////////////////////////////////////

  template <typename DataT, unsigned N>
  inline constexpr Affine<DataT, N>::Affine(const Matrix<DataT, N, N> &SR, const Vector<DataT, N> &T) noexcept
      : mSR(SR),
        mT(T)
  {}

  template <typename DataT, unsigned N>
  void constexpr Affine<DataT, N>::scale(const Vector<DataT, N> &xy)
  {
    mSR = xt::linalg::dot(mSR, xt::diag(xy));
  }

  template <typename DataT, unsigned N>
  inline constexpr void Affine<DataT, N>::translate(const Vector<DataT, N> &T)
  {
    mT += T;
  }

  template <typename DataT, unsigned N>
  constexpr Affine<DataT, N> Affine<DataT, N>::inverse(void) const
  {
    Affine<DataT, N> ret;
    ret.mSR = xt::linalg::inv(mSR);
    ret.mT = xt::linalg::dot(ret.mSR, mT) * -1;
    return ret;
  }

  template <typename DataT, unsigned N>
  constexpr Affine<DataT, N> operator*(const Affine<DataT, N> &lhs, const Affine<DataT, N> &rhs)
  {
    //return Affine(mSR * lhs.SRMatrix(), mSR * lhs.Translation() + mT);
    return lhs(rhs);
  }

  template <typename DataT, unsigned N>
  constexpr Affine<DataT, N> operator/(const Affine<DataT, N> &lhs, const Affine<DataT, N> &rhs)
  {
    return lhs.divideBy(rhs);
  }

  template <typename DataT, unsigned N>
  constexpr bool Affine<DataT, N>::isReal() const
  {
    for(auto x : mSR) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    for(auto x : mT) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    return true;
  }

  template <typename DataT>
  inline constexpr Affine<DataT, 2> affineFromScaleAngleTranslation(const Vector<DataT, 2> &scale, DataT angle, const Vector<DataT, 2> &translation)
  {
    Matrix<DataT, 2, 2> SR = {{std::cos(angle) * scale[1], -std::sin(angle) * scale[0]},
                              {std::sin(angle) * scale[1], std::cos(angle) * scale[0]}};
    return Affine<DataT, 2>(SR, translation);
  }

  //! @brief Computer inverse of affine transformation.
  template <typename DataT, unsigned N>
  constexpr Affine<DataT, N> inverse(const Affine<DataT, N> &aff)
  {
    return aff.inverse();
  }

  //! @brief Send to output stream.
  template <typename DataT, unsigned N>
  inline std::ostream &operator<<(std::ostream &os, const Affine<DataT, N> &aff)
  {
    os << "Affine(" << aff.SRMatrix() << "," << aff.Translation() << ")";
    return os;
  }

  extern template class Affine<float, 2>;
  extern template class Affine<float, 3>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT, unsigned N>
struct fmt::formatter<Ravl2::Affine<RealT, N>> : fmt::ostream_formatter {
};
#endif
