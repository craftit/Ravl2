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
  class ScaleTranslate
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimension = N;

    //! Construct no-change transform.
    //! Scale = 1, Translate = 0.
    inline ScaleTranslate() = default;

    //! Copy constructor.
    inline ScaleTranslate(const ScaleTranslate &Oth) = default;

    //! Construct from scale and a translation vector.
    inline ScaleTranslate(const Vector<DataT, N> &scale, const Vector<DataT, N> &translate);

    //! Access the translation component of the transformation.
    inline Vector<DataT, N> &translation() { return mT; }

    //! Constant access to the translation component of the transformation.
    inline const Vector<DataT, N> &translation() const { return mT; }

    //! In place Scaling along the X & Y axis by value given in the vector.
    // If all values 1, then no effect.
    inline void scale(const Vector<DataT, N> &xy);

    //! Add a translation in direction T.
    inline void translate(const Vector<DataT, N> &T);

    //! Generate an inverse transformation.
    [[nodiscard]] ScaleTranslate<DataT, N> inverse(void) const;

    //! Transform Vector,  Scale, Rotate, Translate.
    // Take a vector and put it though the transformation.
    [[nodiscard]] inline Vector<DataT, N> operator*(const Vector<DataT, N> &In) const;

    //! Compose this transform with 'In'
    [[nodiscard]] inline ScaleTranslate<DataT, N> operator*(const ScaleTranslate &in) const;

    //! Return 'In' / 'Out'
    [[nodiscard]] inline ScaleTranslate<DataT, N> operator/(const ScaleTranslate &in) const;

    //! Get Scale/Rotate matrix.
    [[nodiscard]] Vector<DataT, N> &scaleVector() { return mS; }

    //! Get Scale/Rotate matrix.
    [[nodiscard]] const Vector<DataT, N> &scaleVector() const { return mS; }

    //! Assignment.
    inline ScaleTranslate<DataT, N> &operator=(const ScaleTranslate &Oth);

    //! Check all components of transform are real.
    [[nodiscard]] bool isReal() const;

    //! Transform Vector,  scale, Rotate, translate.
    // Take a vector and put it though the transformation.
    [[nodiscard]] auto operator()(const Vector<DataT, N> &pnt) const
    {
      return mS * pnt + mT;
    }

    //! Compose this transform with 'In'
    [[nodiscard]] inline auto operator()(const ScaleTranslate &In) const
    {
      return ScaleTranslate(mS * In.scaleVector(), mS * In.translation() + mT);
    }

    //! Serialization support
    template <class Archive>
    void serialize( Archive & ar )
    {
      ar( cereal::make_nvp("S",mS), cereal::make_nvp("T",mT));
    }

  protected:
    Vector<DataT, N> mS = xt::ones<DataT>({N}); // Scale/rotate.
    Vector<DataT, N> mT = xt::zeros<DataT>({N});// Translate.
  };

  //! Generate an inverse transformation.
  template <typename DataT, unsigned N>
  ScaleTranslate<DataT, N> inverse(const ScaleTranslate<DataT, N> &In)
  {
    return In.inverse();
  }

  /////////////////////////////////////////////////

  template <typename DataT, unsigned N>
  inline ScaleTranslate<DataT, N>::ScaleTranslate(const Vector<DataT, N> &SR, const Vector<DataT, N> &T)
      : mS(SR),
        mT(T)
  {}

  template <typename DataT, unsigned N>
  void ScaleTranslate<DataT, N>::scale(const Vector<DataT, N> &xy)
  {
    mS = xy;
  }

  template <typename DataT, unsigned N>
  inline void ScaleTranslate<DataT, N>::translate(const Vector<DataT, N> &T)
  {
    mT += T;
  }

  template <typename DataT, unsigned N>
  ScaleTranslate<DataT, N> ScaleTranslate<DataT, N>::inverse(void) const
  {
    ScaleTranslate<DataT, N> ret;
    ret.mS = DataT(1) / mS;
    ret.mT = ret.mS * mT;
    ret.mT *= -1;
    return ret;
  }

  template <typename DataT, unsigned N>
  Vector<DataT, N> ScaleTranslate<DataT, N>::operator*(const Vector<DataT, N> &in) const
  {
    return (mS * in) + mT;
  }

  template <typename DataT, unsigned N>
  ScaleTranslate<DataT, N> ScaleTranslate<DataT, N>::operator*(const ScaleTranslate<DataT, N> &In) const
  {
    return ScaleTranslate(mS * In.scaleVector(), mS * In.translation() + mT);
  }

  template <typename DataT, unsigned N>
  ScaleTranslate<DataT, N> ScaleTranslate<DataT, N>::operator/(const ScaleTranslate<DataT, N> &in) const
  {
    Vector<DataT, N> invScale = DataT(1) / in.scaleVector();
    return ScaleTranslate(mS * invScale, invScale * (mT - in.translation()));
  }

  template <typename DataT, unsigned N>
  inline ScaleTranslate<DataT, N> &ScaleTranslate<DataT, N>::operator=(const ScaleTranslate<DataT, N> &Oth)
  {
    mS = Oth.mS;
    mT = Oth.mT;
    return *this;
  }

  template <typename DataT, unsigned N>
  bool ScaleTranslate<DataT, N>::isReal() const
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

  extern template class ScaleTranslate<float, 2>;
  extern template class ScaleTranslate<float, 3>;

}// namespace Ravl2
