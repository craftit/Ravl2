// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Affine2f.hh"



namespace RavlN {

#if 0
  Affine2dC FitAffine(const SArray1dC<Point2dC> &org,const SArray1dC<Point2dC> &newPos, const SArray1dC<RealT> &weights)
  {
    RealT residual;
    return FitAffine(org,newPos, weights, residual);
  }
  
  Affine2dC FitAffine(const SArray1dC<Point2dC> &org,
                      const SArray1dC<Point2dC> &newPos, 
                      const SArray1dC<RealT> &weights, 
                      RealT& residual
                      )
  {
    UIntT samples = org.Size();
    RavlAssertMsg(samples == newPos.Size(),"Affine2dC FitAffine(), Point arrays must have the same size.");
    RavlAssertMsg(samples == weights.Size(),"Affine2dC FitAffine(), Weight array must have the same size as points.");
    
    RealT meanWeight = weights.Sum() / static_cast<RealT>(weights.Size());
    
    if ( samples < 3 )
      throw ExceptionC("Sample size too small in FitAffine(). ");
    
    MatrixC A(samples,3);
    VectorC b(samples);
    VectorC c(samples);
    UIntT i = 0;
    
    for(;i<org.Size();i++) {
      
      RealT x1, y1, x2, y2;
      x1=org[i][0]; y1=org[i][1];
      x2=newPos[i][0]; y2=newPos[i][1];
      RealT w = weights[i] / meanWeight; // Normalise weight.
      
      A[i][0] = x1 *w; 
      A[i][1] = y1*w; 
      A[i][2] = 1*w;
      b[i] = x2*w;
      c[i] = y2*w;
    }
    
    residual = 0;
    
    MatrixC tA = A.Copy();
    if(A.Rows() == A.Cols()) {
      // solve for solution vector
      residual = 0;
      if(!SolveIP(tA,b)) 
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
      if(!SolveIP(A,c))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
    } else {
      if(!LeastSquaresQR_IP(tA,b,residual))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
      if(!LeastSquaresQR_IP(A,c,residual))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
    }
    
    Matrix2dC sr(b[0],b[1],
		 c[0],c[1]);
    Vector2dC tr(b[2],c[2]);
    return Affine2dC(sr,tr);
    
  }
  
  //: Fit an affine transform given to the mapping between original and newPos.
  Affine2dC FitAffine(const SArray1dC<Point2dC> &org,const SArray1dC<Point2dC> &newPos) {
    RealT residual;
    return FitAffine(org,newPos,residual);
  }
  
  //: Fit an affine transform given to the mapping between original and newPos.
  
  Affine2dC FitAffine(const DListC<Point2dC> &org,const DListC<Point2dC> &newPos) {
    RealT residual;
    return FitAffine(org,newPos,residual);
  }
  
  //: Fit affine transform from mapping of 3 points.
  bool FitAffine(const Point2dC &p1a,const Point2dC &p1b,
		 const Point2dC &p2a,const Point2dC &p2b,
		 const Point2dC &p3a,const Point2dC &p3b,
		 Affine2dC &affine) {

    Matrix3dC A(p1a[0],p1a[1],1.0,
		p2a[0],p2a[1],1.0,
		p3a[0],p3a[1],1.0);
    Vector3dC b(p1b[0],p2b[0],p3b[0]);
    Vector3dC c(p1b[1],p2b[1],p3b[1]);
    
    Matrix3dC tA = A; // Copy matrix, else it will be destryed by the first solve
    
    if(!SolveIP(tA,b))
      return false;
    if(!SolveIP(A,c))
      return false;
    
    affine.Translation()[0] = b[2];
    affine.Translation()[1] = c[2];
    affine.SRMatrix() = Matrix2dC(b[0],b[1],c[0],c[1]);
    
    return true;
  }
  
  
  //: Fit an affine transform given to the mapping between original and newPos.
  
  Affine2dC FitAffine(const DListC<Point2dC> &org,const DListC<Point2dC> &newPos,RealT &residual) {
    UIntT samples = org.Size();
    RavlAssertMsg(samples == newPos.Size(),"Affine2dC FitAffine(), Point arrays must have the same size.");
    
    if ( samples < 3 )
      throw ExceptionC("Sample size too small in FitAffine2dPointsBodyC::FitModel(). ");
    
    MatrixC A(samples,3);
    VectorC b(samples);
    VectorC c(samples);
    UIntT i = 0;
    DLIterC<Point2dC> it1(org);
    DLIterC<Point2dC> it2(newPos);
    for(;it1;it1++,it2++,i++) {
      RavlAssert(it2.IsElm());
      RealT x1, y1, x2, y2;
      x1=(*it1)[0]; y1=(*it1)[1];
      x2=(*it2)[0]; y2=(*it2)[1];
      
      A[i][0] = x1; 
      A[i][1] = y1; 
      A[i][2] = 1;
      b[i] = x2;
      c[i] = y2;
    }
    MatrixC tA = A.Copy();
    if(A.Rows() == A.Cols()) {
      // solve for solution vector
      residual = 0;
      if(!SolveIP(tA,b))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
      if(!SolveIP(A,c))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
    } else {
      if(!LeastSquaresQR_IP(tA,b,residual))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
      if(!LeastSquaresQR_IP(A,c,residual))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
    }
    
    Matrix2dC sr(b[0],b[1],
		 c[0],c[1]);
    Vector2dC tr(b[2],c[2]);
    return Affine2dC(sr,tr);
    
  }
  
  //: Fit an affine transform given to the mapping between original and newPos.
  
  // FIXME :- This can be done more efficiently.
  
  template <typename ContainterOfPoint2dT> 
  Affine2dC FitAffineTempl(const ContainterOfPoint2dT &org,const ContainterOfPoint2dT &newPos,RealT &residual) {
    RavlAssertMsg(org.Size() == newPos.Size(),"Affine2dC FitAffine(), Point arrays must have the same size.");
    
    UIntT samples = org.Size();
    if ( samples < 3 )
      throw ExceptionC("Sample size too small in FitAffine2dPointsBodyC::FitModel(). ");
    
    MatrixC A(samples,3);
    VectorC b(samples);
    VectorC c(samples);
    UIntT i = 0;
    for(typename ContainterOfPoint2dT::IteratorT itOrg(org), itNewPos(newPos);itOrg;itOrg++,itNewPos++,i++) {
      RealT x1, y1, x2, y2;
      x1=itOrg.Data()[0]; y1=itOrg.Data()[1];
      x2=itNewPos.Data()[0]; y2=itNewPos.Data()[1];
      
      A[i][0] = x1; 
      A[i][1] = y1; 
      A[i][2] = 1;
      b[i] = x2;
      c[i] = y2;
    }
    MatrixC tA = A.Copy();
    if(A.Rows() == A.Cols()) {
      // solve for solution vector
      residual = 0;
      if(!SolveIP(tA,b))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
      if(!SolveIP(A,c))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
    } else {
      if(!LeastSquaresQR_IP(tA,b,residual))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
      if(!LeastSquaresQR_IP(A,c,residual))
	throw ExceptionNumericalC("Dependent linear equations in FitAffine() ");
    }
    
    Matrix2dC sr(b[0],b[1],
		 c[0],c[1]);
    Vector2dC tr(b[2],c[2]);
    return Affine2dC(sr,tr);
  }
  Affine2dC FitAffine(const SArray1dC<Point2dC> &org,const SArray1dC<Point2dC> &newPos,RealT &residual) 
  {
    return FitAffineTempl(org, newPos, residual);
  }
  Affine2dC FitAffine(const DArray1dC<Point2dC> &org,const DArray1dC<Point2dC> &newPos,RealT &residual) 
  {
    return FitAffineTempl(org, newPos, residual);
  }
  
  template <typename ContainterOfPairPoint2dT> 
  Affine2dC FitAffineTempl(const ContainterOfPairPoint2dT &matchPairs,RealT &residual)
  {
    BufferC<Point2dC> mpBuf(matchPairs.Size()*2, (Point2dC*)&(matchPairs[0]));
    Slice1dC<Point2dC> from(mpBuf, matchPairs.Size(), 0, 2);
    Slice1dC<Point2dC> to(mpBuf, matchPairs.Size(), 1, 2);
    
    return FitAffineTempl(from,to,residual);
  }
  Affine2dC FitAffine(const SArray1dC<PairC<Point2dC> > &matchPairs,RealT &residual) 
  {
    return FitAffineTempl(matchPairs, residual);
  }
  Affine2dC FitAffine(const DArray1dC<PairC<Point2dC> > &matchPairs,RealT &residual) 
  {
    return FitAffineTempl(matchPairs, residual);
  }
  
  void Affine2dC::Rotate(RealT A) {
    RealT TC = Cos(A),TS = Sin(A);    
    Matrix2dC result;
#if !RAVL_COMPILER_VISUALCPP
    MulM(SR,Matrix2dC(TC,-TS,TS,TC),result);
#else
    MulM<RealT,2,2,2>(SR,Matrix2dC(TC,-TS,TS,TC),result);
#endif
    SR = result;
  }
  
  
  bool FitSimilarity(const SArray1dC<Point2dC> &points1,
                     const SArray1dC<Point2dC> &points2,
                     Matrix2dC &rotation,
                     Vector2dC &translation,
                     RealT &scale,
                     bool forceUnitScale,
                     bool forceUnitRotation
                     ) 
  {
    
    // Compute the means.
    
    RealT n = points1.Size();
    Point2dC mean1(0,0),mean2(0,0);
    for(SArray1dIter2C<Point2dC,Point2dC> it(points1,points2);it;it++) {
      mean1 += it.Data1();
      mean2 += it.Data2();
    }
    
    mean1 /= n;
    mean2 /= n;
    
    // Compute the covariance matrix.
    
    Matrix2dC covar(0,0,
                    0,0);
    
    RealT ps1 = 0,ps2 = 0;
    
    for(SArray1dIter2C<Point2dC,Point2dC> it(points1,points2);it;it++) {
      Point2dC p1 = (it.Data1() - mean1);
      ps1 += Sqr(p1[0]) + Sqr(p1[1]);
      Point2dC p2 = (it.Data2() - mean2);      
      ps2 += Sqr(p2[0]) + Sqr(p2[1]);
      for(int i = 0;i < 2;i++) {
        covar[i][0] += p1[0] * p2[i];
        covar[i][1] += p1[1] * p2[i];
      }
    }
    
    // Compute the scaling.
    scale = Sqrt(ps2/ps1);
    
    // Sometimes we do not want to rotate, useful for cropping things as is out
    // of the image
    if(forceUnitRotation) {
      rotation = Matrix2dC(1., 0., 0., 1.);
    }
    // Otherwise compute the rotation from the covariance matrix.
    else {

      covar /= n;
      Matrix2dC u,v;
      Vector2dC d = RavlN::SVD_IP(covar,u,v);
    
      // TODO :- Make this faster by avoiding use of so many temporaries.
    
      Matrix2dC s(1,0,
                0,1);
    
      // Correct mirroring.

      if((u.Det() * v.Det()) < 0) {
        s[1][1] = -1;
        d[1] *= -1;
      }
    
      rotation = u * s * v.T();
    }

    // Compute the translation.
    if(forceUnitScale) {
      translation = mean2 - rotation * mean1;
    } else {
      translation = mean2 - rotation * mean1 * scale;
    }
    
    return true;
  }

  //! Fit a rigid transform between the two point sets.
  //! If 'forceUnitScale' is true then unit scaling will be assumed.
  
  bool FitSimilarity(const SArray1dC<Point2dC> &points1,
                     const SArray1dC<Point2dC> &points2,
                     Affine2dC &transform,
                     bool forceUnitScale,
                     bool forceUnitRotation
                     ) {
    Matrix2dC rotation;
    Vector2dC translation;
    RealT scale;

    if(!FitSimilarity(points1,points2,rotation,translation,scale,forceUnitScale,forceUnitRotation))
      return false;

    if(forceUnitScale)
      transform = Affine2dC(rotation,translation);
    else
      transform = Affine2dC(rotation * scale,translation);
    return true;
  }


  //: Decompose affine transform.
  
  void Affine2dC::Decompose(Point2dC &translation,Vector2dC &scaling,RealT &skew,RealT &rotation) const {
    translation = T;
    Matrix2dC u, v;
    Vector2dC mag = SVD(SR,u,v);
    
    // Compute the scaling matrix.
    Matrix2dC scalingMat = v * Matrix2dC(mag[0],0,
                                       0,mag[1]) * v.T();
    scaling[0] = scalingMat[0][0]; // Row scale
    scaling[1] = scalingMat[1][1]; // Col scale
    skew       = scalingMat[1][0]; // Skew.
    
    // Get the rotation.
    Matrix2dC rot = u * v.T();
    rotation = ATan2(rot[0][1],rot[0][0]);
    
  }

  
  //: Compose an affine transform the components generated from decompose.
  
  Affine2dC Affine2dC::Compose(Point2dC translation,Vector2dC scaling,RealT skew,RealT rotation) {
    Matrix2dC scalingMat(scaling[0],skew,
                      skew,scaling[1]);
    Matrix2dC rotationMat(Cos(rotation),Sin(rotation),
                       -Sin(rotation),Cos(rotation));
    return Affine2dC(rotationMat * scalingMat,translation);
  }


  //: Apply a affine transform to a point set

  SArray1dC<Point2dC> operator*(const FAffineC<2> &trans,const SArray1dC<Point2dC> &points) {
    SArray1dC<Point2dC> ret(points.Size());
    for(SArray1dIter2C<Point2dC,Point2dC> it(ret,points);it;it++)
      it.Data1() = trans * it.Data2();
    return ret;
  }
#endif
}
