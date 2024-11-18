// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{

  //! Scale and translation

  template <typename DataT, unsigned N>
  class ScaleTranslate
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimension = N;

    //! Construct no-change transform.
    //! Scale = 1, Translate = 0.
    inline constexpr ScaleTranslate() = default;

    //! Copy constructor.
    inline constexpr ScaleTranslate(const ScaleTranslate &Oth) = default;

    //! Construct from scale and a translation vector.
    inline constexpr ScaleTranslate(const Vector<DataT, N> &scale, const Vector<DataT, N> &translate);

    //! Access the translation component of the transformation.
    [[nodiscard]]inline constexpr Vector<DataT, N> &translation() { return mT; }

    //! Constant access to the translation component of the transformation.
    [[nodiscard]] inline constexpr const Vector<DataT, N> &translation() const { return mT; }

    //! In place Scaling along the X & Y axis by value given in the vector.
    // If all values 1, then no effect.
    inline constexpr void scale(const Vector<DataT, N> &xy);

    //! Add a translation in direction T.
    inline constexpr void translate(const Vector<DataT, N> &T);

    //! Generate an inverse transformation.
    [[nodiscard]] constexpr ScaleTranslate<DataT, N> inverse(void) const;

    //! Transform Vector,  Scale, Rotate, Translate.
    // Take a vector and put it though the transformation.
    [[nodiscard]] constexpr inline Vector<DataT, N> operator*(const Vector<DataT, N> &In) const;

    //! Compose this transform with 'In'
    [[nodiscard]] constexpr inline ScaleTranslate<DataT, N> operator*(const ScaleTranslate &in) const;

    //! Compose this transform with 'In's
    [[nodiscard]] constexpr inline Range<DataT,N> operator*(const Range<DataT,N> &in) const;

    //! Return 'In' / 'Out'
    [[nodiscard]] constexpr inline ScaleTranslate<DataT, N> operator/(const ScaleTranslate &in) const;

    //! Get Scale/Rotate matrix.
    [[nodiscard]] constexpr Vector<DataT, N> &scaleVector() { return mS; }

    //! Get Scale/Rotate matrix.
    [[nodiscard]] constexpr const Vector<DataT, N> &scaleVector() const { return mS; }

    //! Assignment.
    inline constexpr ScaleTranslate<DataT, N> &operator=(const ScaleTranslate &Oth);

    //! Check all components of transform are real.
    [[nodiscard]] constexpr bool isReal() const;

    //! Transform Vector,  scale, Rotate, translate.
    // Take a vector and put it though the transformation.
    [[nodiscard]] constexpr Point<DataT,N> operator()(const Point<DataT, N> &pnt) const
    {
      return mS * pnt + mT;
    }

    //! Compose this transform with 'In'
    [[nodiscard]] constexpr inline auto operator()(const ScaleTranslate &In) const
    {
      return ScaleTranslate(mS * In.scaleVector(), mS * In.translation() + mT);
    }

    //! Transform a range.
    [[nodiscard]] constexpr inline auto operator()(const Range<DataT,N> &In) const
    {
      return Range<DataT,N>(mS * In.min() + mT, mS * In.max() + mT);
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("S", mS), cereal::make_nvp("T", mT));
    }

  protected:
    Vector<DataT, N> mS = Vector<DataT, N>::Ones(); //!< Scale
    Vector<DataT, N> mT = Vector<DataT, N>::Zero();//!< Translate.
  };

  //! Generate an inverse transformation.
  template <typename DataT, unsigned N>
  [[nodiscard]] constexpr ScaleTranslate<DataT, N> inverse(const ScaleTranslate<DataT, N> &In)
  {
    return In.inverse();
  }

  /////////////////////////////////////////////////

  template <typename DataT, unsigned N>
  inline constexpr ScaleTranslate<DataT, N>::ScaleTranslate(const Vector<DataT, N> &SR, const Vector<DataT, N> &T)
      : mS(SR),
        mT(T)
  {}

  template <typename DataT, unsigned N>
  constexpr void ScaleTranslate<DataT, N>::scale(const Vector<DataT, N> &xy)
  {
    mS *= xy;
  }

  template <typename DataT, unsigned N>
  inline constexpr void ScaleTranslate<DataT, N>::translate(const Vector<DataT, N> &T)
  {
    mT += T;
  }

  template <typename DataT, unsigned N>
  ScaleTranslate<DataT, N> constexpr ScaleTranslate<DataT, N>::inverse() const
  {
    ScaleTranslate<DataT, N> ret;
    ret.mS = DataT(1) / mS;
    ret.mT = ret.mS * mT;
    ret.mT *= -1;
    return ret;
  }

  template <typename DataT, unsigned N>
  Vector<DataT, N> constexpr ScaleTranslate<DataT, N>::operator*(const Vector<DataT, N> &in) const
  {
    return (mS * in) + mT;
  }

  template <typename DataT, unsigned N>
  [[nodiscard]] constexpr ScaleTranslate<DataT, N> ScaleTranslate<DataT, N>::operator*(const ScaleTranslate<DataT, N> &In) const
  {
    return ScaleTranslate(mS * In.scaleVector(), mS * In.translation() + mT);
  }

  template <typename DataT, unsigned N>
  [[nodiscard]] constexpr Range<DataT, N> ScaleTranslate<DataT, N>::operator*(const Range<DataT, N> &In) const
  {
    return (*this)(In);
  }

  template <typename DataT, unsigned N>
  constexpr ScaleTranslate<DataT, N> ScaleTranslate<DataT, N>::operator/(const ScaleTranslate<DataT, N> &in) const
  {
    Vector<DataT, N> invScale = DataT(1) / in.scaleVector();
    return ScaleTranslate(mS * invScale, invScale * (mT - in.translation()));
  }

  template <typename DataT, unsigned N>
  inline constexpr ScaleTranslate<DataT, N> &ScaleTranslate<DataT, N>::operator=(const ScaleTranslate<DataT, N> &Oth)
  {
    mS = Oth.mS;
    mT = Oth.mT;
    return *this;
  }

  template <typename DataT, unsigned N>
  constexpr bool ScaleTranslate<DataT, N>::isReal() const
  {
    for(auto x : mS) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    for(auto x : mT) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    return true;
  }

  //! Fit a scale translate between ranges.
  template <typename DataT, unsigned N>
  bool fit(ScaleTranslate<DataT,N> &transform,const Range<DataT,N> &rngTo,const Range<DataT,N> &from) {
    bool ret = true;
    for(unsigned i = 0; i < N; i++) {
      if(rngTo.size()[i] == 0 || from.size()[i] == 0) {
        transform.scaleVector()[i] = 1;
        ret = false;
      } else {
        transform.scaleVector()[i] = rngTo.size()[i] / from.size()[i];
      }
      transform.translation()[i] = rngTo.min()[i] - from.min()[i] * transform.scaleVector()[i];
    }
    return ret;
  }

  //! Fit a scale to translate between index ranges.
  template <typename DataT, unsigned N>
  void fit(ScaleTranslate<DataT,N> &transform,const IndexRange<N> &rngTo,const IndexRange<N> &from) {
    for(unsigned i = 0; i < N; i++) {
      transform.scaleVector()[i] = DataT(rngTo.size()[i]) / DataT(from.size()[i]);
      transform.translation()[i] = DataT(rngTo.min()[i]) - DataT(from.min()[i]) * transform.scaleVector()[i];
    }
  }

  //! Stream output.
  template <typename DataT, unsigned N>
  std::ostream &operator<<(std::ostream &os, const ScaleTranslate<DataT, N> &In)
  {
    os << "Scale: " << In.scaleVector() << " Translate: " << In.translation();
    return os;
  }

  extern template class ScaleTranslate<float, 2>;
  extern template class ScaleTranslate<float, 3>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT, unsigned N>
struct fmt::formatter<Ravl2::ScaleTranslate<RealT, N>> : fmt::ostream_formatter {
};
#endif
