// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Math/LinearAlgebra.hh"
#include "Ravl2/Geometry/ScaleTranslate.hh"

namespace Ravl2
{
  template <typename DataT, unsigned N>
  class ScaleTranslate;

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
    inline constexpr Affine(const Matrix<DataT, N, N> &SR, const Vector<DataT, N> &T) noexcept
      : mSR(SR),
        mT(T)
    {}

    //! Construct from a scale and a translation vector.
    inline constexpr Affine(const ScaleTranslate<DataT, N> &st) noexcept
      : mSR(Eigen::DiagonalWrapper(st.scaleVector())),
        mT(st.translation())
    {}
    
    //! Get the identity transformation.
    [[nodiscard]] inline static constexpr Affine identity()
    { return Affine(Matrix<DataT, N, N>::Identity(), Vector<DataT, N>::Zero()); }

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
    [[nodiscard]] constexpr std::optional<Affine<DataT, N> > inverse() const;

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
    [[nodiscard]] constexpr PointT operator()(PointT pnt) const
    {
      return mSR * pnt + mT;
    }

    //! Compose this transform with 'In'
    [[nodiscard]] inline constexpr auto operator()(const Affine &in) const
    {
      return Affine(mSR * in.SRMatrix(), mSR * in.Translation() + mT);
    }

    //! @brief Divide this transform by 'in'
    [[nodiscard]] inline constexpr auto divideBy(const Affine &in) const
    {
      Matrix<DataT, N, N> invSr = *Ravl2::inverse(in.SRMatrix());
      return Affine(mSR * invSr, invSr * (mT - in.Translation()));
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("SR", mSR), cereal::make_nvp("T", mT));
    }

  private:
    Matrix<DataT, N, N> mSR = Matrix<DataT, N, N>::Identity();//!< Scale/rotate.
    Vector<DataT, N> mT = Vector<DataT,N>::Zero();//!< Translate.
  };

  /////////////////////////////////////////////////


  template <typename DataT, unsigned N>
  void constexpr Affine<DataT, N>::scale(const Vector<DataT, N> &xy)
  {
    mSR = mSR * Eigen::DiagonalWrapper(xy);
  }

  template <typename DataT, unsigned N>
  constexpr void Affine<DataT, N>::translate(const Vector<DataT, N> &T)
  {
    mT += T;
  }

  template <typename DataT, unsigned N>
  constexpr std::optional<Affine<DataT, N> > Affine<DataT, N>::inverse() const
  {
    Affine<DataT, N> ret;
    auto inv = Ravl2::inverse(mSR);
    if(!inv.has_value()) {
      return std::nullopt;
    }
    ret.mSR = inv.value();
    ret.mT = ret.mSR * mT * -1;
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
    return Eigen::isfinite(mSR.array()).all() && Eigen::isfinite(mT.array()).all();
  }

  template <typename DataT>
  inline constexpr Affine<DataT, 2> affineFromScaleAngleTranslation(const Vector<DataT, 2> &scale, DataT angle, const Vector<DataT, 2> &translation)
  {
    Matrix<DataT, 2, 2> SR( {{std::cos(angle) * scale[1], -std::sin(angle) * scale[0]},
                              {std::sin(angle) * scale[1], std::cos(angle) * scale[0]}});
    return Affine<DataT, 2>(SR, translation);
  }

  //! @brief Computer inverse of affine transformation.
  template <typename DataT, unsigned N>
  constexpr std::optional<Affine<DataT, N> > inverse(const Affine<DataT, N> &aff)
  {
    return aff.inverse();
  }

  //! @brief Convert ScaleTranslate to Affine.
  template <typename DataT, unsigned N>
  Affine<DataT, N> toAffine(ScaleTranslate<DataT, N> const &st)
  {
    return Affine<DataT, N>(Eigen::DiagonalWrapper(st.scaleVector()), st.translation());
  }

  //! @brief Compose transforms
  template <typename DataT, unsigned N>
  Affine<DataT, N> operator*(const Affine<DataT, N> &lhs, const ScaleTranslate<DataT, N> &rhs)
  {
    return lhs(toAffine(rhs));
  }

  //! @brief Compose transforms
  template <typename DataT, unsigned N>
  Affine<DataT, N> operator*(const ScaleTranslate<DataT, N> &lhs, const Affine<DataT, N> &rhs)
  {
    return toAffine(lhs)(rhs);
  }


  //! @brief Send to output stream.
  template <typename DataT, unsigned N>
  inline std::ostream &operator<<(std::ostream &os, const Affine<DataT, N> &aff)
  {
    os << "Affine(SR=" << Eigen::WithFormat(aff.SRMatrix(), Ravl2::defaultEigenFormat()) << ",T=" << Eigen::WithFormat(aff.Translation(), Ravl2::defaultEigenFormat()) << ")";
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
