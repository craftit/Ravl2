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

  template<typename DataT, unsigned N>
  class ScaleTranslate
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimension = N;

    //! Construct no-change transform.
    inline ScaleTranslate()
    {
      mT = xt::zeros<DataT>({N});
      mS = xt::ones<DataT>({N});
    }


    //! Copy constructor.
    inline ScaleTranslate(const ScaleTranslate &Oth) = default;

    //! Construct from scale and a translation vector.
    inline ScaleTranslate(const Vector<DataT,N> &scale, const Vector<DataT,N> &translate);

    inline Vector<DataT,N> &translation() { return mT; }
    //: Access the translation component of the transformation.

    inline const Vector<DataT,N> &translation() const { return mT; }
    //: Constant access to the translation component of the transformation.

    inline void scale(const Vector<DataT, N> &xy);
    //: In place Scaling along the X & Y axis by value given in the vector.
    // If all values 1, then no effect.

    inline void translate(const Vector<DataT, N> &T);
    //: Add a translation in direction T.

    inline Vector<DataT,N> operator*(const Vector<DataT,N> &In) const;
    //: Transform Vector,  Scale, Rotate, Translate.
    // Take a vector and put it though the transformation.

    inline ScaleTranslate<DataT,N> operator*(const ScaleTranslate &In) const;
    //: Compose this transform with 'In'

    inline ScaleTranslate<DataT,N> operator/(const ScaleTranslate &In) const;
    //: 'In' / 'Out' = this;

    ScaleTranslate<DataT,N> inverse() const;
    //: Generate an inverse transformation.

    Vector<DataT,N> &scaleVector() { return mS; }
    //: Get Scale/Rotate matrix.

    const Vector<DataT,N> &scaleVector() const { return mS; }
    //: Get Scale/Rotate matrix.

    inline ScaleTranslate<DataT,N> &operator=(const ScaleTranslate &Oth);
    //: Assignment.

    //! Check all components of transform are real.
    [[nodiscard]] bool isReal() const;

    //! Transform Vector,  scale, Rotate, translate.
    // Take a vector and put it though the transformation.
    auto operator()(const Vector<DataT,N> &pnt) const
    {
      return mS * pnt + mT;
    }

    //: Compose this transform with 'In'
    inline auto operator()(const ScaleTranslate &In) const
    {
      return ScaleTranslate(mS * In.scaleVector(), mS * In.translation() + mT);
    }


  protected:
    Vector<DataT,N> mS; // Scale/rotate.
    Vector<DataT,N> mT;   // Translate.
  };

  /////////////////////////////////////////////////

  template<typename DataT,unsigned N>
  inline ScaleTranslate<DataT,N>::ScaleTranslate(const Vector<DataT,N> &SR, const Vector<DataT,N> &T)
    : mS(SR),
      mT(T)
  {}

  template<typename DataT,unsigned N>
  void ScaleTranslate<DataT,N>::scale(const Vector<DataT, N> &xy)
  { mS = xy; }

  template<typename DataT,unsigned N>
  inline void ScaleTranslate<DataT,N>::translate(const Vector<DataT, N> &T) {
    mT += T;
  }

  template<typename DataT,unsigned N>
  ScaleTranslate<DataT,N> ScaleTranslate<DataT,N>::inverse(void) const {
    ScaleTranslate<DataT,N> ret;
    ret.mS = DataT(1)/mS;
    ret.mT = ret.mS * mT;
    ret.mT *= -1;
    return ret;
  }

  template<typename DataT,unsigned N>
  Vector<DataT,N> ScaleTranslate<DataT,N>::operator*(const Vector<DataT,N> &in) const {
    return (mS * in) + mT;
  }

  template<typename DataT,unsigned N>
  ScaleTranslate<DataT,N> ScaleTranslate<DataT,N>::operator*(const ScaleTranslate<DataT,N> &In) const{
    return ScaleTranslate(mS * In.scaleVector(), mS * In.translation() + mT);
  }

  template<typename DataT,unsigned N>
  ScaleTranslate<DataT,N> ScaleTranslate<DataT,N>::operator/(const ScaleTranslate<DataT,N> &in) const{
    Vector<DataT,N> inverse = DataT(1)/in.scaleVector();
    return ScaleTranslate(mS * inverse, inverse * (mT - in.translation()));
  }

  template<typename DataT,unsigned N>
  inline ScaleTranslate<DataT,N> &ScaleTranslate<DataT,N>::operator=(const ScaleTranslate<DataT,N> &Oth) {
    mS = Oth.mS;
    mT = Oth.mT;
    return *this;
  }

  template<typename DataT,unsigned N>
  bool ScaleTranslate<DataT,N>::isReal() const
  {
    for(auto x : mS) {
      if(std::isinf(x) || std::isnan(x))
	return false;
    }
    for(auto x : mT) {
      if (std::isinf(x) || std::isnan(x))
	return false;
    }
    return true;
  }

  extern template class ScaleTranslate<float,2>;
  extern template class ScaleTranslate<float,3>;


}

