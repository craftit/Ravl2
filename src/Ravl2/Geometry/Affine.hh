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
  
  template<typename DataT, size_t N>
  class Affine
  {
  public:
    inline Affine();
    //: Construct no-change transform.
    
    inline Affine(const Affine &Oth);
    //: Copy constructor.
    
    inline Affine(const Matrix<DataT,N,N> &SR, const Vector<DataT,N> &T);
    //: Construct from Scale/Rotate matrix and a translation vector.
    
    inline Vector<DataT,N> &Translation() { return T; }
    //: Access the translation component of the transformation.
    
    inline const Vector<DataT,N> &Translation() const { return T; }
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
    
    Matrix<DataT,N,N> &SRMatrix() { return SR; }
    //: Get Scale/Rotate matrix.
    
    const Matrix<DataT,N,N> &SRMatrix() const { return SR; }
    //: Get Scale/Rotate matrix.
    
    inline const Affine<DataT,N> &operator=(const Affine &Oth);
    //: Assignment.
    
    bool IsReal() const;
    //: Check all components of transform are real.

  protected:
    Matrix<DataT,N,N> SR; // Scale/rotate.
    Vector<DataT,N> T;   // Translate.
    
  };

  /////////////////////////////////////////////////
  
  template<typename DataT,size_t N>
  inline Affine<DataT,N>::Affine()
    : SR()
  {
    T.Fill(0);
    SR.Fill(0.0);
    for(size_t i = 0;i < N;i++)
      SR[i][i] = 1.0;
  }
  
  template<typename DataT,size_t N>
  inline Affine<DataT,N>::Affine(const Affine &Oth)
    : SR(Oth.SR),
      T(Oth.T)
  {}
  
  template<typename DataT,size_t N>
  inline Affine<DataT,N>::Affine(const Matrix<DataT,N,N> &nSR, const Vector<DataT,N> &nT)
    : SR(nSR),
      T(nT)
  {}
  
  template<typename DataT,size_t N>
  void Affine<DataT,N>::Scale(const Vector<DataT,N> &xy) {
    for(size_t i = 0;i < N;i++)
      for(size_t j = 0;j < N;j++)
	SR[i][j] *= xy[j];
  }
  
  template<typename DataT,size_t N>
  inline void Affine<DataT,N>::Translate(const Vector<DataT,N> &DT) {
    T += DT;
  }
  
  template<typename DataT,size_t N>
  Affine<DataT,N> Affine<DataT,N>::Inverse(void) const {
    Affine<DataT,N> ret;
    ret.SR = SR.Inverse();
    Mul(ret.SR,T,ret.T);
    ret.T *= -1;
    return ret;
  }
  
  template<typename DataT,size_t N>
  Vector<DataT,N> Affine<DataT,N>::operator*(const Vector<DataT,N> &In) const {
    return (SR * In) + T;
  }
  
  template<typename DataT,size_t N>
  Affine<DataT,N> Affine<DataT,N>::operator*(const Affine &In) const{
    return Affine(SR * In.SRMatrix(), SR * In.Translation() + T);
  }
  
  template<typename DataT,size_t N>
  Affine<DataT,N> Affine<DataT,N>::operator/(const Affine &In) const{
    Matrix<DataT,N,N> inverse = In.SRMatrix().Inverse();
    return Affine(SR * inverse, inverse * (T - In.Translation()));
  }
  
  template<typename DataT,size_t N>
  inline const Affine<DataT,N> &Affine<DataT,N>::operator=(const Affine &Oth) {
    SR = Oth.SR;
    T = Oth.T;
    return *this;
  }
  
  template<typename DataT,size_t N>
  bool Affine<DataT,N>::IsReal() const
  {
    for(auto x : SR) {
      if(std::isinf(x) || std::isnan(x))
        return false;
    }
    for(auto x : T) {
      if (std::isinf(x) || std::isnan(x))
        return false;
    }
    return true;
  }

}

