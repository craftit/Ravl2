// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

  //! userlevel=Normal
  //: General affine transformation.
  
  template<typename DataT, unsigned N>
  class Affine
  {
  public:
    using ValueT = DataT;
    constexpr static unsigned dimension = N;

    inline Affine();
    //: Construct no-change transform.
    
    inline Affine(const Affine &Oth);
    //: Copy constructor.
    
    inline Affine(const Matrix<DataT,N,N> &SR, const Vector<DataT,N> &T);
    //: Construct from Scale/Rotate matrix and a translation vector.
    
    inline Vector<DataT,N> &Translation() { return mT; }
    //: Access the translation component of the transformation.
    
    inline const Vector<DataT,N> &Translation() const { return mT; }
    //: Constant access to the translation component of the transformation.
    
    inline void Scale(const Vector<DataT,N> &xy);
    //: In place Scaling along the X & Y axis by value given in the vector.
    // If all values 1, then no effect.
    
    inline void Translate(const Vector<DataT,N> &T);
    //: Add a translation in direction T.
    
    inline Vector<DataT,N> operator*(const Vector<DataT,N> &In) const;
    //: Transform Vector,  Scale, Rotate, Translate.
    // Take a vector and put it though the transformation.
    
    inline Affine<DataT,N> operator*(const Affine &In) const;
    //: Compose this transform with 'In'
    
    inline Affine<DataT,N> operator/(const Affine &In) const;
    //: 'In' / 'Out' = this;

    Affine<DataT,N> Inverse() const;
    //: Generate an inverse transformation.
    
    Matrix<DataT,N,N> &SRMatrix() { return mSR; }
    //: Get Scale/Rotate matrix.
    
    const Matrix<DataT,N,N> &SRMatrix() const { return mSR; }
    //: Get Scale/Rotate matrix.
    
    inline Affine<DataT,N> &operator=(const Affine &Oth);
    //: Assignment.

    //! Check all components of transform are real.
    [[nodiscard]] bool IsReal() const;

    //! Transform Vector,  Scale, Rotate, Translate.
    // Take a vector and put it though the transformation.
    auto operator()(const Vector<DataT,N> &In) const
    {
      return (mSR * In) + mT;
    }

    //: Compose this transform with 'In'
    inline auto operator()(const Affine &In) const
    {
      return Affine(mSR * In.SRMatrix(), mSR * In.Translation() + mT);
    }


  protected:
    Matrix<DataT,N,N> mSR; // Scale/rotate.
    Vector<DataT,N> mT;   // Translate.
  };

  /////////////////////////////////////////////////
  
  template<typename DataT,unsigned N>
  inline Affine<DataT,N>::Affine()
    : mSR()
  {
    mT.Fill(0);
    mSR.Fill(0.0);
    for(size_t i = 0;i < N;i++)
      mSR[i][i] = 1.0;
  }
  
  template<typename DataT,unsigned N>
  inline Affine<DataT,N>::Affine(const Affine &Oth)
    : mSR(Oth.mSR),
      mT(Oth.mT)
  {}
  
  template<typename DataT,unsigned N>
  inline Affine<DataT,N>::Affine(const Matrix<DataT,N,N> &SR, const Vector<DataT,N> &T)
    : mSR(SR),
      mT(T)
  {}
  
  template<typename DataT,unsigned N>
  void Affine<DataT,N>::Scale(const Vector<DataT,N> &xy) {
    for(size_t i = 0;i < N;i++)
      for(size_t j = 0;j < N;j++)
        mSR[i][j] *= xy[j];
  }
  
  template<typename DataT,unsigned N>
  inline void Affine<DataT,N>::Translate(const Vector<DataT,N> &T) {
    mT += T;
  }
  
  template<typename DataT,unsigned N>
  Affine<DataT,N> Affine<DataT,N>::Inverse(void) const {
    Affine<DataT,N> ret;
    ret.mSR = mSR.Inverse();
    Mul(ret.mSR, mT, ret.mT);
    ret.mT *= -1;
    return ret;
  }
  
  template<typename DataT,unsigned N>
  Vector<DataT,N> Affine<DataT,N>::operator*(const Vector<DataT,N> &In) const {
    return (mSR * In) + mT;
  }
  
  template<typename DataT,unsigned N>
  Affine<DataT,N> Affine<DataT,N>::operator*(const Affine &In) const{
    return Affine(mSR * In.SRMatrix(), mSR * In.Translation() + mT);
  }
  
  template<typename DataT,unsigned N>
  Affine<DataT,N> Affine<DataT,N>::operator/(const Affine &In) const{
    Matrix<DataT,N,N> inverse = In.SRMatrix().Inverse();
    return Affine(mSR * inverse, inverse * (mT - In.Translation()));
  }
  
  template<typename DataT,unsigned N>
  inline Affine<DataT,N> &Affine<DataT,N>::operator=(const Affine &Oth) {
    mSR = Oth.mSR;
    mT = Oth.mT;
    return *this;
  }
  
  template<typename DataT,unsigned N>
  bool Affine<DataT,N>::IsReal() const
  {
    for(auto x : mSR) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    for(auto x : mT) {
      if (std::isinf(x) || std::isnan(x))
        return false;
    }
    return true;
  }

}

