// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2007, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Affine3d.hh"
//#include "Ravl/Matrix.hh"
//#include "Ravl/Vector.hh"
//#include "Ravl/Point2d.hh"
//#include "Ravl/Point3d.hh"
//#include "Ravl/SArray1dIter2.hh"
//#include "Ravl/LeastSquares.hh"
//#include "Ravl/Sums1d2.hh"
//
//namespace RavlN {
//
//  //: Fit an affine transformation
//
//  Affine3dC FitAffine(const SArray1dC<Point3dC> &org,const SArray1dC<Point3dC> &newPos,RealT &residual) {
//    RavlAssertMsg(org.Size() == newPos.Size(),"Affine3dC FitAffine(), Point arrays must have the same size.");
//
//    UIntT samples = org.Size();
//    if ( samples < 4 )
//      throw ExceptionC("Sample size too small in FitAffine(). ");
//
//    MatrixC A(samples,4);
//    VectorC b(samples);
//    VectorC c(samples);
//    VectorC d(samples);
//    UIntT i = 0;
//    for(SArray1dIter2C<Point3dC,Point3dC> it(org,newPos);it;it++,i++) {
//      RealT x1, y1, z1, x2, y2, z2;
//      x1=it.Data1()[0]; y1=it.Data1()[1]; z1=it.Data1()[2];
//      x2=it.Data2()[0]; y2=it.Data2()[1]; z2=it.Data2()[2];
//
//      A[i][0] = x1;
//      A[i][1] = y1;
//      A[i][2] = z1;
//      A[i][3] = 1;
//      b[i] = x2;
//      c[i] = y2;
//      d[i] = z2;
//    }
//
//    MatrixC tA = A.Copy();
//    MatrixC sA = A.Copy();
//    if(A.Rows() == A.Cols()) {
//      // solve for solution vector
//      residual = 0;
//      if(!SolveIP(tA,b))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!SolveIP(sA,c))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!SolveIP(A,d))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//    } else {
//      if(!LeastSquaresQR_IP(tA,b,residual))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!LeastSquaresQR_IP(sA,c,residual))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!LeastSquaresQR_IP(A,d,residual))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//    }
//
//    Matrix3dC sr(b[0],b[1],b[2],
//		 c[0],c[1],c[2],
//                 d[0],d[1],d[2]);
//    Vector3dC tr(b[3],c[3],d[3]);
//
//    return Affine3dC(sr,tr);
//  }
//
//  //: Fit points on a 2d plane (this assumes the z position is always zero) to a 3d position.
//
//  Affine3dC FitAffine(const SArray1dC<Point2dC> &org,const SArray1dC<Point3dC> &newPos,RealT &residual) {
//    RavlAssertMsg(org.Size() == newPos.Size(),"Affine3dC FitAffine(), Point arrays must have the same size.");
//
//    UIntT samples = org.Size();
//    if ( samples < 3 )
//      throw ExceptionC("Sample size too small in FitAffine(). ");
//
//    MatrixC A(samples,3);
//    VectorC b(samples);
//    VectorC c(samples);
//    VectorC d(samples);
//    UIntT i = 0;
//    for(SArray1dIter2C<Point2dC,Point3dC> it(org,newPos);it;it++,i++) {
//      RealT x1, y1, x2, y2, z2;
//      x1=it.Data1()[0]; y1=it.Data1()[1];
//      x2=it.Data2()[0]; y2=it.Data2()[1]; z2=it.Data2()[2];
//
//      A[i][0] = x1;
//      A[i][1] = y1;
//      A[i][2] = 1;
//      b[i] = x2;
//      c[i] = y2;
//      d[i] = z2;
//    }
//
//    MatrixC tA = A.Copy();
//    MatrixC sA = A.Copy();
//    if(A.Rows() == A.Cols()) {
//      // solve for solution vector
//      residual = 0;
//      if(!SolveIP(tA,b))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!SolveIP(sA,c))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!SolveIP(A,d))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//    } else {
//      if(!LeastSquaresQR_IP(tA,b,residual))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!LeastSquaresQR_IP(sA,c,residual))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//      if(!LeastSquaresQR_IP(A,d,residual))
//	throw ExceptionNumericalC("Dependent linear equations in FitAffine, 3d ");
//    }
//
//    Matrix3dC sr(b[0],b[1],0,
//		 c[0],c[1],0,
//                 d[0],d[1],0);
//
//    // Fill in values as a rotation into the z axis.
//    for(int i = 0;i < 3;i++) {
//      RealT s = Sqr(sr[i][0]) + Sqr(sr[i][1]);
//      if(s < 1.0)
//        sr[i][2] = Sqrt(1-s);
//    }
//
//    Vector3dC tr(b[2],c[2],d[2]);
//
//    return Affine3dC(sr,tr);
//  }
//
//
//  //: Fit an affine transformation given some directions and positions.
//  // The transform takes a point and from newPos into dir's
//
//  Affine3dC FitAffineDirection(const SArray1dC<Point3dC> &points,const SArray1dC<Vector3dC> &directions) {
//    RavlAssert(points.Size() == directions.Size());
//
//
//    // Gather some stats about the 3d points.
//
//    Sums1d2C sums[3];
//    for(SArray1dIterC<Point3dC> it(points);it;it++) {
//      for(int i = 0;i < 3;i++)
//        sums[i] += (*it)[i];
//    }
//
//    Vector3dC gain;
//    Vector3dC offset;
//    for(int i = 0;i < 3;i++) {
//      offset[i] = sums[i].Mean();
//#if 1
//      RealT var = sums[i].Variance(false);
//      if(IsNan(var) || IsInf(var)) {
//        gain[i] = 1.0;
//      } else
//        gain[i] = 1.0/Sqrt(var);
//#else
//      gain[i] = 1.0;
//#endif
//    }
//
//    // Normalise points and form matrix.
//
//    MatrixC A(points.Size() * 3,12);
//
//    // v := {v0,v1,v2};
//    // q := {q0,q1,q2};
//    // ar := {{ar00,ar01,a02},{ar10,ar11,ar12},{ar20,ar21,ar22}};
//    // at := {at0,at1,at2};
//
//    // CrossProduct(v,ar * q + at) = 0;
//
//    // ar00 ar01 ar02  ar10 ar11 ar12  ar20 ar21 ar22  at0 at1 at2
//
//
//    // q -> v
//
//    UIntT row = 0;
//    for(SArray1dIter2C<Vector3dC,Point3dC> it(directions,points);it;it++) {
//      Point3dC q = (it.Data2() - offset) * gain;
//      Vector3dC v = it.Data1().Unit();
//
//      // v1*ar20*q0 +v1*ar21*q1 +v1*ar22*q2 -q2*v2*ar12 -q1*v2*ar11 -q0*v2*ar10 +v1*at2 -v2*at1
//      {
//        A[row][ 0] = 0;            // ar00
//        A[row][ 1] = 0;            // ar01
//        A[row][ 2] = 0;            // ar02
//
//        A[row][ 3] = -v[2] * q[0];  // ar10
//        A[row][ 4] = -v[2] * q[1];  // ar11
//        A[row][ 5] = -v[2] * q[2];  // ar12
//
//        A[row][ 6] = v[1] * q[0];  // ar20
//        A[row][ 7] = v[1] * q[1];  // ar21
//        A[row][ 8] = v[1] * q[2];  // ar22
//
//        A[row][ 9] = 0;            // at0
//        A[row][10] = -v[2];        // at1
//        A[row][11] = v[1];         // at2
//
//        row++;
//      }
//
//      //v2*ar00*q0 +v2*ar01*q1 +v2*ar02*q2 -q2*v0*ar22 -q1*v0*ar21 -q0*v0*ar20 +v2*at0 -v0*at2
//      {
//        A[row][ 0] = v[2] * q[0]; // ar00
//        A[row][ 1] = v[2] * q[1]; // ar01
//        A[row][ 2] = v[2] * q[2]; // ar02
//
//        A[row][ 3] = 0;            // ar10
//        A[row][ 4] = 0;            // ar11
//        A[row][ 5] = 0;            // ar12
//
//        A[row][ 6] = -v[0] * q[0];  // ar20
//        A[row][ 7] = -v[0] * q[1];  // ar21
//        A[row][ 8] = -v[0] * q[2];  // ar22
//
//        A[row][ 9] = v[2];         // at0
//        A[row][10] = 0;            // at1
//        A[row][11] = -v[0];        // at2
//
//        row++;
//      }
//
//      //v0*ar10*q0 +v0*ar11*q1 +v0*ar12*q2 -q2*v1*ar02 -q1*v1*ar01 -q0*v1*ar00 +v0*at1 -v1*at0
//      {
//        A[row][ 0] = -v[1] * q[0];   // ar00
//        A[row][ 1] = -v[1] * q[1];   // ar01
//        A[row][ 2] = -v[1] * q[2];   // ar02
//
//        A[row][ 3] = v[0] * q[0];    // ar10
//        A[row][ 4] = v[0] * q[1];    // ar11
//        A[row][ 5] = v[0] * q[2];    // ar12
//
//        A[row][ 6] = 0;              // ar20
//        A[row][ 7] = 0;              // ar21
//        A[row][ 8] = 0;              // ar22
//
//        A[row][ 9] = -v[1];          // at0
//        A[row][10] = v[0];           // at1
//        A[row][11] = 0;              // at2
//
//        row++;
//      }
//    }
//
//    RavlAssert(row == A.Rows());
//
//    VectorC v;
//    LeastSquaresEq0Mag1(A,v);
//
//    Matrix3dC SR(v[0]*gain[0],v[1]*gain[1],v[2]*gain[2],
//                 v[3]*gain[0],v[4]*gain[1],v[5]*gain[2],
//                 v[6]*gain[0],v[7]*gain[1],v[8]*gain[2]);
//
//
//    Vector3dC T(v[9],v[10],v[11]);
//
//    // (SR * In) + T;
//    // q = (it.Data2() - offset) * gain;
//
//    Affine3dC ret;
//
//    // Is the solution a mirror image of the required one ?
//    if(SR.Det() >= 0)
//      ret = Affine3dC(SR,T - SR * offset);
//    else
//      ret = Affine3dC(SR * -1,(T - SR * offset) * -1);
//
//    //std::cerr << "Aff=" << ret << " Det=" << ret.SRMatrix().Det() << "\n";
//
//    return ret;
//  }
//
//
//  //! Fit a rigid transform between the two point sets.
//  //
//  // See 'Least-Squares Estimation of Transformation Parametres Between Two Point Patterns' by
//  // Shinji Umeyama.  IEEE Transactions on Pattern Analysis and Machine Intelligence Vol 13, No 4
//  // April 1991. Page 376
//  //
//  // TODO: This actually works with any number of dimensions.  Generalise code.
//
//  bool FitSimilarity(const SArray1dC<Point3dC> &points1,
//                     const SArray1dC<Point3dC> &points2,
//                     Matrix3dC &rotation,
//                     Vector3dC &translation,
//                     RealT &scale,
//                     bool forceUnitScale
//                     )
//  {
//
//    // Compute the means.
//
//    RealT n = points1.Size();
//    Point3dC mean1(0,0,0),mean2(0,0,0);
//    for(SArray1dIter2C<Point3dC,Point3dC> it(points1,points2);it;it++) {
//      mean1 += it.Data1();
//      mean2 += it.Data2();
//    }
//
//    mean1 /= n;
//    mean2 /= n;
//
//    // Compute the covariance matrix.
//
//    Matrix3dC covar(0,0,0,
//                    0,0,0,
//                    0,0,0);
//
//    RealT ps1 = 0,ps2 = 0;
//
//    for(SArray1dIter2C<Point3dC,Point3dC> it(points1,points2);it;it++) {
//      Point3dC p1 = (it.Data1() - mean1);
//      ps1 += Sqr(p1[0]) + Sqr(p1[1]) + Sqr(p1[2]);
//      Point3dC p2 = (it.Data2() - mean2);
//      ps2 += Sqr(p2[0]) + Sqr(p2[1]) + Sqr(p2[2]);
//      for(int i = 0;i < 3;i++) {
//        covar[i][0] += p1[0] * p2[i];
//        covar[i][1] += p1[1] * p2[i];
//        covar[i][2] += p1[2] * p2[i];
//      }
//    }
//
//    // Compute the scaling.
//    scale = Sqrt(ps2/ps1);
//
//    // Compute the rotation from the covariance matrix.
//    covar /= n;
//    Matrix3dC u,v;
//    Vector3dC d = RavlN::SVD_IP(covar,u,v);
//
//    // TODO :- Make this faster by avoiding use of so many temporaries.
//
//    Matrix3dC s(1,0,0,
//                0,1,0,
//                0,0,1);
//
//    // Correct mirroring.
//
//    if((u.Det() * v.Det()) < 0) {
//      s[2][2] = -1;
//      d[2] *= -1;
//    }
//
//    rotation = u * s * v.T();
//
//    // Compute the translation.
//    if(forceUnitScale) {
//      translation = mean2 - rotation * mean1;
//    } else {
//      translation = mean2 - rotation * mean1 * scale;
//    }
//
//    return true;
//  }
//
//  //! Fit a rigid transform between the two point sets.
//  //! If 'forceUnitScale' is true then unit scaling will be assumed.
//
//  bool FitSimilarity(const SArray1dC<Point3dC> &points1,
//                     const SArray1dC<Point3dC> &points2,
//                     Affine3dC &transform,
//                     bool forceUnitScale
//                     ) {
//    Matrix3dC rotation;
//    Vector3dC translation;
//    RealT scale;
//    if(!FitSimilarity(points1,points2,rotation,translation,scale,forceUnitScale))
//      return false;
//    if(forceUnitScale)
//      transform = Affine3dC(rotation,translation);
//    else
//      transform = Affine3dC(rotation * scale,translation);
//    return true;
//  }
//
//
//
//}
